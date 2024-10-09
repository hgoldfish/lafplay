#include <QtCore/qloggingcategory.h>
#include <QtGui/qpainter.h>
#include "animation_viewer_p.h"

Q_LOGGING_CATEGORY(logger, "lafplay.ffmpeg")

AVContext::AVContext()
    : formatCtx(nullptr)
    , codecCtx(nullptr)
    , swsContext(nullptr)
    , nativeFrame(nullptr)
    , rgbFrame(nullptr)
    , videoStream(0)
    , timeBase(0.001)
{
}

AVContext::~AVContext()
{
    if (rgbFrame) {
        if (rgbFrame->data[0]) {
            av_freep(&rgbFrame->data[0]);
        }
        av_frame_free(&rgbFrame);
    }
    if (nativeFrame) {
        av_frame_free(&nativeFrame);
    }
    if (swsContext) {
        sws_freeContext(swsContext);
    }
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
    }
    if (formatCtx) {
        avformat_close_input(&formatCtx);
    }
}

bool AVContext::initSwsContext()
{
    if (swsContext) {
        return true;
    }
    AVPixelFormat orignalFormat;
    switch (codecCtx->pix_fmt) {
    // 这一段代码是干啥用的？
    case AV_PIX_FMT_YUVJ420P:
        orignalFormat = AV_PIX_FMT_YUV420P;
        break;
    case AV_PIX_FMT_YUVJ422P:
        orignalFormat = AV_PIX_FMT_YUV422P;
        break;
    case AV_PIX_FMT_YUVJ444P:
        orignalFormat = AV_PIX_FMT_YUV444P;
        break;
    case AV_PIX_FMT_YUVJ440P:
        orignalFormat = AV_PIX_FMT_YUV440P;
        break;
    default:
        orignalFormat = codecCtx->pix_fmt;
        break;
    }
    swsContext = sws_getContext(codecCtx->width, codecCtx->height, orignalFormat, codecCtx->width, codecCtx->height,
                                AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsContext) {
        qCWarning(logger) << "can not allocate sws context.";
        return false;
    }
    return true;
}

AVContext *makeContext(const QString &url, QString *reason)
{
    QScopedPointer<AVContext> context(new AVContext());
    if (int ret = avformat_open_input(&context->formatCtx, qPrintable(url), nullptr, nullptr)) {
        if (reason) {
            *reason = QString::fromUtf8("can not open file: %1").arg(ret);
        }
        context->formatCtx = nullptr;
        return nullptr;
    }

    context->videoStream = av_find_best_stream(context->formatCtx, AVMEDIA_TYPE_VIDEO, 0, 0, nullptr, 0);
    if (context->videoStream < 0) {
        if (reason) {
            *reason = QString::fromUtf8("can not find video stream.");
        }
        return nullptr;
    }

    AVStream *stream = context->formatCtx->streams[context->videoStream];
    AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        if (reason) {
            *reason = QString::fromUtf8("can not find codec.");
        }
        return nullptr;
    }
    context->codecCtx = avcodec_alloc_context3(codec);
    if (!context->codecCtx) {
        if (reason) {
            *reason = QString::fromUtf8("can not allocate codec context.");
        }
        return nullptr;
    }
    if (avcodec_parameters_to_context(context->codecCtx, stream->codecpar) < 0) {
        if (reason) {
            *reason = QString::fromUtf8("can not set parameters to codec context.");
        }
        return nullptr;
    }
    if (avcodec_open2(context->codecCtx, nullptr, nullptr)) {
        if (reason) {
            *reason = QString::fromUtf8("can not open codec context.");
        }
        return nullptr;
    }
    context->nativeFrame = av_frame_alloc();
    AVFrame *rgbFrame = av_frame_alloc();
    context->rgbFrame = rgbFrame;
    if (av_image_alloc(rgbFrame->data, rgbFrame->linesize, context->codecCtx->width, context->codecCtx->height,
                       AV_PIX_FMT_RGBA, 32)
        < 0) {
        if (reason) {
            *reason = QString::fromUtf8("can not allocate rgb frame buffer.");
        }
        return nullptr;
    }
    context->width = rgbFrame->width = context->codecCtx->width;
    context->height = rgbFrame->height = context->codecCtx->height;
    rgbFrame->format = AV_PIX_FMT_RGBA;
    context->timeBase = av_q2d(stream->time_base);
    return context.take();
}

DecoderThread::DecoderThread(QObject *viewerPrivate)
    : viewerPrivate(viewerPrivate)
    , state(AnimationViewer::NotParsed)
    , frameBufferSize(10)
    , autoRepeat(false)
    , exiting(false)
{
}

