#include <QtCore/qvariantanimation.h>
#include <QtGui/qevent.h>
#include <QtGui/qpainter.h>
#include <QtGui/qpixmap.h>
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qstyleoption.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qstylepainter.h>
#include "vertical_navigator.h"

struct NavigatorButton
{
public:
    NavigatorButton()
        : id(0)
        , isStretch(false)
        , selected(false)
        , hasFocus(true)
        , isVisible(true)
    {
    }
    QIcon on;
    QIcon off;
    QPixmap error;
    QString text;
    int id;
    int number;
    bool isStretch;
    bool selected;
    bool hasFocus;
    bool isVisible;

    QString onIconPath;
    QString offIconPath;
};

class VerticalNavigatorPrivate
{
public:
    VerticalNavigatorPrivate(VerticalNavigator *q);
public:
    void drawHighlightBackground(const QRectF &rect);
    QRectF getRect(int index) const;
    void setCurrentButton(int pageId);
    void _q_doAnimation(const QVariant &value);
public:
    QList<NavigatorButton> buttons;
    QVariantAnimation backgroundAnimation;
    QRectF currentAnimationbackgroundRect;

    QMargins margins;
    QSize iconSize;
    int padding;
private:
    VerticalNavigator * const q_ptr;
    Q_DECLARE_PUBLIC(VerticalNavigator);
};

VerticalNavigatorPrivate::VerticalNavigatorPrivate(VerticalNavigator *q)
    : margins(16, 7, 17, 7)
    , iconSize(32, 32)
    , padding(7)
    , q_ptr(q)
//    , colorSelectItemBgStart(Qt::black)
//    , colorSelectItemBgEnd(Qt::white)
{
    backgroundAnimation.setDuration(100);
    backgroundAnimation.setEasingCurve(QEasingCurve::OutCurve);
    QObject::connect(&backgroundAnimation, SIGNAL(valueChanged(QVariant)), q, SLOT(_q_doAnimation(QVariant)));
}

void VerticalNavigatorPrivate::drawHighlightBackground(const QRectF &rect)
{
    backgroundAnimation.stop();
    if (!currentAnimationbackgroundRect.isNull()) {
        backgroundAnimation.setStartValue(currentAnimationbackgroundRect);
    } else {
        backgroundAnimation.setStartValue(QVariant::fromValue(rect));
    }
    backgroundAnimation.setEndValue(rect);
    backgroundAnimation.start();
}

void VerticalNavigatorPrivate::_q_doAnimation(const QVariant &value)
{
    Q_Q(VerticalNavigator);
    currentAnimationbackgroundRect = value.toRect();
    if (currentAnimationbackgroundRect.isNull()) {
        currentAnimationbackgroundRect = backgroundAnimation.endValue().toRect();
    }
    q->update();
}

QRectF VerticalNavigatorPrivate::getRect(int pageId) const
{
    Q_Q(const VerticalNavigator);
    const QRectF &r = q->rect();
    const int w = r.width();
    const int h = iconSize.height() + margins.top() + margins.bottom();
    const int left = margins.left();
    const int right = margins.right();
    int top = 0;
    int i = 0;
    for (; i < buttons.size(); ++i) {
        if (buttons[i].isStretch) {
            break;
        }
        if (buttons[i].id == pageId) {
            if (!buttons[i].isVisible) {
                return QRectF();
            } else {
                return QRectF(left, top, w - left - right, h);
            }
        }
        if (buttons[i].isVisible) {
            top += h + padding;
        }
    }
    if (i < buttons.size()) {
        top = r.height() - h * 1.5;
        for (i = buttons.size() - 1; i >= 0; --i) {
            if (buttons[i].isStretch) {
                if (pageId == 0) {
                    QRectF stretchRect(left, top + h / 2, w - left - right, 0.1);
                    return stretchRect;
                } else {
                    return QRectF();
                }
            }

            if (buttons[i].id == pageId) {
                if (!buttons[i].isVisible) {
                    return QRectF();
                } else {
                    return QRectF(left, top, w - left - right, h);
                }
            }
            if (buttons[i].isVisible) {
                top -= h + padding / 2;
            }
        }
    }
    return QRectF();
}

void VerticalNavigatorPrivate::setCurrentButton(int pageId)
{
    for (NavigatorButton &button : buttons) {
        button.selected = button.id == pageId;
    }
    const QRectF &r = getRect(pageId);
    drawHighlightBackground(r);
}

VerticalNavigator::VerticalNavigator(QWidget *parent)
    : QFrame(parent)
    , dd_ptr(new VerticalNavigatorPrivate(this))
{
    setFrameStyle(QFrame::NoFrame);
}

VerticalNavigator::~VerticalNavigator()
{
    delete dd_ptr;
}

