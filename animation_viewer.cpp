extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "blocking_queue.h"
#include "image_viewer_p.h"
#include "animation_viewer.h"

enum ParseResult {
    ParseSuccess,
    ParseFailed,
    NotParsed,
};

class AVContext
{
public:
    AVContext();
    ~AVContext();
public:
    AVFormatContext *formatCtx;
    AVCodecContext *codecCtx;
    int videoStream;
    double timeBase;
};

class DecoderThread: public QThread
{
public:
    DecoderThread();
    virtual ~DecoderThread() override;
public:
    virtual void run() override;
public:
    ParseResult parse(const QString &filePath);
    void seek

};

class AnimationViewerPrivate : public ImageViewerPrivate
{
    Q_OBJECT
public:
    AnimationViewerPrivate(AnimationViewer *q);
    virtual ~AnimationViewerPrivate() override;
public:
    bool parseFile();
    bool isPlaying();
    void clearTherad();
    void start();
    void stop();
    void resume();
    void pause();
private slots:
    void updateProgress();
    void emitFinished();
public:
    QScopedPointer<AVContext> context;
    DecoderThread *thread;
    QString filePath;
    QTimer timer;
    qint64 playTime;  // in msecs
    ParseResult parseResult;
    QAtomicInteger<bool> autoRepeat;
private:
    Q_DECLARE_PUBLIC(AnimationViewer)
};

class VideoFrame
{
public:
    VideoFrame(const QImage &image, int64_t pts, int64_t dts)
        : image(image)
        , pts(pts)
        , dts(dts)
    {
    }
    VideoFrame(const VideoFrame &other)
        : image(other.image)
        , pts(other.pts)
        , dts(other.dts)
    {
    }
    VideoFrame()
        : pts(-1)
        , dts(-1)
    {
    }
public:
    bool isValid() const { return pts >= 0 && dts >= 0; }
public:
    QImage image;
    int64_t pts;
    int64_t dts;
};

class DecoderThread : public QThread
{
public:
    DecoderThread(AVContext *context);
    virtual void run() override;
public:
    void stop();
    bool isExiting() const;
public:
    QScopedPointer<AVContext> context;
    BlockingQueue<VideoFrame> queue;
    QAtomicInt exiting;
};

AVContext::AVContext()
    : formatCtx(nullptr)
    , codecCtx(nullptr)
    , videoStream(0)
    , timeBase(0.001)
{
}

AVContext::~AVContext()
{
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
    }
    if (formatCtx) {
        avformat_close_input(&formatCtx);
    }
}

AnimationViewerPrivate::AnimationViewerPrivate(AnimationViewer *q)
    : ImageViewerPrivate(q)
    , thread(nullptr)
    , playTime(0)
    , parseResult(NotParsed)
    , autoRepeat(false)
{
#if LIBAVFORMAT_VERSION_INT <= AV_VERSION_INT(58, 9, 100)
    static QAtomicInt registeredFormats(z0);
    if (!registeredFormats.fetchAndAddRelaxed(1)) {
        av_register_all();
    }
#endif
    timer.setInterval(5);
    timer.setSingleShot(false);
    connect(&timer, SIGNAL(timeout()), SLOT(updateProgress()));
}

AnimationViewerPrivate::~AnimationViewerPrivate()
{
    stop();
}

bool AnimationViewerPrivate::parseFile()
{
    if (filePath.isEmpty()) {
        return false;
    }
    if (parseResult == ParseSuccess) {
        return true;
    } else if (parseResult == ParseFailed) {
        return false;
    }

    QScopedPointer<AVContext> tmpAvContext(new AVContext());
    if (int ret = avformat_open_input(&tmpAvContext->formatCtx, qPrintable(filePath), nullptr, nullptr)) {
        qDebug() << "can not open file." << ret;
        parseResult = ParseFailed;
        tmpAvContext->formatCtx = nullptr;
        return false;
    }

    tmpAvContext->videoStream = av_find_best_stream(tmpAvContext->formatCtx, AVMEDIA_TYPE_VIDEO, 0, 0, nullptr, 0);
    if (tmpAvContext->videoStream < 0) {
        parseResult = ParseFailed;
        return false;
    }

    AVStream *stream = tmpAvContext->formatCtx->streams[tmpAvContext->videoStream];
    AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        qDebug() << "can not find codec.";
        parseResult = ParseFailed;
        return false;
    }
    tmpAvContext->codecCtx = avcodec_alloc_context3(codec);
    if (!tmpAvContext->codecCtx) {
        qDebug() << "can not allocate codec context.";
        parseResult = ParseFailed;
        return false;
    }
    if (avcodec_parameters_to_context(tmpAvContext->codecCtx, stream->codecpar) < 0) {
        qDebug() << "can not set parameters to codec context.";
        parseResult = ParseFailed;
        return false;
    }
    if (avcodec_open2(tmpAvContext->codecCtx, nullptr, nullptr)) {
        qDebug() << "can not open codec context.";
        parseResult = ParseFailed;
        return false;
    }
    tmpAvContext->timeBase = av_q2d(stream->time_base);
    parseResult = ParseSuccess;
    qSwap(this->context, tmpAvContext);
    return true;
}

