#include <QtCore/qthread.h>
#include <QtCore/qdebug.h>
#include <QtCore/qatomic.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qpointer.h>
#include <QtCore/qthreadpool.h>
#include <QtGui/qpainter.h>
#include <QtGui/qevent.h>
#include <QtGui/qimagereader.h>
#include <QtWidgets/qapplication.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include "blocking_queue.h"
#include "lafplay.h"


class ImageViewerPrivate: public QObject
{
    Q_OBJECT
public:
    ImageViewerPrivate(ImageViewer *q);
    void dropCache();
    void rebuildCached();
public slots:
    void prepareBuildingCache();
    void cacheBuilt(const QPixmap &pixmap, const QSize &targetSize, const QPoint &tagetPosition, double targetRatio);
public:
    QImage image;
    QString imagePath;
    QImage loadImage();

    QPixmap cached;
    QSize cachedSize;  // cachedSize != cached.size()
    ImageViewer::Mode mode;
    QPoint pos;               // the position to draw cache.

    // for drag support
    QPoint startDragPos;
    QPoint originalPos;

    Qt::TransformationMode transformationMode;
    double ratio;
    QAtomicInt building;
protected:
    ImageViewer * const q_ptr;
private:
    Q_DECLARE_PUBLIC(ImageViewer)
};


ImageViewerPrivate::ImageViewerPrivate(ImageViewer *q)
    : mode(ImageViewer::OriginalSize)
    , pos(0, 0)
    , transformationMode(Qt::FastTransformation)
    , ratio(1.0)
    , building(false)
    , q_ptr(q)
{

}


void ImageViewerPrivate::dropCache()
{
    cached = QPixmap();
}


void ImageViewerPrivate::rebuildCached()
{
    Q_Q(ImageViewer);
    if (q->isVisible() && !building) {
        QTimer::singleShot(0, this, SLOT(prepareBuildingCache()));
    }
}


QImage ImageViewerPrivate::loadImage()
{
    if (image.isNull()) {
        if (imagePath.isEmpty()) {
            return QImage();
        }
        return QImage(imagePath);
    } else {
        return image;
    }
}


class BuildCache: public QRunnable
{
public:
    BuildCache(QPointer<ImageViewerPrivate> p, Qt::TransformationMode tranMode, ImageViewer::Mode mode,
               double raite, const QImage &image, const QString &imagePath, const QSize &targetRect,
               const QPoint &targetPosition, double devicePixelRatioF);
    virtual ~BuildCache() override;
    virtual void run() override;
    inline QImage loadImage();
public:
    QPointer<ImageViewerPrivate> p;
    const Qt::TransformationMode tranMode;
    const ImageViewer::Mode mode;
    const double ratio;
    const QImage image;
    const QString imagePath;
    const QSize targetSize;
    const QPoint targetPosition;
    const double devicePixelRatioF;
};


BuildCache::BuildCache(QPointer<ImageViewerPrivate> p, Qt::TransformationMode tranMode, ImageViewer::Mode mode,
                       double ratio, const QImage &image, const QString &imagePath, const QSize &targetSize,
                       const QPoint &targetPosition, double devicePixelRatioF)
    : p(p)
    , tranMode(tranMode)
    , mode(mode)
    , ratio(ratio)
    , image(image)
    , imagePath(imagePath)
    , targetSize(targetSize)
    , targetPosition(targetPosition)
    , devicePixelRatioF(devicePixelRatioF)
{}


BuildCache::~BuildCache() {}