void VerticalNavigator::paintEvent(QPaintEvent *event)
{
    Q_D(VerticalNavigator);
    QWidget::paintEvent(event);
    QStyle *s = this->style();
    QStylePainter painter(this);
    QPalette p = palette();

    const QColor &highlightedText = QColor("#FF7D00");
    const QColor &normalText = QColor("#AFB3C7");
    const QColor &selected = QColor("#FFEDDC");
    const QColor &stretchColor = QColor("#E0E0E0");
    const int rounded = 10;
    const int marginLeft = 15;
    const int marginTop = 2;

    QStyleOptionToolButton option;
    option.font = this->font();
    option.direction = this->layoutDirection();
    option.fontMetrics = this->fontMetrics();
    option.styleObject = this;
    option.toolButtonStyle = Qt::ToolButtonTextUnderIcon;
    option.features = QStyleOptionToolButton::None;
    option.arrowType = Qt::NoArrow;
    option.state = QStyle::State_Enabled;
    option.subControls = QStyle::SC_ToolButton;
    option.activeSubControls = QStyle::SC_ToolButton;
    // option.rect = QRectF(0, 0, w, h);
    option.iconSize = d->iconSize;

    if (Q_LIKELY(!d->currentAnimationbackgroundRect.isNull())) {
        // painter.drawPixmap(d->currentAnimationbackgroundRect, d->selectorBackgroud);
        painter.setPen(selected);
        painter.setBrush(selected);
        painter.drawRoundedRect(d->currentAnimationbackgroundRect, rounded, rounded);
    }

    for (const NavigatorButton &button : d->buttons) {
        if (button.isStretch || !button.isVisible) {
            continue;
        }
        option.text = QString("    ") + button.text;
        QTransform transform;
        const QRectF &buttonRect = d->getRect(button.id);
        transform.translate(marginLeft, buttonRect.top() + marginTop);
        painter.setTransform(transform);
        if (button.selected) {
            option.icon = button.on;
            p.setColor(QPalette::ButtonText, highlightedText);
        } else {
            option.icon = button.off;
            p.setColor(QPalette::ButtonText, normalText);
        }
        option.palette = p;
        option.toolButtonStyle = Qt::ToolButtonTextBesideIcon;
        option.rect =
                QRect(marginLeft, marginTop, buttonRect.width() - marginLeft * 2, buttonRect.height() - marginTop * 2);
        s->drawComplexControl(QStyle::CC_ToolButton, &option, &painter, this);

        // paint number
        if (button.number > 0) {
            QFont f = option.font;
            f.setPointSize(6);
            painter.setFont(f);
            QFontMetrics fontMetrics(f);

            QString number = button.number > 99 ? QString("99+") : QString::number(button.number);
            QSize textSize = fontMetrics.boundingRect(number).size();
            textSize += QSize(2, 2);  // margin
            if (textSize.width() < 16) {
                textSize.setWidth(16);
            }
            if (textSize.height() < 10) {
                textSize.setHeight(10);
            }
            // +4 是因为qt style 画 icon 才加上的
            QRect textRt(marginLeft + d->iconSize.width() + 6 - textSize.width(), marginTop, textSize.width(), textSize.height());
            if (textRt.left() < marginLeft) {
                textRt.moveLeft(marginLeft);
            }
            painter.setBrush(highlightedText);
            painter.setPen(Qt::white);
            painter.drawRoundedRect(textRt, textRt.height() >> 1, textRt.height() >> 1);
            painter.drawText(textRt, Qt::AlignCenter, number);
        } else if (button.number < 0 && !button.error.isNull()) {
            // +4 是因为qt style 画 icon 才加上的
#define ERROR_ICON_SIZE 15
            QRect iconRt(marginLeft + d->iconSize.width() + 4 - ERROR_ICON_SIZE, marginTop, ERROR_ICON_SIZE, ERROR_ICON_SIZE);
            if (iconRt.left() < marginLeft) {
                iconRt.moveLeft(marginLeft);
            }
            painter.drawPixmap(iconRt, button.error);
        }
    }

    const QRectF &stretchRect = d->getRect(0);
    if (!stretchRect.isNull()) {
        painter.resetTransform();
        painter.setPen(stretchColor);
        painter.setBrush(stretchColor);
        painter.drawRect(stretchRect);
    }
}

void VerticalNavigator::mousePressEvent(QMouseEvent *event)
{
    Q_D(VerticalNavigator);
    if (!(event->buttons() & Qt::LeftButton)) {
        QFrame::mousePressEvent(event);
        return;
    }
    for (const NavigatorButton &button : d->buttons) {
        if (button.isStretch || !button.isVisible) {
            continue;
        }
        const QRectF &r = d->getRect(button.id);
        if (r.contains(event->pos())) {
            if (button.hasFocus) {
                d->setCurrentButton(button.id);
            }
            emit triggered(button.id);
            event->accept();
            return;
        }
    }
}

void VerticalNavigator::resizeEvent(QResizeEvent *event)
{
    Q_D(VerticalNavigator);
    QFrame::resizeEvent(event);
    d->backgroundAnimation.stop();
    int pageId = currentButton();
    if (pageId) {
        d->currentAnimationbackgroundRect = d->getRect(pageId);
    }
    update();
}

