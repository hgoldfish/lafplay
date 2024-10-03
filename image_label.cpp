#include <QtGui/QPainter>
#include <QtGui/QPicture> 
#include <QtGui/QPainterPath>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption> 
#include "image_label.h"

ImageLabel::ImageLabel(QWidget *parent)
    : QLabel(parent)
    , scaledpixmap(nullptr)
    , cachedimage(nullptr)
    , radius(8)
{
}

ImageLabel::~ImageLabel()
{
    delete scaledpixmap;
    delete cachedimage;
}

void ImageLabel::setPixmap(const QPixmap &pixmap)
{
    QLabel::setPixmap(pixmap);
    delete scaledpixmap;
    scaledpixmap = nullptr;
    delete cachedimage;
    cachedimage = nullptr;
}

void ImageLabel::setText(const QString &str)
{
    this->str = str;
}

void ImageLabel::setRadius(int radius)
{
    this->radius = radius;
    update();
}

QPainterPath borderClip(QRect r, int radius)
{
#if 1
    QPainterPath path;
    path.addRoundedRect(r, radius, radius);
    return path;
#else
    QSize tlr, trr, blr, brr;
    qNormalizeRadii(r, bd->radii, &tlr, &trr, &blr, &brr);
    if (tlr.isNull() && trr.isNull() && blr.isNull() && brr.isNull())
        return QPainterPath();

    const QRectF rect(r);
    const int *borders = border()->borders;
    QPainterPath path;
    qreal curY = rect.y() + borders[TopEdge] / 2.0;
    path.moveTo(rect.x() + tlr.width(), curY);
    path.lineTo(rect.right() - trr.width(), curY);
    qreal curX = rect.right() - borders[RightEdge] / 2.0;
    path.arcTo(curX - 2 * trr.width() + borders[RightEdge], curY,
        trr.width() * 2 - borders[RightEdge], trr.height() * 2 - borders[TopEdge], 90, -90);

    path.lineTo(curX, rect.bottom() - brr.height());
    curY = rect.bottom() - borders[BottomEdge] / 2.0;
    path.arcTo(curX - 2 * brr.width() + borders[RightEdge], curY - 2 * brr.height() + borders[BottomEdge],
        brr.width() * 2 - borders[RightEdge], brr.height() * 2 - borders[BottomEdge], 0, -90);

    path.lineTo(rect.x() + blr.width(), curY);
    curX = rect.left() + borders[LeftEdge] / 2.0;
    path.arcTo(curX, rect.bottom() - 2 * blr.height() + borders[BottomEdge] / 2,
        blr.width() * 2 - borders[LeftEdge], blr.height() * 2 - borders[BottomEdge], 270, -90);

    path.lineTo(curX, rect.top() + tlr.height());
    path.arcTo(curX, rect.top() + borders[TopEdge] / 2,
        tlr.width() * 2 - borders[LeftEdge], tlr.height() * 2 - borders[TopEdge], 180, -90);

    path.closeSubpath();
    return path;
#endif
}

void ImageLabel::paintEvent(QPaintEvent *)
{
    QStyle *style = QWidget::style();
    QPainter painter(this);

    QRect cr = contentsRect();
    cr.adjust(margin(), margin(), -margin(), -margin());
    int align = QStyle::visualAlignment(layoutDirection(), QFlag(this->alignment()));

    QPainterPath clipPath = borderClip(cr, radius);
    if (!clipPath.isEmpty()) {
        painter.save();
        painter.setClipPath(clipPath, Qt::IntersectClip);
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    const QPicture &pictureTmp = this->picture(Qt::ReturnByValue);
    const QPixmap &pixmapTmp = this->pixmap(Qt::ReturnByValue);
    const QPicture *picture = &pictureTmp;
    const QPixmap *pixmap = &pixmapTmp;
#else
    const QPicture *picture = this->picture();
    const QPixmap *pixmap = this->pixmap();
#endif
    QStyleOption opt;
    if (picture && !picture->isNull()) {
        QRect br = picture->boundingRect();
        int rw = br.width();
        int rh = br.height();
        if (hasScaledContents()) {
            painter.save();
            painter.translate(cr.x(), cr.y());
            painter.scale((double) cr.width() / rw, (double) cr.height() / rh);
            painter.drawPicture(-br.x(), -br.y(), *picture);
            painter.restore();
        } else {
            int xo = 0;
            int yo = 0;
            if (align & Qt::AlignVCenter)
                yo = (cr.height() - rh) / 2;
            else if (align & Qt::AlignBottom)
                yo = cr.height() - rh;
            if (align & Qt::AlignRight)
                xo = cr.width() - rw;
            else if (align & Qt::AlignHCenter)
                xo = (cr.width() - rw) / 2;
            painter.drawPicture(cr.x() + xo - br.x(), cr.y() + yo - br.y(), *picture);
        }
    } else if (pixmap && !pixmap->isNull()) {
        QPixmap pix;
        if (hasScaledContents()) {
            QSize scaledSize = cr.size() * devicePixelRatioF();
            if (!scaledpixmap || scaledpixmap->size() != scaledSize) {
                if (!cachedimage)
                    cachedimage = new QImage(pixmap->toImage());
                delete scaledpixmap;
                QImage scaledImage =
                        cachedimage->scaled(scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                scaledpixmap = new QPixmap(QPixmap::fromImage(std::move(scaledImage)));
                scaledpixmap->setDevicePixelRatio(devicePixelRatioF());
            }
            pix = *scaledpixmap;
        } else {
            pix = *pixmap;
        }

        opt.initFrom(this);
        if (!isEnabled()) {
            pix = style->generatedIconPixmap(QIcon::Disabled, pix, &opt);
        }
        style->drawItemPixmap(&painter, cr, align, pix);
    }

    if (!str.isEmpty()) {
        style->drawItemText(&painter, cr, align, opt.palette, isEnabled(), str, foregroundRole());
    }

    if (!clipPath.isEmpty()) {
        painter.restore();
    }
}