void BuildCache::run()
{
    if (p.isNull())
        return;
    QImage result;
    double targetRatio = 1.0;

    if(mode == ImageViewer::OriginalSize || (mode == ImageViewer::CustomRatio && qFuzzyCompare(ratio, 1.0))) {
        result = loadImage();
    } else if(mode == ImageViewer::FitHeight) {
        const QImage &img = loadImage();
        if (!img.isNull()) {
            result = img.scaledToHeight(static_cast<int>(targetSize.height() * devicePixelRatioF), tranMode);
            targetRatio = result.height() / img.height();
        }
    } else if(mode == ImageViewer::FitWidth) {
        const QImage &img = loadImage();
        if (!img.isNull()) {
            result = img.scaledToWidth(static_cast<int>(targetSize.width() * devicePixelRatioF), tranMode);
            targetRatio = result.width() / img.width();
        }
    } else if(mode == ImageViewer::CustomRatio) {
        const QImage &img = loadImage();
        if (!img.isNull()) {
            result = img.scaled(targetSize * ratio * devicePixelRatioF, Qt::KeepAspectRatio, tranMode);
            targetRatio = ratio;
        }
    } else { // defautl to fitwindow
        const QImage &img = loadImage();
        if (!img.isNull()) {
            result = img.scaled(targetSize * devicePixelRatioF, Qt::KeepAspectRatio, tranMode);
            targetRatio = result.width() / img.width();
        }
    }
    if (p.isNull()) {
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    p->cacheBuilt(QPixmap::fromImage(result), targetSize, targetPosition, targetRatio);
#else
    QMetaObject::invokeMethod(p.data(), "cacheBuilt", Qt::QueuedConnection,
                              Q_ARG(QPixmap, QPixmap::fromImage(result)), Q_ARG(QSize, targetSize),
                              Q_ARG(QPoint, targetPosition), Q_ARG(double, targetRatio));
#endif
}


QImage BuildCache::loadImage()
{
    if (image.isNull()) {
        if (imagePath.isEmpty()) {
            return QImage();
        }
        return QImage(imagePath);
    } else {
        return image;
    }
}



void ImageViewerPrivate::prepareBuildingCache()
{
    Q_Q(ImageViewer);
    if (!q->hasImage() || building.loadAcquire()) {
        return;
    }
    building.storeRelease(true);
    BuildCache *task = new BuildCache(this, transformationMode, mode, ratio, image, imagePath,
                                      q->size(), pos, q->devicePixelRatioF());
    task->setAutoDelete(true);
    task->run();
    QThreadPool::globalInstance()->start(task);
}


void ImageViewerPrivate::cacheBuilt(const QPixmap &result, const QSize &targetSize, const QPoint &, double targetRatio)
{
    Q_Q(ImageViewer);
    building.storeRelease(false);
    if (targetSize != q->size()) {  // maybe size is changed while generating.
        rebuildCached();
    } else {
        cached = result;
        cachedSize = targetSize;
        ratio = targetRatio;
        q->update();
    }
}


ImageViewer::ImageViewer(QWidget *parent)
    : QWidget(parent), dd_ptr(new ImageViewerPrivate(this))

{
}


ImageViewer::ImageViewer(QWidget *parent, ImageViewerPrivate *d)
    : QWidget(parent), dd_ptr(d)
{
}



ImageViewer::~ImageViewer()
{
    delete dd_ptr;
}


void ImageViewer::clear()
{
    Q_D(ImageViewer);
    d->image = QImage();
    d->imagePath.clear();
    d->dropCache();
}


bool ImageViewer::setImage(const QImage &image)
{
    Q_D(ImageViewer);
    if (image.isNull()) {
        return false;
    }
    d->image = image;
    if (isVisible()) {
        d->rebuildCached();
    } else {
        d->dropCache();
    }
    return true;
}


bool ImageViewer::setFile(const QString &imagePath)
{
    Q_D(ImageViewer);
    if (imagePath.isEmpty()) {
        return false;
    }
    d->image = QImage();
    d->imagePath = imagePath;
    // will not read image in GUI thread.
    // NOT HERE: d->image = QImage(imagePath);
    if (isVisible()) {
        d->rebuildCached();
    } else {
        d->dropCache();
    }
    return true;
}


void ImageViewer::resizeEvent(QResizeEvent *event)
{
    Q_D(ImageViewer);
    QWidget::resizeEvent(event);
    d->rebuildCached();
}


void ImageViewer::paintEvent(QPaintEvent *event)
{
    Q_D(ImageViewer);
    if (!hasImage()) {
        QWidget::paintEvent(event);
        return;
    }
    if (d->cached.isNull() || d->cachedSize != this->size()) {
        d->rebuildCached();
        return;
    }

    QPainter painter(this);
    double dpr = this->devicePixelRatioF();
    QRect source = d->cached.rect();
    QRect r = rect();
    QRect viewport(0, 0, static_cast<int>(source.width() / dpr), static_cast<int>(source.height() / dpr));
    if (viewport.width() > r.width()) {
        int delta = viewport.width() - r.width();
        d->pos.setX(qMin(delta, qMax(-delta, d->pos.x())));
        viewport.setWidth(r.width());
        source.setWidth(static_cast<int>(r.width() * dpr));
        source.moveLeft(static_cast<int>(-d->pos.x() * dpr));
    }
    if (viewport.height() > r.height()) {
        int delta = viewport.height() - r.height();
        d->pos.setY(qMin(delta, qMax(-delta, d->pos.y())));
        viewport.setHeight(r.height());
        source.setHeight(static_cast<int>(r.height() * dpr));
        source.moveTop(static_cast<int>(-d->pos.y() * dpr));
    }
    viewport.moveCenter(rect().center());
    painter.drawPixmap(viewport, d->cached, source);

//    QRect ir = d->cached.rect();
//    ir.setSize(ir.size() / dpr);
//    ir.moveCenter(rect().center());

//    QRect invalidated = event->rect().intersected(ir);
//    QRect source = invalidated;
//    source.adjust(-d->pos.x(), -d->pos.y(), -d->pos.x(), -d->pos.y());
//    source.moveTopLeft(source.topLeft() * dpr);
//    source.setSize(source.size() * dpr);
//    painter.drawPixmap(invalidated, d->cached, source);
}


void ImageViewer::mousePressEvent(QMouseEvent *event)
{
    Q_D(ImageViewer);
    if (event->button() == Qt::LeftButton) {
        setMouseTracking(true);
        d->startDragPos = event->pos();
        d->originalPos = d->pos;
    }
    QWidget::mousePressEvent(event);
}


void ImageViewer::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(ImageViewer);
    if (event->button() == Qt::LeftButton) {
        int distance = static_cast<QApplication*>(QApplication::instance())->startDragDistance();
        if ((event->pos() - d->startDragPos).manhattanLength() < distance) {
            emit clicked(event->modifiers());
        }
        setMouseTracking(false);
    } else if (event->button() == Qt::RightButton) {
        emit showContextMenu();
    }
    QWidget::mouseReleaseEvent(event);
}


