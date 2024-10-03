#include <QtCore/qdatetime.h>
#include <QtCore/qrandom.h>
#include <QtGui/qpainter.h>
#include <math.h>
#include "scanning_widget.h"

ScanningWidget::ScanningWidget(QWidget *parent)
    : QWidget(parent)
    , angle(0)
    , scanning(false)
{
    timer.setInterval(10);
    timer.start();
    connect(&timer, SIGNAL(timeout()), SLOT(updateAngle()));
    QRandomGenerator *generator = QRandomGenerator::global();
    while (points.size() < 10) {
        double x = -1.0 + generator->generateDouble() * 2.0;
        double y = -1.0 + generator->generateDouble() * 2.0;
        if (x * x + y * y > 1.0) {
            continue;
        }
        points.append(QPointF(x, y));
    }
}

ScanningWidget::~ScanningWidget()
{
    timer.stop();
}

void ScanningWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QRect r = rect();
    int size = qMin(r.width(), r.height());
    QRect viewportRect(0, 0, size, size);
    viewportRect.moveCenter(r.center());
    QPainter painter(this);

    QBrush line(Qt::white);
    QConicalGradient cg(0, 0, angle);

    cg.setColorAt(0, QColor("#fda852"));
    cg.setColorAt(0.18, Qt::white);
    QBrush sweep(cg);

    // painter.setBrush(_black);
    // painter.drawRect(rect());

    painter.setViewport(viewportRect);
    painter.setWindow(-100, -100, 200, 200);
    painter.setRenderHint(QPainter::Antialiasing, true);

    //    QTransform transform;
    //    transform.shear(0.8, 0);
    //    transform.scale(0.8, 0.8);
    //    painter.setWorldTransform(transform);

    if (scanning) {
        painter.setBrush(sweep);
        painter.setPen(QPen(line, 1));
        painter.drawPie(QRect(-97, -97, 194, 194), angle * 16, 60 * 16);
    }

    painter.setBrush(QBrush());
    painter.setPen(QPen(line, 1));
    painter.drawEllipse(QRectF(-97, -97, 194, 194));
    // painter.drawEllipse(QRectF(-85, -85, 170, 170));
    for (float i = 0; i < 90; i += 30) {
        painter.drawEllipse(QRectF(-90 + i, -90 + i, 180 - i * 2, 180 - i * 2));
    }
    painter.setPen(QPen(line, 1));
    painter.drawLine(0, 0, 0, -97);
    for (float i = 0; i < 360; i += 30) {
        painter.drawLine(0, 0, sin(i * 3.1415926 * 2 / 360.0) * 97, -97 * cos(i * 3.1415926 * 2 / 360));
    }

    //    float x = sin(angle * 3.1415926 * 2 / 360);
    //    float y = -cos(angle * 3.1415926 * 2 / 360);
    // draw some shining stars
    for (const QPointF &p : points) {
        float ang = acos(p.x() / sqrt(p.x() * p.x() + p.y() * p.y()));
        if (p.y() > 0) {
            ang = 3.1415926 * 2 - ang;
        }
        ang = ang / 3.1415926 / 2 * 360;
        QPoint c(p.x() * 97, p.y() * 97);
        painter.drawPoint(c);
        if (angle + 60 >= 360 && ang < 60) {
            ang += 360;
        }
        if (angle < ang && ang < angle + 60) {
            painter.drawEllipse(c, 2, 2);
        }
    }
}

void ScanningWidget::updateAngle()
{
    angle += 2;
    if (angle >= 360) {
        angle -= 360;
    }
    if (isVisible()) {
        update();
    }
}

void ScanningWidget::start()
{
    scanning = true;
    updateAngle();
}

void ScanningWidget::stop()
{
    scanning = false;
    updateAngle();
}