void DecoderThread::run()
{
    bool ready = false;
    while (!isExiting()) {
        const Command &cmd = commands.get();
        qCDebug(logger) << "got command:" << cmd.type << cmd.str_arg;
        switch (cmd.type) {
        case Command::Invalid:
            return;
        case Command::Parse:
            state = AnimationViewer::NotParsed;
            if (parse(cmd.str_arg)) {
                state = AnimationViewer::ParseSuccess;
            } else {
                state = AnimationViewer::ParseFailed;
            }
            QMetaObject::invokeMethod(viewerPrivate, "parsed", Q_ARG(int, state));
            break;
        case Command::Play:
        play:
            if (state != AnimationViewer::ParseSuccess) {
                qCDebug(logger) << "can not play: state=" << state;
                break;
            }
            ready = false;
            switch (play()) {
            case Finished:
                qCDebug(logger) << "play finished.";
                break;
            case Ready:
                qCDebug(logger) << "play ready.";
                ready = true;
                break;
            case Exit:
                qCDebug(logger) << "play exit.";
                return;
            case Error:
                qCDebug(logger) << "play error.";
                stop();
                break;
            }
            break;
        case Command::Stop:
            stop();
            break;
        case Command::Seek:
            seek(cmd.int_arg1);
            if (ready) {
                goto play;
            } else {
                break;
            }
        case Command::Resize:
            if (context) {
                context->width = static_cast<int>(cmd.int_arg1);
                context->height = static_cast<int>(cmd.int_arg2);
            }
            if (ready) {
                goto play;
            } else {
                break;
            }
        default:
            qCWarning(logger) << "unknown command type:";
        }
    }
}

bool DecoderThread::parse(const QString &url)
{
    Q_ASSERT(!url.isEmpty() && state == AnimationViewer::NotParsed);
    QString reason;
    QScopedPointer<AVContext> context(makeContext(url, &reason));
    if (!context) {
        qCDebug(logger) << reason;
        return false;
    } else {
        this->context.reset(context.take());
    }
    return true;
}

struct ScopedPointerAvPacketDeleter
{
    static inline void cleanup(AVPacket *&pointer) { av_packet_free(&pointer); }
};

DecoderThread::PlayResult DecoderThread::play()
{
    Q_ASSERT(!context.isNull() && context->isValid());

    // 优先处理命令，如果没有命令，则主要去读数据。
    while (true) {
        if (isExiting()) {
            return PlayResult::Exit;
        }
        if (!commands.isEmpty()) {
            return PlayResult::Ready;
        }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        int bsize = frameBufferSize.loadRelaxed();
#else
        int bsize = frameBufferSize.load();
#endif
        if (frames.size() >= bsize) {
            return PlayResult::Ready;
        }
        QScopedPointer<AVPacket, ScopedPointerAvPacketDeleter> packet(av_packet_alloc());
        if (av_read_frame(context->formatCtx, packet.data())) {
            frames.put(VideoFrame::makeFinishedFrame());
            return PlayResult::Finished;
        }

        if (isExiting()) {
            return PlayResult::Exit;
        }

        // 跳过音频等等。。
        if (packet->stream_index != context->videoStream) {
            continue;
        }
        int r = avcodec_send_packet(context->codecCtx, packet.data());
        qCDebug(logger) << "发送帧:" << r;
        if (r != 0) {
            if (r == AVERROR(EAGAIN)) {
                qCWarning(logger) << "too much packet.";
            } else {
                qCWarning(logger) << "can not send packet.";
            }
            return PlayResult::Error;
        }

        // 上面已经丢了一些帧进去，把所有数据都解码出来再处理其它事。
        while (true) {
            if (isExiting()) {
                return PlayResult::Exit;
            }
            if (!commands.isEmpty()) {
                return PlayResult::Ready;
            }
            if (frames.size() >= bsize) {
                return PlayResult::Ready;
            }

            r = avcodec_receive_frame(context->codecCtx, context->nativeFrame);
            qCDebug(logger) << "接收帧:" << r;
            if (r == AVERROR(EAGAIN)) {
                break;
            } else if (r != 0) {
                return PlayResult::Error;
            } else {  // r == 0
                AVFrame *t;
                if (context->codecCtx->pix_fmt == AV_PIX_FMT_RGBA) {
                    t = context->nativeFrame;
                } else {
                    if (!context->initSwsContext()) {
                        return PlayResult::Error;
                    }
                    if (isExiting()) {
                        return PlayResult::Exit;
                    }
                    int h = sws_scale(context->swsContext, context->nativeFrame->data, context->nativeFrame->linesize,
                                      0, context->codecCtx->height, context->rgbFrame->data,
                                      context->rgbFrame->linesize);
                    if (h <= 0) {
                        return PlayResult::Error;
                    }

                    t = context->rgbFrame;
                }
                QImage image(reinterpret_cast<const uchar *>(t->data[0]), t->width, t->height, t->linesize[0],
                             QImage::Format_RGBA8888_Premultiplied);

                // 把 pts 转成以 ms 为单位，省事一些。
                int64_t pts = static_cast<int64_t>(context->nativeFrame->pts * context->timeBase * 1000.0);
                VideoFrame vf(image.copy(), pts, context->nativeFrame->pkt_dts);
                if (isExiting()) {
                    return PlayResult::Exit;
                }
                qCDebug(logger) << "解压成功一个帧，放到队列里面。";
                frames.put(vf);
            }
        }
    }
}