void ImageViewer::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(ImageViewer);
    if (!hasImage()) {
        return;
    }
    if ((event->pos() - d->startDragPos).manhattanLength() > static_cast<QApplication*>(QApplication::instance())->startDragDistance()) {
        QPoint oldPos = d->pos;
        d->pos = d->originalPos + (event->pos() - d->startDragPos);
        QRect r = d->cached.rect();
        double dpr = this->devicePixelRatioF();
        r.setSize(r.size() / dpr);
        r.moveBottomRight(rect().bottomRight());
        int leftMost = qMin(r.left(), 0);
        int topMost = qMin(r.top(), 0);
        int rightMost = qMax(r.left(), 0);
        int bottomMost = qMax(r.top(), 0);
        if(d->pos.x() < leftMost) {
            d->pos.setX(leftMost);
        }
        if(d->pos.x() > rightMost) {
            d->pos.setX(rightMost);
        }
        if(d->pos.y() < topMost) {
            d->pos.setY(topMost);
        }
        if(d->pos.y() > bottomMost) {
            d->pos.setY(bottomMost);
        }
        if(d->pos != oldPos) {
            update();
        }
    }
}


void ImageViewer::hideEvent(QHideEvent *event)
{
    Q_D(ImageViewer);
    QWidget::hideEvent(event);
    d->dropCache();
}


//void ImageViewer::wheelEvent(QWheelEvent *event)
//{
//    Q_D(ImageViewer);
//    if (d->mode == CustomRatio) {
//        QPoint numPixels = event->pixelDelta();
//        QPoint numDegrees = event->angleDelta() / 8;

//        if (!numPixels.isNull()) {
//            setMode(CustomRatio, d->ratio / pow(2.0, - numPixels.y() / 32));
//        } else if (!numDegrees.isNull()) {
//            QPoint numSteps = numDegrees / 15;
//            setMode(CustomRatio, d->ratio / pow(2.0, - numSteps.y()));
//        }

//        event->accept();
//    }
//}


bool ImageViewer::hasImage()
{
    Q_D(ImageViewer);
    return !d->image.isNull() || !d->imagePath.isEmpty();;
}


QImage ImageViewer::image()
{
    Q_D(ImageViewer);
    return d->loadImage();
}


