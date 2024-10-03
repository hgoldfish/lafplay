#ifndef LAFPLAY_IMAGELABEL_H
#define LAFPLAY_IMAGELABEL_H

#include <QLabel>

/// 可支持qss设置border,然后绘制圆角图片
class ImageLabel : public QLabel
{
public:
    explicit ImageLabel(QWidget *parent);
    virtual ~ImageLabel();
public:
    void setPixmap(const QPixmap &pixmap);
    void setText(const QString &str);
    void setRadius(int radius);
protected:
    void paintEvent(QPaintEvent *) override;
protected:
    QPixmap *scaledpixmap;
    QImage *cachedimage;
    QString str;
    int radius;
};

#endif // LAFPLAY_IMAGELABEL_H
