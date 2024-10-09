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
    QScopedPointer<AVContext> tmpAvContext(new AVContext());
    if (int ret = avformat_open_input(&tmpAvContext->formatCtx, qPrintable(url), nullptr, nullptr)) {
        if (reason) {
            *reason = QString::fromUtf8("can not open file: %1").arg(ret);
        }
        tmpAvContext->formatCtx = nullptr;
        return nullptr;
    }

    tmpAvContext->videoStream = av_find_best_stream(tmpAvContext->formatCtx, AVMEDIA_TYPE_VIDEO, 0, 0, nullptr, 0);
    if (tmpAvContext->videoStream < 0) {
        if (reason) {
            *reason = QString::fromUtf8("can not find video stream.");
        }
        return nullptr;
    }

    AVStream *stream = tmpAvContext->formatCtx->streams[tmpAvContext->videoStream];
    AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        if (reason) {
            *reason = QString::fromUtf8("can not find codec.");
        }
        return nullptr;
    }
    tmpAvContext->codecCtx = avcodec_alloc_context3(codec);
    if (!tmpAvContext->codecCtx) {
        if (reason) {
            *reason = QString::fromUtf8("can not allocate codec context.");
        }
        return nullptr;
    }
    if (avcodec_parameters_to_context(tmpAvContext->codecCtx, stream->codecpar) < 0) {
        if (reason) {
            *reason = QString::fromUtf8("can not set parameters to codec context.");
        }
        return nullptr;
    }
    if (avcodec_open2(tmpAvContext->codecCtx, nullptr, nullptr)) {
        if (reason) {
            *reason = QString::fromUtf8("can not open codec context.");
        }
        return nullptr;
    }
    tmpAvContext->nativeFrame = av_frame_alloc();
    AVFrame *rgbFrame = av_frame_alloc();
    tmpAvContext->rgbFrame = rgbFrame;
    if (av_image_alloc(rgbFrame->data, rgbFrame->linesize, tmpAvContext->codecCtx->width,
                       tmpAvContext->codecCtx->height, AV_PIX_FMT_RGBA, 32)
        < 0) {
        if (reason) {
            *reason = QString::fromUtf8("can not allocate rgb frame buffer.");
        }
        return nullptr;
    }
    tmpAvContext->width = rgbFrame->width = tmpAvContext->codecCtx->width;
    tmpAvContext->height = rgbFrame->height = tmpAvContext->codecCtx->height;
    rgbFrame->format = AV_PIX_FMT_RGBA;
    tmpAvContext->timeBase = av_q2d(stream->time_base);
    return tmpAvContext.take();
}

DecoderThread::DecoderThread(QObject *viewerPrivate)
    : viewerPrivate(viewerPrivate)
    , state(AnimationViewer::NotParsed)
    , frameBufferSize(30 * 10)
    , autoRepeat(false)
    , exiting(false)
    , paused(false)
{
}

void DecoderThread::run()
{
    while (!isExiting()) {
        const Command &cmd = commands.get();
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
            if (state != AnimationViewer::ParseSuccess || paused) {
                break;
            }
            switch (play()) {
            case Finished:
                Q_FALLTHROUGH();
            case Ready:
                if (!viewerPrivate.isNull() && !frames.isEmpty())
                    QMetaObject::invokeMethod(viewerPrivate.data(), "next");
                break;
            case Exit:
                return;
            case Error:
                stop();
                break;
            }
            break;
        case Command::Stop:
            stop();
            break;
        case Command::Pause:
            paused = true;
            break;
        case Command::Resume:
            paused = false;
            commands.put(Command(Command::Play));
            break;
        case Command::Seek:
            seek(cmd.int_arg1);
            break;
        case Command::Resize:
            context->width = static_cast<int>(cmd.int_arg1);
            context->height = static_cast<int>(cmd.int_arg2);
            break;
        default:
            qCWarning(logger) << "unknown command type:";
        }
    }
}

bool DecoderThread::parse(const QString &url)
{
    Q_ASSERT(!url.isEmpty() && state == AnimationViewer::NotParsed);
    QString reason;
    AVContext *context = makeContext(url, &reason);
    if (!context) {
        qCDebug(logger) << reason;
        return false;
    } else {
        this->context.reset(context);
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
        if (frames.size() >= frameBufferSize) {
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
            if (frames.size() >= frameBufferSize) {
                return PlayResult::Ready;
            }

            r = avcodec_receive_frame(context->codecCtx, context->nativeFrame);
            if (r == AVERROR(EAGAIN)) {
                break;
            } else if (r != 0) {
                qCDebug(logger) << "can not decode frame.";
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
                        qCDebug(logger) << "can not scale frame.";
                        return PlayResult::Error;
                    }

                    t = context->rgbFrame;
                }
                QImage image(reinterpret_cast<const uchar *>(t->data[0]), t->width, t->height, t->linesize[0],
                             QImage::Format_RGBA8888_Premultiplied);

                int64_t pts = static_cast<int64_t>(context->nativeFrame->pts * context->timeBase * 1000.0);
                VideoFrame vf(image.copy(), pts, context->nativeFrame->pkt_dts);
                if (isExiting()) {
                    return PlayResult::Exit;
                }
                frames.put(vf);
            }
        }
    }
}

void DecoderThread::stop()
{
    state = AnimationViewer::NotParsed;
    context.reset();
    paused = false;
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
    , pausedBeforeHidden(false)
{
#if LIBAVFORMAT_VERSION_INT <= AV_VERSION_INT(58, 9, 100)
    static QAtomicInt registeredFormats(z0);
    if (!registeredFormats.fetchAndAddRelaxed(1)) {
        av_register_all();
    }
#endif
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
    timer.setSingleShot(true);
    connect(&timer, SLOT(timeout()), this, SLOT(next()));
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
    VideoFrame f = thread->frames.peek();
    if (!f.isValid()) {
        return;
    }
    if (f.pts < playTime) {
        timer.setInterval(f.pts - playTime);
        timer.start();
    } else {
        thread->frames.get();
        current = f.image;
        VideoFrame n = thread->frames.peek();
        if (n.isValid()) {
            timer.setInterval(n.pts - playTime);
            timer.start();
        }
        q->update();
    }
}

void AnimationViewerPrivate::finished()
{
    Q_Q(AnimationViewer);
    emit q->finished();
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

bool AnimationViewer::setUrl(const QString &url)
{
    Q_D(AnimationViewer);
    d->mediaUrl = url;
    DecoderThread::Command cmd(DecoderThread::Command::Parse);
    cmd.str_arg = url;
    d->thread->commands.put(cmd);
}

bool AnimationViewer::play()
{
    Q_D(AnimationViewer);
    DecoderThread::Command cmd(DecoderThread::Command::Play);
    d->thread->commands.put(cmd);
}

void AnimationViewer::stop()
{
    Q_D(AnimationViewer);
    DecoderThread::Command cmd(DecoderThread::Command::Stop);
    d->thread->commands.put(cmd);
}

void AnimationViewer::pause()
{
    Q_D(AnimationViewer);
    DecoderThread::Command cmd(DecoderThread::Command::Pause);
    d->thread->commands.put(cmd);
}

void AnimationViewer::resume()
{
    Q_D(AnimationViewer);
    DecoderThread::Command cmd(DecoderThread::Command::Resume);
    d->thread->commands.put(cmd);
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
    QPainter painter(this);
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
    d->pausedBeforeHidden = d->thread->paused;
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