bool AnimationViewerPrivate::isPlaying()
{
    return thread && !thread->isExiting() && thread->isRunning();
}

void AnimationViewerPrivate::updateProgress()
{
    Q_Q(AnimationViewer);
    if (isPlaying()) {
        playTime += timer.interval();
        // thread 不从属于 AnimationViewerPrivate，太容易在删除后访问搞出野指针了。
        const VideoFrame &vf = thread->queue.peek();
        if (vf.isValid() && vf.pts <= playTime) {
            thread->queue.get();
            q->ImageViewer::setImage(vf.image);
        }
    } else {
        timer.stop();
        emitFinished();
    }
}

void AnimationViewerPrivate::start()
{
    if (thread || playTime || timer.isActive()) {
        qWarning("start() must called in stop state.");
        return;
    }
    if (context.isNull()) {
        qWarning("the file is not parsed, can not start.");
        return;
    }
    Q_ASSERT(parseResult == ParseSuccess);
    Q_ASSERT(!thread);
    thread = new DecoderThread(context.take());
    parseResult = NotParsed;
    thread->start();
    playTime = 0;
    timer.start();
}

void AnimationViewerPrivate::stop()
{
    if (thread) {
        thread->stop();
        thread->wait();
        delete thread;
        thread = nullptr;
        playTime = 0;
        timer.stop();
    } else {
        Q_ASSERT(playTime == 0);
        Q_ASSERT(!timer.isActive());
    }
}

void AnimationViewerPrivate::resume()
{
    if (!isPlaying()) {
        return;
    }
    timer.start();
}

void AnimationViewerPrivate::pause()
{
    if (!isPlaying()) {
        return;
    }
    timer.stop();
}

void AnimationViewerPrivate::emitFinished()
{
    Q_Q(AnimationViewer);
    stop();
    emit q->finished();
}

DecoderThread::DecoderThread(AVContext *context)
    : context(context)
    , queue(5)
    , exiting(false)
{
    Q_ASSERT(context != nullptr);
    // connect(this, &QThread::finished, this, &QObject::deleteLater);
}

struct ScopedPointerNativeFrameDeleter
{
    static inline void cleanup(AVFrame *&pointer) { av_frame_free(&pointer); }
};

struct ScopedPointerRgbFrameDeleter
{
    static inline void cleanup(AVFrame *&pointer)
    {
        if (pointer->data[0]) {
            av_freep(&pointer->data[0]);
        }
        av_frame_free(&pointer);
    }
};

struct ScopedPointerAvPacketDeleter
{
    static inline void cleanup(AVPacket *&pointer) { av_packet_free(&pointer); }
};

struct ScopedPointerSwsContextDeleter
{
    static inline void cleanup(SwsContext *pointer) { sws_freeContext(pointer); }
};

