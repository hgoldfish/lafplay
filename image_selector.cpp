#include <QTimer>
#include <QPainter>
#include <QMouseEvent>
#include "image_selector.h"

class ImageSelectorPrivate
{
public:
    ImageSelectorPrivate();
public:
    QList<QImage> bigImages;
    QSize cachingSize;
    QPixmap animationCache;
    QTimer animationTimer;
    Qt::Orientation orientation;
    int animationDeltaX;
    int currentIndex;
};

ImageSelectorPrivate::ImageSelectorPrivate()
    : orientation(Qt::Horizontal)
    , animationDeltaX(0)
    , currentIndex(-1)
{
    animationTimer.setInterval(30);
}

ImageSelector::ImageSelector(QWidget *parent)
    : QWidget(parent)
    , dd_ptr(new ImageSelectorPrivate)
{
    Q_D(ImageSelector);
    connect(&d->animationTimer, SIGNAL(timeout()), SLOT(playNextFrame()));
    setCurrentIndex(0);
}

ImageSelector::~ImageSelector()
{
    delete dd_ptr;
}

QSize ImageSelector::imageSize() const
{
    Q_D(const ImageSelector);
    if (d->orientation == Qt::Horizontal) {
        int height = this->height() - 2;
        return QSize(height, height);
    } else {
        int width = this->width() - 2;
        return QSize(width, width);
    }
}

int ImageSelector::displayCells() const
{
    Q_D(const ImageSelector);
    int maxCells;
    if (d->orientation == Qt::Horizontal) {
        maxCells = this->width() / this->height();
    } else {
        maxCells = this->height() / this->width();
    }
    if (maxCells % 2 == 0) {
        return maxCells - 1;
    } else {
        return maxCells;
    }
}

void ImageSelector::setOrientation(Qt::Orientation orientation)
{
    Q_D(ImageSelector);
    d->orientation = orientation;
}

Qt::Orientation ImageSelector::orientation() const
{
    Q_D(const ImageSelector);
    return d->orientation;
}

int ImageSelector::currentIndex() const
{
    Q_D(const ImageSelector);
    return d->currentIndex;
}

int ImageSelector::addImage(const QImage &image)
{
    Q_D(ImageSelector);
    int index = d->bigImages.size();
    d->bigImages.append(image);
    d->animationCache = QPixmap();
    if (d->currentIndex < 0) {
        d->currentIndex = 0;
    }
    if (isVisible()) {
        update();
    }
    return index;
}

void ImageSelector::clear()
{
    Q_D(ImageSelector);
    d->currentIndex = -1;
    d->bigImages.clear();
    d->animationDeltaX = 0;
    d->animationTimer.stop();
    update();
}

void ImageSelector::setCurrentIndex(int index)
{
    Q_D(ImageSelector);
    if (d->currentIndex == index) {
        return;
    }
    if (d->currentIndex >= 0) {
        int cellSize;
        if (d->orientation == Qt::Horizontal) {
            cellSize = imageSize().width();
        } else {
            cellSize = imageSize().height();
        }
        d->animationDeltaX = (index - d->currentIndex) * cellSize;
        d->currentIndex = index;
        d->animationCache = QPixmap();
        d->animationTimer.start();
    } else {
        d->currentIndex = index;
        update();
    }
}

QSize ImageSelector::sizeHint() const
{
    Q_D(const ImageSelector);
    if (d->orientation == Qt::Horizontal) {
        return QSize(30 * 7, 30);
    } else {
        return QSize(30, 30 * 7);
    }
}

