#include "image_viewer_p.h"

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

class BuildCache : public QRunnable
{
public:
    BuildCache(QPointer<ImageViewerPrivate> p, Qt::TransformationMode tranMode, ImageViewer::Mode mode, double raite,
               const QImage &image, const QString &imagePath, const QSize &targetRect, const QPoint &targetPosition,
               double devicePixelRatioF);
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
{
}

BuildCache::~BuildCache() { }

void BuildCache::run()
{
    if (p.isNull())
        return;
    QImage result;
    double targetRatio = 1.0;

    if (mode == ImageViewer::OriginalSize || (mode == ImageViewer::CustomRatio && qFuzzyCompare(ratio, 1.0))) {
        result = loadImage();
    } else if (mode == ImageViewer::FitHeight) {
        const QImage &img = loadImage();
        if (!img.isNull()) {
            result = img.scaledToHeight(static_cast<int>(targetSize.height() * devicePixelRatioF), tranMode);
            targetRatio = result.height() / img.height();
        }
    } else if (mode == ImageViewer::FitWidth) {
        const QImage &img = loadImage();
        if (!img.isNull()) {
            result = img.scaledToWidth(static_cast<int>(targetSize.width() * devicePixelRatioF), tranMode);
            targetRatio = result.width() / img.width();
        }
    } else if (mode == ImageViewer::CustomRatio) {
        const QImage &img = loadImage();
        if (!img.isNull()) {
            result = img.scaled(targetSize * ratio * devicePixelRatioF, Qt::KeepAspectRatio, tranMode);
            targetRatio = ratio;
        }
    } else {  // defautl to fitwindow
        const QImage &img = loadImage();
        if (!img.isNull()) {
            result = img.scaled(targetSize * devicePixelRatioF, Qt::KeepAspectRatio, tranMode);
            targetRatio = result.width() / img.width();
        }
    }
    if (p.isNull()) {
        return;
    }
    QMetaObject::invokeMethod(p.data(), "cacheBuilt", Qt::QueuedConnection, Q_ARG(QImage, result),
                              Q_ARG(QSize, targetSize), Q_ARG(QPoint, targetPosition), Q_ARG(double, targetRatio));
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
    BuildCache *task = new BuildCache(this, transformationMode, mode, ratio, image, imagePath, q->size(), pos,
                                      q->devicePixelRatioF());
    task->setAutoDelete(true);
    QThreadPool::globalInstance()->start(task);
}

void ImageViewerPrivate::cacheBuilt(const QImage &result, const QSize &targetSize, const QPoint &, double targetRatio)
{
    Q_Q(ImageViewer);
    building.storeRelease(false);
    if (targetSize != q->size()) {  // maybe size is changed while generating.
        rebuildCached();
    } else {
        cached = QPixmap::fromImage(result);
        cachedSize = targetSize;
        ratio = targetRatio;
        q->update();
    }
}

ImageViewer::ImageViewer(QWidget *parent)
    : QWidget(parent)
    , dd_ptr(new ImageViewerPrivate(this))

{
}

ImageViewer::ImageViewer(QWidget *parent, ImageViewerPrivate *d)
    : QWidget(parent)
    , dd_ptr(d)
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

    // 能否只绘制窗口被覆盖的部分，而不是整个窗口重绘？
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
        int distance = static_cast<QApplication *>(QApplication::instance())->startDragDistance();
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
    if ((event->pos() - d->startDragPos).manhattanLength()
        > static_cast<QApplication *>(QApplication::instance())->startDragDistance()) {
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
        if (d->pos.x() < leftMost) {
            d->pos.setX(leftMost);
        }
        if (d->pos.x() > rightMost) {
            d->pos.setX(rightMost);
        }
        if (d->pos.y() < topMost) {
            d->pos.setY(topMost);
        }
        if (d->pos.y() > bottomMost) {
            d->pos.setY(bottomMost);
        }
        if (d->pos != oldPos) {
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

// void ImageViewer::wheelEvent(QWheelEvent *event)
//{
//     Q_D(ImageViewer);
//     if (d->mode == CustomRatio) {
//         QPoint numPixels = event->pixelDelta();
//         QPoint numDegrees = event->angleDelta() / 8;

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
    return !d->image.isNull() || !d->imagePath.isEmpty();
}

QImage ImageViewer::image()
{
    Q_D(ImageViewer);
    if (d->image.isNull()) {
        if (d->imagePath.isEmpty()) {
            return QImage();
        }
        return QImage(d->imagePath);
    } else {
        return d->image;
    }
}

QString ImageViewer::imagePath()
{
    Q_D(ImageViewer);
    return d->imagePath;
}

QSize ImageViewer::imageSize()
{
    return image().size();
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
    if (!changed) {
        return;
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
    d->rebuildCached();
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
    if (d->transformationMode != transformationMode) {
        d->transformationMode = transformationMode;
        d->rebuildCached();
    }
}
