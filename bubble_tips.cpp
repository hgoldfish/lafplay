#include <QtCore/QTimer>
#include <QtCore/QPointer>
#include <QtGui/QPainter>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtWidgets/QHBoxLayout>
#include "bubble_tips.h"

QPointer<BubbleTips> s_lastTip;

BubbleTips::BubbleTips(QWidget *parent)
    : QWidget(parent)
    , label(new QLabel(this))
    , tm(new QTimer(this))
    , enter(false)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(label);

    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    setBackColor(0, 0, 0, 150);
    setDirect(DIRECT::DIRECT_BOTTOM, 0.75);

    QFont f;
    f.setPixelSize(14);
    setContentFont(f);

    setLeftTopMargin(5, 5);

    tm->start(1000);
    connect(tm, &QTimer::timeout, this, &BubbleTips::onTimer);

    if (!s_lastTip.isNull()) {
        s_lastTip->hide();
        s_lastTip->close();
    }
    QPointer<BubbleTips> self(this);
    s_lastTip = self;
}

BubbleTips::~BubbleTips()
{
    delete tm;
    delete label;
}

void BubbleTips::setBackColor(int r, int g, int b, int a)
{
    backColor = QColor(r, g, b, a);
}

void BubbleTips::setBackColor(const QColor &c)
{
    backColor = c;
}

void BubbleTips::setDirect(DIRECT direct, double size)
{
    this->direct = direct;
    posSize = size;
}

void BubbleTips::setContentFont(QFont font)
{
    label->setFont(font);
}

void BubbleTips::setContent(const QString &content, QColor color)
{
    label->setText(content);
    label->setStyleSheet(QString("color: rgb(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue()));
}

void BubbleTips::setLeftTopMargin(int leftMargin, int topMargin)
{
    this->leftMargin = leftMargin;
    this->topMargin = topMargin;
    this->layout()->setContentsMargins(leftMargin + transparentLength, topMargin + transparentLength,
                             leftMargin + transparentLength, topMargin + transparentLength);
}

void BubbleTips::setWordWrap(bool on)
{
    label->setWordWrap(on);
}

QPointer<BubbleTips> BubbleTips::showTips(const QString &message, QWidget *posFromWidget, const QPoint &diff/* = QPoint()*/)
{
    QPoint center;
    if (posFromWidget) {
        QRect rt = posFromWidget->geometry();
        QPoint topLeft = posFromWidget->mapToGlobal(QPoint(0, 0));
        rt.moveTopLeft(topLeft);
        center = rt.center();
    } else {
        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) {
            return nullptr;
        }
        center = screen->geometry().center();
    }
    center += diff;

    BubbleTips *tips = new BubbleTips();
    tips->setContent(message, Qt::white);
    tips->setDirect(DIRECT::DIRECT_NONE, 0);
    QTimer::singleShot(0, tips, [tips, center] {
        QSize sz = tips->size();
        QPoint dst = center - QPoint(sz.width() / 2, sz.height() / 2);
        tips->move(dst);
    });
    tips->move(center);
    tips->show();
    return tips;
}

QPointer<BubbleTips> BubbleTips::showTips(const QString &message, const QPoint &center)
{
    BubbleTips *tips = new BubbleTips();
    tips->setContent(message, Qt::white);
    tips->setDirect(DIRECT::DIRECT_NONE, 0);
    QTimer::singleShot(0, tips, [tips, center] {
        QSize sz = tips->size();
        QPoint dst = center - QPoint(sz.width() / 2, sz.height() / 2);
        tips->move(dst);
    });
    tips->move(center);
    tips->show();
    return tips;
}

void BubbleTips::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);
    painter.setBrush(backColor);

    // 相对于当前布局的起点坐标
    painter.drawRoundedRect(transparentLength, transparentLength, width() - transparentLength * 2,
                            height() - transparentLength * 2, 4, 4);
    QPointF points[3];

    switch (direct) {
    case DIRECT::DIRECT_NONE:
        return;
    case DIRECT::DIRECT_LEFT: {
        points[0] = QPointF(transparentLength, height() * posSize - triangleHeight);
        points[1] = QPointF(transparentLength - triangleHeight, height() * posSize);
        points[2] = QPointF(transparentLength, height() * posSize + triangleHeight);
        break;
    }
    case DIRECT::DIRECT_TOP: {
        points[0] = QPointF(width() * posSize - triangleHeight, transparentLength);
        points[1] = QPointF(width() * posSize, transparentLength - triangleHeight);
        points[2] = QPointF(width() * posSize + triangleHeight, transparentLength);
        break;
    }
    case DIRECT::DIRECT_RIGHT: {
        points[0] = QPointF(width() - transparentLength, height() * posSize - triangleHeight);
        points[1] = QPointF(width() - triangleHeight, height() * posSize);
        points[2] = QPointF(width() - transparentLength, height() * posSize + triangleHeight);
        break;
    }
    case DIRECT::DIRECT_BOTTOM: {
        points[0] = QPointF(width() * posSize - triangleHeight, height() - transparentLength);
        points[1] = QPointF(width() * posSize, height() - triangleHeight);
        points[2] = QPointF(width() * posSize + triangleHeight, height() - transparentLength);
        break;
    }
    default:
        break;
    }

    painter.drawPolygon(points, 3);
}

void BubbleTips::enterEvent(QEvent *)
{
    enter = true;
}

void BubbleTips::leaveEvent(QEvent *)
{
    enter = false;
}

void BubbleTips::onTimer()
{
    if (enter) {
        return;
    }
    display--;
    if (display <= 0) {
        close();
        return;
    }
}