void ImageSelector::paintEvent(QPaintEvent *event)
{
    Q_D(ImageSelector);
    if (d->currentIndex < 0 || d->currentIndex >= d->bigImages.size()) {
        return QWidget::paintEvent(event);
    }
    const QSize &s = imageSize();
    int cells = displayCells();
    double dpr = devicePixelRatioF();
    if (d->animationCache.isNull() || d->cachingSize != s) {
        int index = d->currentIndex - (cells / 2 + 1);
        int width, height;
        if (d->orientation == Qt::Horizontal) {
            width = s.width() * (cells + 2);
            height = s.height();
        } else {
            width = s.width();
            height = s.height() * (cells + 2);
        }
        d->animationCache = QPixmap(static_cast<int>(width * dpr), static_cast<int>(height * dpr));
        QPainter painter1(&d->animationCache);
        painter1.fillRect(QRectF(0, 0, width * dpr, height * dpr), palette().brush(QPalette::Window));
        for (int i = 0; i < (cells + 2); ++i) {
            if (index < 0) {
                index += 1;
                continue;
            }
            if (index >= d->bigImages.size()) {
                break;
            }
            QRectF rect;
            if (d->orientation == Qt::Horizontal) {
                rect.setRect(s.width() * i * dpr + 1, 1, s.width() * dpr - 2, s.height() * dpr - 2);
            } else {
                rect.setRect(1, s.height() * i * dpr + 1, s.width() * dpr - 2, s.height() * dpr - 2);
            }
            const QImage &smallImage =
                    d->bigImages.at(index).scaled(s * dpr, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            painter1.drawImage(rect, smallImage);
            index += 1;
        }
        d->cachingSize = s;
    }
    QPixmap center;
    QPainter painter2(this);
    painter2.setOpacity(0.8);
    if (d->orientation == Qt::Horizontal) {
        center = d->animationCache.copy(
                static_cast<int>(s.width() * (cells / 2 + 1) * dpr + 1 - d->animationDeltaX * dpr), 1,
                static_cast<int>(s.width() * dpr - 2), s.height() * dpr - 2);
        if (d->animationDeltaX < 0) {
            painter2.drawPixmap(QRectF(d->animationDeltaX, 0, s.width() * (cells + 1), s.height()), d->animationCache,
                                QRectF(s.width() * dpr, 0, s.width() * (cells + 1) * dpr, s.height() * dpr));
        } else if (d->animationDeltaX > 0) {
            painter2.drawPixmap(QRectF(-s.width() + d->animationDeltaX, 0, s.width() * (cells + 1), s.height()),
                                d->animationCache, QRectF(0, 0, s.width() * (cells + 1) * dpr, s.height() * dpr));
        } else {
            painter2.drawPixmap(QRectF(0, 0, s.width() * cells, s.height()), d->animationCache,
                                QRectF(s.width() * dpr, 0, s.width() * cells * dpr, s.height() * dpr));
        }
        painter2.setOpacity(1);
        painter2.drawPixmap(QRectF(s.width() * (cells / 2) + 1, 1, s.width() - 2, s.height() - 2), center,
                            QRect(0, 0, s.width() * dpr - 2, s.height() * dpr - 2));
        painter2.drawRect(QRectF(s.width() * (cells / 2) + 1, 1, s.width() - 2, s.height() - 2));
    } else {
        center = d->animationCache.copy(1, s.height() * (cells / 2 + 1) * dpr + 1 - d->animationDeltaX * dpr,
                                        s.width() * dpr - 2, s.height() * dpr - 2);
        if (d->animationDeltaX < 0) {
            painter2.drawPixmap(QRectF(0, d->animationDeltaX, s.width(), s.height() * (cells + 1)), d->animationCache,
                                QRectF(0, s.height() * dpr, s.width() * dpr, s.height() * (cells + 1) * dpr));
        } else if (d->animationDeltaX > 0) {
            painter2.drawPixmap(QRectF(0, -s.height() + d->animationDeltaX, s.width(), s.height() * (cells + 1)),
                                d->animationCache, QRectF(0, 0, s.width() * dpr, s.height() * (cells + 1) * dpr));
        } else {
            painter2.drawPixmap(QRectF(0, 0, s.width(), s.height() * cells), d->animationCache,
                                QRectF(0, s.height() * dpr, s.width() * dpr, s.height() * cells * dpr));
        }
        painter2.setOpacity(1);
        painter2.drawPixmap(QRectF(1, s.height() * (cells / 2) + 1, s.width() - 2, s.height() - 2), center,
                            QRectF(0, 0, s.width() * dpr - 2, s.height() * dpr - 2));
        painter2.drawRect(QRectF(1, s.height() * (cells / 2) + 1, s.width() - 2, s.height() - 2));
    }
}

void ImageSelector::mousePressEvent(QMouseEvent *event)
{
    Q_D(ImageSelector);
    if (event->button() != Qt::LeftButton) {
        return QWidget::mousePressEvent(event);
    }
    int delta;
    if (d->orientation == Qt::Horizontal) {
        delta = event->pos().x() / imageSize().width() - (displayCells() / 2);
    } else {
        delta = event->pos().y() / imageSize().height() - (displayCells() / 2);
    }

    int index = d->currentIndex + delta;
    if (delta != 0 && index >= 0 && index < d->bigImages.size()) {
        emit currentChanged(index);
        setCurrentIndex(index);
    }
    QWidget::mousePressEvent(event);
}

void ImageSelector::resizeEvent(QResizeEvent *event)
{
    Q_D(ImageSelector);
    d->cachingSize.setHeight(0);
    d->cachingSize.setWidth(0);
    d->animationCache = QPixmap();
    d->animationTimer.stop();
    QWidget::resizeEvent(event);
}

void ImageSelector::wheelEvent(QWheelEvent *event)
{
    Q_D(ImageSelector);
    int numSteps = event->angleDelta().y() / 8 / 15;
    int index;
    if (numSteps > 0) {
        index = d->currentIndex - 1;
    } else {
        index = d->currentIndex + 1;
    }
    if (index >= 0 && index < d->bigImages.size()) {
        emit currentChanged(index);
        setCurrentIndex(index);
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void ImageSelector::playNextFrame()
{
    Q_D(ImageSelector);
    QList<int> keyFrames;
    keyFrames << 45 << 30 << 20 << 15 << 10 << 6 << 3 << 2 << 1 << 0;
    int nextFrame = 0;
    for (int keyFrame : keyFrames) {
        if (qAbs(d->animationDeltaX) > keyFrame) {
            nextFrame = keyFrame;
            break;
        }
    }
    if (d->animationDeltaX < 0) {
        d->animationDeltaX = -nextFrame;
    } else {
        d->animationDeltaX = nextFrame;
    }
    if (d->animationDeltaX == 0) {
        d->animationTimer.stop();
    }
    update();
}