QString ImageViewer::imagePath()
{
    Q_D(ImageViewer);
    return d->imagePath;
}


QSize ImageViewer::imageSize()
{
    Q_D(ImageViewer);
    return d->loadImage().size();
}


void ImageViewer::setMode(Mode mode, double ratio)
{
    Q_D(ImageViewer);
    if (ratio < 0) {
        qWarning() << "setMode() got invalid ratio:" << ratio;
        return;
    }
    bool changed = d->mode != mode;
    if (!changed && mode == CustomRatio && !qFuzzyCompare(d->ratio, ratio)) {
        changed = true;
    }
    d->mode = mode;
    if (mode == OriginalSize) {
        d->ratio = 1.0;
    } else {
        if (mode == CustomRatio) {
            if (ratio < 0.25) {
                ratio = 0.25;
            } else if (ratio > 4.0) {
                ratio = 4.0;
            }
            d->ratio = ratio;
        }
    }
    if (changed) {
        d->rebuildCached();
    }
}


ImageViewer::Mode ImageViewer::mode() const
{
    Q_D(const ImageViewer);
    return d->mode;
}


double ImageViewer::ratio() const
{
    Q_D(const ImageViewer);
    return d->ratio;
}


Qt::TransformationMode ImageViewer::transformationMode() const
{
    Q_D(const ImageViewer);
    return d->transformationMode;
}


void ImageViewer::setTransformationMode(Qt::TransformationMode transformationMode)
{
    Q_D(ImageViewer);
    d->transformationMode = transformationMode;
    d->rebuildCached();
}


enum ParseResult
{
    ParseSuccess,
    ParseFailed,
    NotParsed,
};


class DecoderThread;
class AnimationViewerPrivate: public ImageViewerPrivate
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
    QScopedPointer<DecoderThread> thread;
    AVFormatContext *context;
    AVCodecContext *codecCtx;
    QString filePath;
    QTimer timer;
    qint64 playTime;  // in msecs
    double timeBase;
    int videoStream;
    ParseResult parseResult;
    QAtomicInteger<bool> autoRepeat;
private:
    Q_DECLARE_PUBLIC(AnimationViewer)
};


class VideoFrame
{
public:
    VideoFrame(const QImage &image, int64_t pts, int64_t dts)
        :image(image), pts(pts), dts(dts) {}
    VideoFrame(const VideoFrame &other)
        :image(other.image), pts(other.pts), dts(other.dts) {}
    VideoFrame()
        :pts(0), dts(0) {}
public:
    QImage image;
    int64_t pts;
    int64_t dts;
};


class DecoderThread: public QThread
{
public:
    DecoderThread(AnimationViewerPrivate *parent);
    virtual void run() override;
public:
    void stop();
public:
    BlockingQueue<VideoFrame> queue;
    AnimationViewerPrivate * const parent;
    QAtomicInt exiting;
};


AnimationViewerPrivate::AnimationViewerPrivate(AnimationViewer *q)
    : ImageViewerPrivate (q)
    , thread(nullptr)
    , context(nullptr)
    , codecCtx(nullptr)
    , playTime(0)
    , timeBase(0.001)
    , parseResult(NotParsed)
    , autoRepeat(false)
{
#if LIBAVFORMAT_VERSION_INT <= AV_VERSION_INT(58, 9, 100)
    static QAtomicInt registeredFormats(0);
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
    if (int ret = avformat_open_input(&context, qPrintable(filePath), nullptr, nullptr)) {
        qDebug() << "can not open file." << ret;
        parseResult = ParseFailed;
        context = nullptr;
        return false;
    }

    videoStream = av_find_best_stream(context, AVMEDIA_TYPE_VIDEO, 0, 0, nullptr, 0);
    if (videoStream < 0) {
        avformat_close_input(&context);
        parseResult = ParseFailed;
        return false;
    }

    AVStream *stream = context->streams[videoStream];
    AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        qDebug() << "can not find codec.";
        avformat_close_input(&context);
        parseResult = ParseFailed;
        return false;
    }
    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        qDebug() << "can not allocate codec context.";
        avformat_close_input(&context);
        parseResult = ParseFailed;
        return false;
    }
    if (avcodec_parameters_to_context(codecCtx, stream->codecpar) < 0) {
        qDebug() << "can not set parameters to codec context.";
        avcodec_free_context(&codecCtx);
        avformat_close_input(&context);
        parseResult = ParseFailed;
        return false;
    }
    if (avcodec_open2(codecCtx, nullptr, nullptr)) {
        qDebug() << "can not open codec context.";
        avcodec_free_context(&codecCtx);
        avformat_close_input(&context);
        parseResult = ParseFailed;
        return false;
    }
    timeBase = av_q2d(stream->time_base);
    parseResult = ParseSuccess;
    return true;
}


