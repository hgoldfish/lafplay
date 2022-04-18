#ifndef LAFPLAY_H
#define LAFPLAY_H

#include <QtWidgets/qlabel.h>


class ImageViewerPrivate;
class ImageViewer: public QWidget
{
    Q_OBJECT
public:
    enum Mode {
        OriginalSize,
        FitWindow,
        FitHeight,
        FitWidth,
        CustomRatio,
    };
public:
    ImageViewer(QWidget *parent = nullptr);
    virtual ~ImageViewer() override;
signals:
    void clicked(Qt::KeyboardModifiers modifiers);
    void showContextMenu();
public slots:
    bool setImage(const QImage &image);
    virtual bool setFile(const QString &imagePath);
    void clear();
public:
    bool hasImage();
    QImage image();
    virtual QString imagePath();
    QSize imageSize();
    void setMode(Mode mode, double ratio = 0.0);
    Mode mode() const;
    double ratio() const;
    Qt::TransformationMode transformationMode() const;
    void setTransformationMode(Qt::TransformationMode transformationMode);
protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void hideEvent(QHideEvent *event) override;
//    void wheelEvent(QWheelEvent *event) override;
protected:
    ImageViewerPrivate * const dd_ptr;
    ImageViewer(QWidget *parent, ImageViewerPrivate *d);
private:
    Q_DECLARE_PRIVATE_D(dd_ptr, ImageViewer)
};


class AnimationViewerPrivate;
class AnimationViewer: public ImageViewer
{
    Q_OBJECT
public:
    explicit AnimationViewer(QWidget *parent = nullptr);
    virtual ~AnimationViewer() override;
public:
    virtual QString imagePath() override;
public slots:
    virtual bool setFile(const QString &filePath) override;
    bool play();
    void stop();
    void pause();
    void setAutoRepeat(bool autoRepeat);
protected:
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
signals:
    void finished();
private:
    Q_DECLARE_PRIVATE_D(dd_ptr, AnimationViewer)
};


QList<QImage> convertVideoToImages(const QString &filePath, QString *reason);

#endif
