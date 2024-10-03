#include <QtCore/QEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QScreen>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QApplication>
#include "tool_tip_widget.h"

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<QApplication *>(QCoreApplication::instance()))


ToolTipWidget::ToolTipWidget(QWidget *parent)
    : QWidget(parent, Qt::ToolTip)
    , triggerWidget(nullptr)
    , hideProtect(false)
{
    //ensurePolished();
    qApp->installEventFilter(this);
    setMouseTracking(true);

    setAttribute(Qt::WA_DeleteOnClose);
}

ToolTipWidget::~ToolTipWidget()
{
    
}


void ToolTipWidget::showText(const QPoint &pos, QWidget *triggerWidget, const QRect &rect, int msecDisplayTime)
{
    Q_UNUSED(msecDisplayTime);
    if (isVisible()) { // a tip does already exist
        if (rect.isNull()) { 
            hideTip();
            return;
        }
    }

    if (rect.isValid()) {
        hideTimer.stop();
        expireTimer.stop();
        setTipRect(triggerWidget, rect);
        placeTip(pos, triggerWidget);
        showNormal();
    }
}

void ToolTipWidget::hideTip()
{
    if (!hideTimer.isActive())
        hideTimer.start(300, this);
}

void ToolTipWidget::hideTipImmediately()
{
    if (hideProtect) {
        return;
    }
    hided();
    close(); // to trigger QEvent::Close which stops the animation
}

void ToolTipWidget::setTipRect(QWidget *w, const QRect &r)
{
    triggerWidget = w;
    rect = r;
}

void ToolTipWidget::restartExpireTimer(int msecDisplayTime)
{
    int time = 10000;
    if (msecDisplayTime > 0)
        time = msecDisplayTime;
    expireTimer.start(time, this);
    hideTimer.stop();
}

void ToolTipWidget::placeTip(const QPoint &pos, QWidget *w)
{
    QPoint p = pos;
    const QScreen *screen = getTipScreen(w);
    // a QScreen's handle *should* never be null, so this is a bit paranoid
    if (const QPlatformScreen *platformScreen = screen ? screen->handle() : nullptr) {

        QPoint offset(2, 20);
        p += offset;

        QRect screenRect = screen->geometry();
        if (p.x() + this->width() > screenRect.x() + screenRect.width())
        p.rx() -= 4 + this->width();
        if (p.y() + this->height() > screenRect.y() + screenRect.height())
        p.ry() -= 24 + this->height();
        if (p.y() < screenRect.y())
            p.setY(screenRect.y());
        if (p.x() + this->width() > screenRect.x() + screenRect.width())
            p.setX(screenRect.x() + screenRect.width() - this->width());
        if (p.x() < screenRect.x())
            p.setX(screenRect.x());
        if (p.y() + this->height() > screenRect.y() + screenRect.height())
            p.setY(screenRect.y() + screenRect.height() - this->height());
    }
    this->move(p);
}


QScreen* ToolTipWidget::getTipScreen(QWidget *triggerWidget)
{
    if (triggerWidget == nullptr) {
        return nullptr;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QScreen *screen = triggerWidget->screen();
#else
    QScreen *screen = QApplication::screenAt(triggerWidget->geometry().center());
#endif
    return screen;
}

void ToolTipWidget::noHideBegin()
{
    hideProtect = true;
}

void ToolTipWidget::noHideEnd()
{
    hideProtect = false;
}

void ToolTipWidget::checkMouse(QMouseEvent *e)
{
    if (!rect.isNull()) {
        QPoint pos = e->globalPos();
        if (triggerWidget)
            pos = triggerWidget->mapFromGlobal(pos);
        if (rect.contains(pos)) {
            return;
        }
        hideTip();
    }

}

void ToolTipWidget::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == hideTimer.timerId()
        || e->timerId() == expireTimer.timerId()){
        hideTimer.stop();
        expireTimer.stop();
        hideTipImmediately();
    }
}
void ToolTipWidget::mouseMoveEvent(QMouseEvent *e)
{
    checkMouse(e);
    QWidget::mouseMoveEvent(e);
}

bool ToolTipWidget::eventFilter(QObject *o, QEvent *e)
{
    switch (e->type()) {
    case QEvent::Leave:
        if (o == triggerWidget) {
            hideTip();
        }
        break;
    case QEvent::Close: // For QTBUG-55523 (QQC) specifically: Hide tooltip when windows are closed
        if (o == triggerWidget) {
            hideTipImmediately();
        }
        break;
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
        hideTipImmediately();
        break;
    case QEvent::MouseMove:
        checkMouse(static_cast<QMouseEvent *>(e));
        break;
    default:
        break;
    }
    return false;
}
