#ifndef LAFPLAY_ANIMATION_VIEWER_H
#define LAFPLAY_ANIMATION_VIEWER_H

#include "image_viewer.h"

class AnimationViewerPrivate;
class AnimationViewer : public ImageViewer
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