bool AnimationViewerPrivate::isPlaying()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    return thread && thread->isRunning() && !thread->exiting.loadRelaxed();
#else
    return thread && thread->isRunning() && !thread->exiting.load();
#endif
}


void AnimationViewerPrivate::updateProgress()
{
    Q_Q(AnimationViewer);
    if (isPlaying()) {
        playTime += timer.interval();
        if (!thread->queue.isEmpty()) {
            const VideoFrame &vf = thread->queue.get();
            if (vf.pts > playTime) {
                thread->queue.returnsForcely(vf);
            } else {
                q->ImageViewer::setImage(vf.image);
            }
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
    }
    thread.reset(new DecoderThread(this));
    thread->start();
    playTime = 0;
    timer.start();
}


void AnimationViewerPrivate::stop()
{
    if (thread) {
        if (thread->isRunning()) {
            thread->stop();
            thread->wait();
        }
        thread.reset();
    }
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
    }
    if (context) {
        avformat_close_input(&context);
    }
    parseResult = NotParsed;
    playTime = 0;
    timer.stop();
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


DecoderThread::DecoderThread(AnimationViewerPrivate *parent)
    : queue(3)
    , parent(parent)
    , exiting(false)
{

}


struct ScopedPointerNativeFrameDeleter
{
    static inline void cleanup(AVFrame *&pointer)
    {
        av_frame_free(&pointer);
    }
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
    static inline void cleanup(AVPacket *&pointer)
    {
        av_packet_free(&pointer);
    }
};


struct ScopedPointerSwsContextDeleter
{
    static inline void cleanup(SwsContext *pointer)
    {
        sws_freeContext(pointer);
    }
};


void DecoderThread::run()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    if (exiting.loadAcquire()) {
#else
    if (exiting.load()) {
#endif
        return;
    }

    AVFormatContext *context = parent->context;
    AVCodecContext *codecCtx = parent->codecCtx;
    if (!context || !codecCtx) {
        return;
    }

    QScopedPointer<AVFrame, ScopedPointerNativeFrameDeleter> nativeFrame(av_frame_alloc());
    QScopedPointer<AVFrame, ScopedPointerRgbFrameDeleter> rgbFrame(av_frame_alloc());
    if (av_image_alloc(rgbFrame->data, rgbFrame->linesize, codecCtx->width, codecCtx->height, AV_PIX_FMT_RGBA, 32) < 0) {
        qDebug() << "can not allocate frame buffer.";
        return;
    }
    rgbFrame->width = codecCtx->width;
    rgbFrame->height = codecCtx->height;
    rgbFrame->format = AV_PIX_FMT_RGBA;

    QScopedPointer<SwsContext, ScopedPointerSwsContextDeleter> swsCtx;

    while (true) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        bool x = exiting.loadAcquire();
#else
        bool x = exiting.load();
#endif
        QScopedPointer<AVPacket, ScopedPointerAvPacketDeleter> packet(av_packet_alloc());
        if (x || av_read_frame(context, packet.data())) {
            return;
        }
        if (packet->stream_index != parent->videoStream) {
            continue;
        }

        int r = avcodec_send_packet(codecCtx, packet.data());
        if (r != 0) {
            if (r == AVERROR(EAGAIN)) {
                qDebug() << "too much packet.";
            } else {
                qDebug() << "can not send packet.";
            }
            return;
        }

        while (true) {
            r = avcodec_receive_frame(codecCtx, nativeFrame.data());
            if (r == AVERROR(EAGAIN)) {
                break;
            } else if (r != 0) {
                qDebug() << "can not decode frame.";
                return;
            } else {
                if (!swsCtx) {
                    AVPixelFormat orignalFormat;
                    switch (codecCtx->pix_fmt) {
                    case AV_PIX_FMT_YUVJ420P :
                        orignalFormat = AV_PIX_FMT_YUV420P;
                        break;
                    case AV_PIX_FMT_YUVJ422P  :
                        orignalFormat = AV_PIX_FMT_YUV422P;
                        break;
                    case AV_PIX_FMT_YUVJ444P   :
                        orignalFormat = AV_PIX_FMT_YUV444P;
                        break;
                    case AV_PIX_FMT_YUVJ440P :
                        orignalFormat = AV_PIX_FMT_YUV440P;
                        break;
                    default:
                        orignalFormat = codecCtx->pix_fmt;
                        break;
                    }
                    swsCtx.reset(sws_getContext(codecCtx->width, codecCtx->height,
                                                orignalFormat,
                                                codecCtx->width, codecCtx->height,
                                                AV_PIX_FMT_RGBA,
                                                SWS_BILINEAR,
                                                nullptr, nullptr, nullptr));
                    if (!swsCtx) {
                        return;
                    }
                }

                int h = sws_scale(swsCtx.data(), nativeFrame->data, nativeFrame->linesize, 0, codecCtx->height,
                                  rgbFrame->data, rgbFrame->linesize);
                if (h <= 0) {
                    qDebug() << "can not scale frame.";
                }
//                rgbFrame->pts = nativeFrame->pts;
//                rgbFrame->color_trc = nativeFrame->color_trc;
//                rgbFrame->key_frame = nativeFrame->key_frame;
//                rgbFrame->pict_type = nativeFrame->pict_type;
//                rgbFrame->color_range = nativeFrame->color_range;
                QImage image(static_cast<const uchar*>(rgbFrame->data[0]),
                        nativeFrame->width, nativeFrame->height, rgbFrame->linesize[0],
                        QImage::Format_RGBA8888_Premultiplied);

                int64_t pts = static_cast<int64_t>(nativeFrame->pts * parent->timeBase * 1000.0);
                VideoFrame vf(image.copy(), pts, nativeFrame->pkt_dts);
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


AnimationViewer::AnimationViewer(QWidget *parent)
    :ImageViewer(parent, new AnimationViewerPrivate(this))
{
}


AnimationViewer::~AnimationViewer()
{
}


bool AnimationViewer::play()
{
    Q_D(AnimationViewer);
    if (d->isPlaying()) {
        d->timer.start();
        return true;
    }
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
        connect(this, SIGNAL(finished()), SLOT(play()));
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
    static inline void cleanup(AVFormatContext *&pointer)
    {
        avformat_close_input(&pointer);
    }
};


struct ScopedPointerAVCodecContextDeleter
{
    static inline void cleanup(AVCodecContext *&pointer)
    {
        avcodec_free_context(&pointer);
    }
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
    if (av_image_alloc(rgbFrame->data, rgbFrame->linesize, codecCtx->width, codecCtx->height, AV_PIX_FMT_RGBA, 32) < 0) {
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
                    case AV_PIX_FMT_YUVJ420P :
                        orignalFormat = AV_PIX_FMT_YUV420P;
                        break;
                    case AV_PIX_FMT_YUVJ422P  :
                        orignalFormat = AV_PIX_FMT_YUV422P;
                        break;
                    case AV_PIX_FMT_YUVJ444P   :
                        orignalFormat = AV_PIX_FMT_YUV444P;
                        break;
                    case AV_PIX_FMT_YUVJ440P :
                        orignalFormat = AV_PIX_FMT_YUV440P;
                        break;
                    default:
                        orignalFormat = codecCtx->pix_fmt;
                        break;
                    }
                    swsCtx.reset(sws_getContext(codecCtx->width, codecCtx->height,
                                            orignalFormat,
                                            codecCtx->width, codecCtx->height,
                                            AV_PIX_FMT_RGBA,
                                            SWS_BILINEAR,
                                            nullptr, nullptr, nullptr));
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
                QImage image(static_cast<const uchar*>(rgbFrame->data[0]),
                        nativeFrame->width, nativeFrame->height, rgbFrame->linesize[0],
                        QImage::Format_RGBA8888_Premultiplied);
                if (!image.isNull()) {
                    frames.append(image.copy());
                }
            }
        }
    }
}

#include "lafplay.moc"