void DecoderThread::stop()
{
    state = AnimationViewer::NotParsed;
    context.reset();
}

void DecoderThread::seek(int64_t pts) { }

void DecoderThread::shutdown()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    return exiting.storeRelease(true);
#else
    return exiting.store(true);
#endif
}

bool DecoderThread::isExiting() const
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    return exiting.loadAcquire();
#else
    return exiting.load();
#endif
}

AnimationViewerPrivate::AnimationViewerPrivate(AnimationViewer *q)
    : q_ptr(q)
    , thread(new DecoderThread(this))
    , playTime(0)
    , autoRepeat(true)
    , pausedBeforeHidden(true)
{
#if LIBAVFORMAT_VERSION_INT <= AV_VERSION_INT(58, 9, 100)
    static QAtomicInt registeredFormats(z0);
    if (!registeredFormats.fetchAndAddRelaxed(1)) {
        av_register_all();
    }
#endif
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
    nextFrameTimer.setInterval(10);
    nextFrameTimer.setSingleShot(false);
    connect(&nextFrameTimer, SIGNAL(timeout()), this, SLOT(next()));
}

AnimationViewerPrivate::~AnimationViewerPrivate()
{
    thread->shutdown();
    thread = nullptr;
}

void AnimationViewerPrivate::parsed(int result)
{
    Q_Q(AnimationViewer);
    emit q->parsed(static_cast<AnimationViewer::ParseResult>(result));
}

void AnimationViewerPrivate::next()
{
    Q_Q(AnimationViewer);

    BlockingQueue<VideoFrame> &frames = thread->frames;
    if (frames.isEmpty()) {
        qCDebug(logger) << "空队列，再等一会儿。";
        return;
    }

    VideoFrame f;
    while (true) {
        f = frames.get();
        if (!f.isValid()) {
            qCDebug(logger) << "不正确的帧。";
            q->stop();
            return;
        }
        if (f.isFinished()) {
            qCDebug(logger) << "播放结束。";
            current = QImage();
            q->update();
            // XXX 未必需要停止，可以使用 seek() 返回到第 0 帧。
            // q->stop();
            emit q->finished();
            return;
        }
        // 丢弃掉一直没有播放的帧，直接从最近一个帧开始播放。
        // 一直读取到 frames 为空，或者下一帧的播放时间是 playTime 之后，或者下个帧是 EOF 帧
        if (frames.isEmpty() || frames.peek().pts == 0 || frames.peek().pts > playTime) {
            break;
        }
    }
    qCDebug(logger) << "获得一个帧准备开始播放:" << f.pts << playTime;
    if (f.pts > playTime) {
        frames.returnsForcely(f);
        playTime += nextFrameTimer.interval();
        return;
    }
    playTime += nextFrameTimer.interval();
    current = f.image;
    q->update();
    if (frames.isEmpty()) {
        DecoderThread::Command cmd(DecoderThread::Command::Play);
        thread->commands.put(cmd);
    }
}

AnimationViewer::AnimationViewer(QWidget *parent)
    : QWidget(parent)
    , dd_ptr(new AnimationViewerPrivate(this))
{
    connect(this, &AnimationViewer::finished, this, &AnimationViewer::play);
}

AnimationViewer::~AnimationViewer()
{
    delete dd_ptr;
}

QString AnimationViewer::url() const
{
    Q_D(const AnimationViewer);
    return d->mediaUrl;
}

bool AnimationViewer::isPlaying() const
{
    Q_D(const AnimationViewer);
    return d->nextFrameTimer.isActive();
}

void AnimationViewer::setUrl(const QString &url)
{
    Q_D(AnimationViewer);
    d->mediaUrl = url;
    DecoderThread::Command cmd(DecoderThread::Command::Parse);
    cmd.str_arg = url;
    d->thread->commands.put(cmd);
}

void AnimationViewer::setFrameBufferSize(int size)
{
    Q_D(AnimationViewer);
    d->thread->frameBufferSize.storeRelaxed(size);
}

void AnimationViewer::play()
{
    Q_D(AnimationViewer);
    DecoderThread::Command cmd(DecoderThread::Command::Play);
    d->thread->commands.put(cmd);
    d->playTime = 0;
    d->nextFrameTimer.start();
}