int VerticalNavigator::currentButton() const
{
    Q_D(const VerticalNavigator);
    for (const NavigatorButton &button : d->buttons) {
        if (button.selected) {
            return button.id;
        }
    }
    return 0;
}

void VerticalNavigator::setCurrentButton(int pageId)
{
    Q_D(VerticalNavigator);
    bool found = false;
    for (const NavigatorButton &button : d->buttons) {
        found = (button.id == pageId);
        if (found) {
            if (!button.hasFocus || !button.isVisible || button.isStretch || button.selected) {
                return;
            } else {
                break;
            }
        }
    }
    if (!found) {
        return;
    }
    d->setCurrentButton(pageId);
}

int VerticalNavigator::addButton(int pageId, const QString &text, const QString &on, const QString &off,
                                 const QString &error, bool hasFocus)
{
    Q_D(VerticalNavigator);
    Q_ASSERT(pageId != 0);
    for (const NavigatorButton &button : d->buttons) {
        Q_ASSERT_X(button.id != pageId, "VerticalNavigator::addButton()", "duplicated page id.");
    }
    NavigatorButton button;
    button.text = text;
    button.onIconPath = on;
    button.offIconPath = off;
    button.id = pageId;
    button.number = 0;
    button.hasFocus = hasFocus;
    button.on = QIcon(on);
    button.off = QIcon(off);
    button.error = QPixmap(error);
    int index = d->buttons.size();
    bool isNew = d->buttons.isEmpty();
    if (isNew) {
        button.selected = true;
    }
    d->buttons.append(button);
    if (isNew) {
        d->currentAnimationbackgroundRect = d->getRect(pageId);
    }
    return index;
}

void VerticalNavigator::updateButtonNumber(int pageId, int number)
{
    Q_D(VerticalNavigator);
    for (NavigatorButton &button : d->buttons) {
        if (button.id == pageId) {
            button.number = number;
            update();
            break;
        }
    }
}

void VerticalNavigator::updateButtonText(int pageId, const QString &text)
{
    Q_D(VerticalNavigator);
    for (NavigatorButton &button : d->buttons) {
        if (button.id == pageId) {
            button.text = text;
            update();
            break;
        }
    }
}

void VerticalNavigator::setMargins(const QMargins &margins)
{
    Q_D(VerticalNavigator);
    d->margins = margins;
    update();
}


QMargins VerticalNavigator::margins() const
{
    Q_D(const VerticalNavigator);
    return d->margins;
}

void VerticalNavigator::setPadding(int padding)
{
    Q_D(VerticalNavigator);
    d->padding = padding;
    update();
}

int VerticalNavigator::padding() const
{
    Q_D(const VerticalNavigator);
    return d->padding;
}

void VerticalNavigator::setIconSize(const QSize &iconSize)
{
    Q_D(VerticalNavigator);
    d->iconSize = iconSize;
    update();
}

QSize VerticalNavigator::iconSize() const
{
    Q_D(const VerticalNavigator);
    return d->iconSize;
}

int VerticalNavigator::addStretch()
{
    Q_D(VerticalNavigator);
    for (const NavigatorButton &button : d->buttons) {
        Q_ASSERT_X(!button.isStretch, "VerticalNavigator::addStretch()", "Only one stretch is allowed.");
    }
    NavigatorButton button;
    button.isStretch = true;
    button.hasFocus = false;
    int index = d->buttons.size();
    d->buttons.append(button);
    return index;
}

void VerticalNavigator::hideButton(int pageId)
{
    Q_D(VerticalNavigator);
    for (NavigatorButton &button : d->buttons) {
        if (button.id == pageId) {
            if (!button.isVisible || button.isStretch) {
                return;
            }
            button.isVisible = false;
            if (button.selected) {
                button.selected = false;
                for (NavigatorButton &t : d->buttons) {
                    if (t.id != pageId && t.isVisible && t.hasFocus && !t.isStretch) {
                        setCurrentButton(t.id);
                        return;
                    }
                }
                // FIXME what about all buttons are hidden?
            }
        }
    }
}

void VerticalNavigator::reloadIcon()
{
    Q_D(VerticalNavigator);
    for (NavigatorButton &button : d->buttons) {
        QIcon on(button.onIconPath);
        QIcon off(button.offIconPath);

        button.on = on;
        button.off = off;
    }
    update();
}

int VerticalNavigator::preferredHeight() const
{
    Q_D(const VerticalNavigator);

    if (d->buttons.isEmpty()) {
        return 0;
    }

    int h = 0;
    for (const NavigatorButton &button : d->buttons) {
        if (!button.isStretch) {
            h = d->getRect(button.id).height();
            break;
        }
    }
    return h * d->buttons.size() + d->padding * (d->buttons.size() - 1);
}

#include "moc_vertical_navigator.cpp"
