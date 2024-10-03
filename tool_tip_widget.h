#ifndef LAFPLAY_TOOLTIPWIDGET_H
#define LAFPLAY_TOOLTIPWIDGET_H

#include <QWidget>
#include <QTimer>

class ToolTipWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolTipWidget(QWidget *parent = nullptr);
    virtual ~ToolTipWidget();
    

    
protected:
    void showText(const QPoint &pos, QWidget *triggerWidget, const QRect &rect, int msecDisplayTime);

    void hideTip();
    void hideTipImmediately();
    void setTipRect(QWidget *trigger, const QRect &r);
    void restartExpireTimer(int msecDisplayTime);
    void placeTip(const QPoint &pos, QWidget *w);

    static QScreen* getTipScreen(QWidget *triggerWidget);

    void noHideBegin();
    void noHideEnd();
    virtual void hided() {}

    virtual void checkMouse(QMouseEvent *event);
protected:
    void timerEvent(QTimerEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    bool eventFilter(QObject *, QEvent *) override;
private:
    QWidget *triggerWidget;
    QRect rect;

    QBasicTimer hideTimer;
    QBasicTimer expireTimer;

    bool hideProtect;
};

#endif // LAFPLAY_TOOLTIPWIDGET_H
