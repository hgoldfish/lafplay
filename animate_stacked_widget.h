#ifndef LAFPLAY_ANIMATESTACKEDWIDGET_H
#define LAFPLAY_ANIMATESTACKEDWIDGET_H


#include <QtCore/QPropertyAnimation>
#include <QtCore/QParallelAnimationGroup>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QGraphicsOpacityEffect>
#include <QtWidgets/QLabel>

class AnimateStackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
    enum AnimateMode {
        Mode_Show,
        Mode_Leave,
    };
    AnimateStackedWidget(QWidget *parent);
    ~AnimateStackedWidget();
    // 设置动画时长
    void setAnimationDuration(int duration);
    void switchToIndex(int index, AnimateMode mode = Mode_Show);
signals:
    // 动画完成时发出信号
    void animationFinished(int indexbefore, int indexcurrent);
protected:
    void startStackedWidgetAnimation(int indexbefore, int indexafter, AnimateMode mode);
protected:
    void startAnimationEngine(AnimateMode mode);
    void onAnimationFinished();

    QParallelAnimationGroup * const animationGroup;
    QLabel * const label;
    QPointF lastAnimationPos;
    qreal lastOpacity = 0.0;
    int animationDuration = 1000;
    int indexBeforeAnimation = 0;
    int indexAfterAnimation = 0;
    bool animating = false;
};

#endif  // LAFPLAY_ANIMATESTACKEDWIDGET_H
