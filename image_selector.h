#ifndef LAFPLAY_IMAGE_SELECTOR_H
#define LAFPLAY_IMAGE_SELECTOR_H

#include <QWidget>
#include <QPixmap>
#include <QImage>


class ImageSelectorPrivate;
class ImageSelector : public QWidget
{
    Q_OBJECT
public:
    explicit ImageSelector(QWidget *parent = nullptr);
    virtual ~ImageSelector() override;
    virtual QSize sizeHint() const override;
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;
public:
    int addImage(const QImage &image);
    QSize imageSize() const;
    int displayCells() const;
    void setOrientation(Qt::Orientation orientation);
    Qt::Orientation orientation() const;
    int currentIndex() const;
public slots:
    void clear();
    void setCurrentIndex(int index);
signals:
    void currentChanged(int index);
private slots:
    void playNextFrame();
private:
    ImageSelectorPrivate * const dd_ptr;
    Q_DECLARE_PRIVATE_D(dd_ptr, ImageSelector)
};


#endif // LAFPLAY_IMAGE_SELECTOR_H
