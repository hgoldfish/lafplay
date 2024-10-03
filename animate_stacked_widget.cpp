#include "animate_stacked_widget.h"

AnimateStackedWidget::AnimateStackedWidget(QWidget *parent)
    : QStackedWidget(parent)
    , animationGroup(new QParallelAnimationGroup(this))
    , label(new QLabel(this))
{
    connect(animationGroup, &QParallelAnimationGroup::finished, this, &AnimateStackedWidget::onAnimationFinished);
}

AnimateStackedWidget::~AnimateStackedWidget()
{
    animationGroup->clear();
    delete animationGroup;
    delete label;
}

void AnimateStackedWidget::setAnimationDuration(int duration)
{
    animationDuration = duration;
}

void AnimateStackedWidget::switchToIndex(int index, AnimateMode mode/* = Mode_Show*/)
{
    if (animating) {
        if (index == indexAfterAnimation) {
            return;
        }
        int last = indexAfterAnimation;
        animating = false;
        animationGroup->stop();
        emit animationGroup->finished();
        startStackedWidgetAnimation(last, index, mode);
        return;
    }
    int oldIndex = currentIndex();
    if (index == oldIndex) {
        return;
    }
    startStackedWidgetAnimation(oldIndex, index, mode);
}

void AnimateStackedWidget::startStackedWidgetAnimation(int indexbefore, int indexafter, AnimateMode mode)
{
    if (animating) {
        return;
    }
    if (indexbefore >= count() || indexafter >= count() || indexbefore < 0 || indexafter < 0
        || indexafter == indexbefore) {
        return;
    }
    indexBeforeAnimation = indexbefore;
    indexAfterAnimation = indexafter;
    animating = true;
    startAnimationEngine(mode);
}

void AnimateStackedWidget::startAnimationEngine(AnimateMode mode)
{
    switch (mode) {
    case AnimateStackedWidget::Mode_Show: {
        int w = width();
        int h = height();
        label->setPixmap(this->grab());
        label->setGeometry(frameWidth(), frameWidth(), w - frameWidth() * 2, h - frameWidth() * 2);
        label->show();

        QWidget *widgetBefore = widget(indexBeforeAnimation);
        QWidget *widgetAfter = widget(indexAfterAnimation);

        setCurrentIndex(indexAfterAnimation);
        label->raise();

        QGraphicsOpacityEffect *animationOpacityEffect = new QGraphicsOpacityEffect();
        widgetAfter->setGraphicsEffect(animationOpacityEffect);
        widgetBefore->setGraphicsEffect(nullptr);

        QPropertyAnimation *animationOpacity = new QPropertyAnimation(animationOpacityEffect, "opacity");
        animationOpacity->setStartValue(0.3);
        animationOpacity->setEndValue(1.0);
        animationOpacity->setDuration(animationDuration);
        animationOpacity->setEasingCurve(QEasingCurve::OutQuad);
        connect(animationOpacity, &QPropertyAnimation::stateChanged, this, [this, animationOpacity] {
            qreal opacity = animationOpacity->currentValue().toReal();
            this->lastOpacity = opacity;
        });

        QPropertyAnimation *animationPos = new QPropertyAnimation(label, "pos");
        animationPos->setStartValue(QPointF(frameWidth(), frameWidth()));
        animationPos->setEndValue(QPointF(w - frameWidth(), -h + frameWidth()));
        animationPos->setDuration(animationDuration);
        animationPos->setEasingCurve(QEasingCurve::OutQuad);
        connect(animationPos, &QPropertyAnimation::stateChanged, this, [this, animationPos] {
            QPointF pt = animationPos->currentValue().toPointF();
            this->lastAnimationPos = pt;
        });

        animationGroup->clear();
        animationGroup->addAnimation(animationPos);
        animationGroup->addAnimation(animationOpacity);
        animationGroup->start();
        break;
    }
    case AnimateStackedWidget::Mode_Leave: {
        label->hide();

        QWidget *widgetBefore = widget(indexBeforeAnimation);
        QWidget *widgetAfter = widget(indexAfterAnimation);
        widgetAfter->raise();
        widgetAfter->show();
        widgetBefore->stackUnder(widgetAfter);
        widgetAfter->move(lastAnimationPos.x(), lastAnimationPos.y());
        widgetBefore->move(frameWidth(), frameWidth());

        QGraphicsOpacityEffect *animationOpacityEffect = new QGraphicsOpacityEffect();
        // animationOpacityEffect->setOpacity(lastOpacity);
        widgetBefore->setGraphicsEffect(animationOpacityEffect);
        widgetAfter->setGraphicsEffect(nullptr);
        QPropertyAnimation *animationOpacity = new QPropertyAnimation(animationOpacityEffect, "opacity");
        animationOpacity->setStartValue(lastOpacity);
        animationOpacity->setEndValue(0);
        animationOpacity->setDuration(animationDuration);
        animationOpacity->setEasingCurve(QEasingCurve::InQuad);

        QPropertyAnimation *animationPos = new QPropertyAnimation(widgetAfter, "pos");
        animationPos->setStartValue(lastAnimationPos);
        animationPos->setEndValue(QPointF(frameWidth(), frameWidth()));
        animationPos->setDuration(animationDuration);
        animationPos->setEasingCurve(QEasingCurve::OutQuad);

        animationGroup->clear();
        animationGroup->addAnimation(animationPos);
        animationGroup->addAnimation(animationOpacity);
        animationGroup->start();
        break;
    }
    default:
        break;
    }
}

void AnimateStackedWidget::onAnimationFinished()
{
    animating = false;
    label->hide();
    widget(indexAfterAnimation)->setGraphicsEffect(nullptr);
    setCurrentIndex(indexAfterAnimation);
    widget(indexAfterAnimation)->show();
    widget(indexAfterAnimation)->raise();
    emit animationFinished(indexBeforeAnimation, indexAfterAnimation);
}
