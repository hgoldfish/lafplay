#ifndef LAFPLAY_IMAGE_VIEWER_P_H
#define LAFPLAY_IMAGE_VIEWER_P_H
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
#include "image_viewer.h"

class ImageViewerPrivate : public QObject
{
    Q_OBJECT
public:
    ImageViewerPrivate(ImageViewer *q);
    void dropCache();
    void rebuildCached();
public slots:
    void prepareBuildingCache();
    void cacheBuilt(const QImage &image, const QSize &targetSize, const QPoint &tagetPosition, double targetRatio);
public:
    QImage image;
    QString imagePath;

    QPixmap cached;
    QSize cachedSize;  // cachedSize != cached.size()
    ImageViewer::Mode mode;
    QPoint pos;  // the position to draw cache.

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

#endif // LAFPLAY_IMAGE_VIEWER_P_H
