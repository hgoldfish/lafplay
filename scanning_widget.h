#ifndef LAFPLAY_SCANING_WIDGET_H
#define LAFPLAY_SCANING_WIDGET_H

#include <QWidget>
#include <QTimer>
#include <QPoint>
#include <QMatrix4x4>

class ScanningWidget : public QWidget
{
    Q_OBJECT
public:
    ScanningWidget(QWidget *parent = 0);
    virtual ~ScanningWidget() override;
public:
    void start();
    void stop();
protected:
    virtual void paintEvent(QPaintEvent *event) override;
private slots:
    void updateAngle();
private:
    QTimer timer;
    QList<QPointF> points;
    float angle;
    bool scanning;
};

#endif  // LAFPLAY_SCANING_WIDGET_H
