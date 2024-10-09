#ifndef LAFPLAY_ANIMATION_VIEWER_H
#define LAFPLAY_ANIMATION_VIEWER_H

#include <QtWidgets/qwidget.h>

class AnimationViewerPrivate;
class AnimationViewer: public QWidget
{
    Q_OBJECT
public:
    enum ParseResult {
        ParseSuccess,
        ParseFailed,
        NotParsed,
    };
public:
    explicit AnimationViewer(QWidget *parent = nullptr);
    virtual ~AnimationViewer() override;
public:
    QString url() const;
    bool isPlaying() const;
public slots:
    void setUrl(const QString &url);
    void setFrameBufferSize(int size);
    void play();
    void stop();
    void pause();
    void resume();
    void setAutoRepeat(bool autoRepeat);
protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void hideEvent(QHideEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;
signals:
    void parsed(ParseResult result);
    void started();
    void finished();
private:
    AnimationViewerPrivate * const dd_ptr;
    Q_DECLARE_PRIVATE_D(dd_ptr, AnimationViewer)
};

QList<QImage> convertVideoToImages(const QString &filePath, QString *reason);


#endif