void AnimationViewer::stop()
{
    Q_D(AnimationViewer);
    DecoderThread::Command cmd(DecoderThread::Command::Stop);
    d->thread->commands.put(cmd);
    d->playTime = 0;
    d->nextFrameTimer.stop();
}

void AnimationViewer::pause()
{
    Q_D(AnimationViewer);
    d->nextFrameTimer.stop();
}

void AnimationViewer::resume()
{
    Q_D(AnimationViewer);
    if (d->thread->context.isNull()) {
        return;
    }
    d->nextFrameTimer.start();
}

void AnimationViewer::setAutoRepeat(bool autoRepeat)
{
    Q_D(AnimationViewer);
    if (d->autoRepeat == autoRepeat) {
        return;
    }
    d->autoRepeat = autoRepeat;
    if (autoRepeat) {
        connect(this, &AnimationViewer::finished, this, &AnimationViewer::play);
    } else {
        disconnect(this, &AnimationViewer::finished, this, &AnimationViewer::play);
    }
}

void AnimationViewer::paintEvent(QPaintEvent *event)
{
    Q_D(AnimationViewer);
    QWidget::paintEvent(event);
    if (d->current.isNull()) {
        return;
    }
    QPainter painter(this);
    painter.drawImage(rect(), d->current);
}

void AnimationViewer::resizeEvent(QResizeEvent *event)
{
    Q_D(AnimationViewer);
    QWidget::resizeEvent(event);
    QSize s = this->size();
    DecoderThread::Command cmd(DecoderThread::Command::Resize);
    cmd.int_arg1 = s.width();
    cmd.int_arg2 = s.height();
    d->thread->commands.put(cmd);
}

void AnimationViewer::hideEvent(QHideEvent *event)
{
    Q_D(AnimationViewer);
    QWidget::hideEvent(event);
    d->pausedBeforeHidden = d->nextFrameTimer.isActive();
    if (!d->pausedBeforeHidden) {
        pause();
    }
}
void AnimationViewer::showEvent(QShowEvent *event)
{
    Q_D(AnimationViewer);
    QWidget::showEvent(event);
    if (!d->pausedBeforeHidden) {
        resume();
    }
}

QList<QImage> convertVideoToImages(const QString &filePath, QString *reason)
{
    QScopedPointer<AVContext> context(makeContext(filePath, reason));
    if (!context) {
        if (reason)
            qCDebug(logger) << *reason;
        return QList<QImage>();
    }

    QList<QImage> frames;
    while (true) {
        QScopedPointer<AVPacket, ScopedPointerAvPacketDeleter> packet(av_packet_alloc());
        if (av_read_frame(context->formatCtx, packet.data())) {
            return frames;
        }
        if (packet->stream_index != context->videoStream) {
            continue;
        }

        int r = avcodec_send_packet(context->codecCtx, packet.data());
        if (r != 0) {
            if (r == AVERROR(EAGAIN)) {
                if (reason) {
                    *reason = QString::fromUtf8("too much packet.");
                }
            } else {
                if (reason) {
                    *reason = QString::fromUtf8("can not send packet.");
                }
            }
            return QList<QImage>();
        }

        while (true) {
            r = avcodec_receive_frame(context->codecCtx, context->nativeFrame);
            if (r == AVERROR(EAGAIN)) {
                break;
            } else if (r != 0) {
                if (reason) {
                    *reason = QString::fromUtf8("can not decode frame.");
                }
                return QList<QImage>();
            } else {
                context->initSwsContext();
                int h = sws_scale(context->swsContext, context->nativeFrame->data, context->nativeFrame->linesize, 0,
                                  context->codecCtx->height, context->rgbFrame->data, context->rgbFrame->linesize);
                if (h <= 0) {
                    if (reason) {
                        *reason = QString::fromUtf8("can not scale frame.");
                    }
                    return QList<QImage>();
                }
                // 理论上复制 nativeFrame 的数据到 rgbFrame，但实际上我们并不使用 rgbFrame，而是过度一下，丢到
                // VideoFrame 里面。 所以我们这里就复制了。
                // rgbFrame->pts = nativeFrame->pts;
                // rgbFrame->color_trc = nativeFrame->color_trc;
                // rgbFrame->key_frame = nativeFrame->key_frame;
                // rgbFrame->pict_type = nativeFrame->pict_type;
                // rgbFrame->color_range = nativeFrame->color_range;
                QImage image(static_cast<const uchar *>(context->rgbFrame->data[0]), context->nativeFrame->width,
                             context->nativeFrame->height, context->rgbFrame->linesize[0],
                             QImage::Format_RGBA8888_Premultiplied);
                if (!image.isNull()) {
                    frames.append(image.copy());
                }
            }
        }
    }
}

#include "animation_viewer.moc"