void DecoderThread::run()
{
    if (isExiting())
        return;

    if (!context || !context->formatCtx || !context->codecCtx) {
        return;
    }

    QScopedPointer<AVFrame, ScopedPointerNativeFrameDeleter> nativeFrame(av_frame_alloc());
    QScopedPointer<AVFrame, ScopedPointerRgbFrameDeleter> rgbFrame(av_frame_alloc());
    if (av_image_alloc(rgbFrame->data, rgbFrame->linesize, context->codecCtx->width, context->codecCtx->height,
                       AV_PIX_FMT_RGBA, 32)
        < 0) {
        qDebug() << "can not allocate frame buffer.";
        return;
    }
    rgbFrame->width = context->codecCtx->width;
    rgbFrame->height = context->codecCtx->height;
    rgbFrame->format = AV_PIX_FMT_RGBA;

    QScopedPointer<SwsContext, ScopedPointerSwsContextDeleter> swsCtx;

    while (true) {
        if (isExiting())
            return;

        QScopedPointer<AVPacket, ScopedPointerAvPacketDeleter> packet(av_packet_alloc());
        if (av_read_frame(context->formatCtx, packet.data())) {
            return;
        }

        if (isExiting())
            return;
        if (packet->stream_index != context->videoStream) {
            continue;
        }

        if (isExiting())
            return;

        Q_ASSERT(context->codecCtx);
        int r = avcodec_send_packet(context->codecCtx, packet.data());
        if (r != 0) {
            if (r == AVERROR(EAGAIN)) {
                qDebug() << "too much packet.";
            } else {
                qDebug() << "can not send packet.";
            }
            return;
        }

        while (true) {
            if (isExiting())
                return;
            Q_ASSERT(context->codecCtx);
            r = avcodec_receive_frame(context->codecCtx, nativeFrame.data());
            if (r == AVERROR(EAGAIN)) {
                break;
            } else if (r != 0) {
                qDebug() << "can not decode frame.";
                return;
            } else {
                AVFrame *t;
                if (context->codecCtx->pix_fmt == AV_PIX_FMT_RGBA) {
                    t = nativeFrame.data();
                } else {
                    if (!swsCtx) {
                        AVPixelFormat orignalFormat;
                        switch (context->codecCtx->pix_fmt) {
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
                            orignalFormat = context->codecCtx->pix_fmt;
                            break;
                        }
                        if (isExiting())
                            return;
                        swsCtx.reset(sws_getContext(context->codecCtx->width, context->codecCtx->height, orignalFormat,
                                                    context->codecCtx->width, context->codecCtx->height,
                                                    AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr));
                        if (!swsCtx) {
                            qDebug() << "can not allocate sws context.";
                            return;
                        }
                    }

                    if (isExiting())
                        return;
                    int h = sws_scale(swsCtx.data(), nativeFrame->data, nativeFrame->linesize, 0,
                                      context->codecCtx->height, rgbFrame->data, rgbFrame->linesize);
                    if (h <= 0) {
                        qDebug() << "can not scale frame.";
                        return;
                    }
                    //                rgbFrame->pts = nativeFrame->pts;
                    //                rgbFrame->color_trc = nativeFrame->color_trc;
                    //                rgbFrame->key_frame = nativeFrame->key_frame;
                    //                rgbFrame->pict_type = nativeFrame->pict_type;
                    //                rgbFrame->color_range = nativeFrame->color_range;
                    t = rgbFrame.data();
                }
                QImage image(reinterpret_cast<const uchar *>(t->data[0]), t->width, t->height, t->linesize[0],
                             QImage::Format_RGBA8888_Premultiplied);

                int64_t pts = static_cast<int64_t>(nativeFrame->pts * context->timeBase * 1000.0);
                VideoFrame vf(image.copy(), pts, nativeFrame->pkt_dts);
                if (isExiting())
                    return;
                queue.put(vf);
            }
        }
    }
}

void DecoderThread::stop()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    exiting.storeRelaxed(true);
#else
    exiting.store(true);
#endif
    queue.clear();
}

bool DecoderThread::isExiting() const
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    return exiting.loadAcquire();
#else
    return exiting.load();
#endif
}

AnimationViewer::AnimationViewer(QWidget *parent)
    : ImageViewer(parent, new AnimationViewerPrivate(this))
{
}

AnimationViewer::~AnimationViewer() { }

bool AnimationViewer::play()
{
    Q_D(AnimationViewer);
    if (d->isPlaying()) {
        d->timer.start();
        return true;
    }
    Q_ASSERT(!d->thread);
    if (!d->parseFile()) {
        return false;
    }
    d->start();
    return true;
}

void AnimationViewer::stop()
{
    Q_D(AnimationViewer);
    d->stop();
}

void AnimationViewer::pause()
{
    Q_D(AnimationViewer);
    d->pause();
}

void AnimationViewer::setAutoRepeat(bool autoRepeat)
{
    Q_D(AnimationViewer);
    d->autoRepeat = autoRepeat;
    if (autoRepeat) {
        connect(this, SIGNAL(finished()), SLOT(play()), Qt::QueuedConnection);
    } else {
        disconnect(this, SIGNAL(finished()), this, SLOT(play()));
    }
}

bool AnimationViewer::setFile(const QString &filePath)
{
    Q_D(AnimationViewer);
    d->stop();
    QImageReader reader(filePath);
    if (reader.canRead() && !reader.supportsAnimation()) {
        // static image.
        return ImageViewer::setFile(filePath);
    } else {
        if (d->filePath == filePath) {
            if (d->parseResult == ParseSuccess) {
                return true;
            } else if (d->parseResult == ParseFailed) {
                return false;
            }
        } else {
            d->filePath = filePath;
            d->parseResult = NotParsed;
        }
        if (!d->parseFile()) {
            return false;
        }
        d->start();
        if (!isVisible()) {
            d->pause();
        }
        return true;
    }
}

QString AnimationViewer::imagePath()
{
    Q_D(AnimationViewer);
    if (d->filePath.isEmpty()) {
        return ImageViewer::imagePath();
    } else {
        return d->filePath;
    }
}

void AnimationViewer::hideEvent(QHideEvent *event)
{
    Q_D(AnimationViewer);
    ImageViewer::hideEvent(event);
    d->pause();
}

void AnimationViewer::showEvent(QShowEvent *event)
{
    Q_D(AnimationViewer);
    ImageViewer::showEvent(event);
    d->resume();
}

struct ScopedPointerAVFormatContextDeleter
{
    static inline void cleanup(AVFormatContext *&pointer) { avformat_close_input(&pointer); }
};

struct ScopedPointerAVCodecContextDeleter
{
    static inline void cleanup(AVCodecContext *&pointer) { avcodec_free_context(&pointer); }
};

QList<QImage> convertVideoToImages(const QString &filePath, QString *reason)
{
    AVFormatContext *tContext = nullptr;
    if (avformat_open_input(&tContext, qPrintable(filePath), nullptr, nullptr)) {
        if (reason) {
            *reason = QString::fromUtf8("can not open video file: %1").arg(filePath);
        }
        return QList<QImage>();
    }
    QScopedPointer<AVFormatContext, ScopedPointerAVFormatContextDeleter> context(tContext);

    int videoStream = av_find_best_stream(context.data(), AVMEDIA_TYPE_VIDEO, 0, 0, nullptr, 0);
    if (videoStream < 0) {
        if (reason) {
            *reason = QString::fromUtf8("no video stream found.");
        }
        return QList<QImage>();
    }

    AVStream *stream = context->streams[videoStream];
    AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        if (reason) {
            *reason = QString::fromUtf8("can not find codec.");
        }
        return QList<QImage>();
    }

    QScopedPointer<AVCodecContext, ScopedPointerAVCodecContextDeleter> codecCtx(avcodec_alloc_context3(codec));
    if (!codecCtx) {
        if (reason) {
            *reason = QString::fromUtf8("can not allocate codec context.");
        }
        return QList<QImage>();
    }
    if (avcodec_parameters_to_context(codecCtx.data(), stream->codecpar) < 0) {
        if (reason) {
            *reason = QString::fromUtf8("can not set parameters to codec context.");
        }
        return QList<QImage>();
    }
    if (avcodec_open2(codecCtx.data(), nullptr, nullptr)) {
        if (reason) {
            *reason = QString::fromUtf8("can not open codec context.");
        }
        return QList<QImage>();
    }

    QScopedPointer<AVFrame, ScopedPointerNativeFrameDeleter> nativeFrame(av_frame_alloc());
    QScopedPointer<AVFrame, ScopedPointerRgbFrameDeleter> rgbFrame(av_frame_alloc());
    if (av_image_alloc(rgbFrame->data, rgbFrame->linesize, codecCtx->width, codecCtx->height, AV_PIX_FMT_RGBA, 32)
        < 0) {
        if (reason) {
            *reason = QString::fromUtf8("can not allocate frame buffer.");
        }
        return QList<QImage>();
    }
    rgbFrame->width = codecCtx->width;
    rgbFrame->height = codecCtx->height;
    rgbFrame->format = AV_PIX_FMT_RGBA;

    QScopedPointer<SwsContext, ScopedPointerSwsContextDeleter> swsCtx;

    QList<QImage> frames;
    while (true) {
        QScopedPointer<AVPacket, ScopedPointerAvPacketDeleter> packet(av_packet_alloc());
        if (av_read_frame(context.data(), packet.data())) {
            return frames;
        }
        if (packet->stream_index != videoStream) {
            continue;
        }

        int r = avcodec_send_packet(codecCtx.data(), packet.data());
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
            r = avcodec_receive_frame(codecCtx.data(), nativeFrame.data());
            if (r == AVERROR(EAGAIN)) {
                break;
            } else if (r != 0) {
                if (reason) {
                    *reason = QString::fromUtf8("can not decode frame.");
                }
                return QList<QImage>();
            } else {
                if (!swsCtx) {
                    AVPixelFormat orignalFormat;
                    switch (codecCtx->pix_fmt) {
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
                    swsCtx.reset(sws_getContext(codecCtx->width, codecCtx->height, orignalFormat, codecCtx->width,
                                                codecCtx->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr,
                                                nullptr));
                    if (!swsCtx) {
                        if (reason) {
                            *reason = QString::fromUtf8("can not create software scale context.");
                        }
                        return QList<QImage>();
                    }
                }
                int h = sws_scale(swsCtx.data(), nativeFrame->data, nativeFrame->linesize, 0, codecCtx->height,
                                  rgbFrame->data, rgbFrame->linesize);
                if (h <= 0) {
                    if (reason) {
                        *reason = QString::fromUtf8("can not scale frame.");
                    }
                    return QList<QImage>();
                }
                QImage image(static_cast<const uchar *>(rgbFrame->data[0]), nativeFrame->width, nativeFrame->height,
                             rgbFrame->linesize[0], QImage::Format_RGBA8888_Premultiplied);
                if (!image.isNull()) {
                    frames.append(image.copy());
                }
            }
        }
    }
}

#include "animation_viewer.moc"
