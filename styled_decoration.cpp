#include "styled_decoration.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPalette>
#include <QResizeEvent>
#include <QStylePainter>
#include <QTimer>

class StyledDecorationEventFilter : public QObject
{
public:
    StyledDecorationEventFilter(StyledDecoration *styledDecoration) { this->styledDecoration = styledDecoration; }

    virtual bool eventFilter(QObject *obj, QEvent *event)
    {
        // styledDecoration always be deleted after childWindow,
        // so we can use it safely here.
        switch (event->type()) {
        case QEvent::ShowToParent:
            QTimer::singleShot(0, styledDecoration, SLOT(show()));
            break;
        case QEvent::HideToParent:
            QTimer::singleShot(0, styledDecoration, SLOT(hide()));
            break;
        case QEvent::WindowTitleChange:
            QTimer::singleShot(0, styledDecoration, SLOT(update()));
            styledDecoration->setWindowTitle(qobject_cast<QWidget *>(obj)->windowTitle());
            break;
        case QEvent::WindowIconChange:
            QTimer::singleShot(0, styledDecoration, SLOT(update()));
            break;
        case QEvent::Enter:
            styledDecoration->unsetCursor();
            break;
        default:
            break;
        }
        return QObject::eventFilter(obj, event);
    }
private:
    StyledDecoration *styledDecoration;
};

Interaction::Interaction(QObject *child, QObject *parent)
    : QObject(parent)
{
    this->child = child;
    connect(child, SIGNAL(destroyed()), SLOT(clear()));
}

Interaction::~Interaction()
{
    if (child)
        delete child;
}

void Interaction::clear()
{
    this->child = NULL;
    deleteLater();
}

StyledDecoration::StyledDecoration(QWidget *childWindow)
    : myWindowState(Qt::WindowNoState)
    , activeSubControl(QStyle::SC_None)
    , hoveredSubControl(QStyle::SC_None)
    , resizeAt(Nowhere)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    setMouseTracking(true);
    setProperty("I_am_styled_decoreation", true);

    this->childWindow = childWindow;
    setGeometry(childWindow->geometry());
    setWindowTitle(childWindow->windowTitle());
    setWindowIcon(childWindow->windowIcon());
    setWindowOpacity(childWindow->windowOpacity());
    if (childWindow->windowFlags() & Qt::WindowStaysOnTopHint) {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    }
    bool isDialog = ((childWindow->windowFlags() & Qt::Dialog) == Qt::Dialog);
    if (isDialog && !childWindow->parentWidget()) {
        setWindowModality(Qt::ApplicationModal);
    } else if (isDialog && childWindow->parentWidget()) {
        setWindowModality(Qt::WindowModal);
        if (childWindow->parentWidget()->windowFlags() & Qt::WindowStaysOnTopHint) {
            setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        }
        // setParent(childWindow->parentWidget(), childWindow->windowFlags());
        setParent(childWindow->parentWidget(), Qt::Dialog);
    }

    // 处理依赖关系，使得当childWindow被删除的时候，StyledDecoration也同时被删除
    // 同时，如果childWindow的拥有者是childWindow->parent()，也要在parent()销毁时同时销毁childWindow
    // 这里是与Python版本唯一的区别。
    if (childWindow->parent()) {
        new Interaction(childWindow, childWindow->parent());
    }
    childWindow->setParent(this);
    connect(childWindow, SIGNAL(destroyed()), SLOT(deleteMyself()));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(childWindow);
    positChild();

    childWindow->installEventFilter(new StyledDecorationEventFilter(this));
}

StyledDecoration::~StyledDecoration() { }

void StyledDecoration::deleteMyself()
{
    childWindow = nullptr;
    deleteLater();
}

void StyledDecoration::installTo(QWidget *childWindow)
{
    new StyledDecoration(childWindow);
}

void StyledDecoration::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (!(myWindowState & Qt::WindowMaximized) && tryToResize(event->pos())) {
            event->accept();
            return;
        }
        QStyle::SubControl control = getSubControl(event->pos());
        if (control != activeSubControl) {
            activeSubControl = control;
            update();
            event->accept();
        }
        if (control == QStyle::SC_TitleBarLabel && !(myWindowState & Qt::WindowMaximized)) {
            beginMovingPosition = mapToGlobal(event->pos());
            event->accept();
        }
    }
    QWidget::mousePressEvent(event);
}

void StyledDecoration::mouseReleaseEvent(QMouseEvent *event)
{
    resizeAt = Nowhere;
    if (event->button() == Qt::LeftButton) {
        event->accept();
        beginMovingPosition = QPoint();
        if (activeSubControl != QStyle::SC_None) {
            activeSubControl = QStyle::SC_None;
            update();
        }
        QStyle::SubControl control = getSubControl(event->pos());
        if (control == QStyle::SC_TitleBarMaxButton) {
            setMaximizedMode();
            update();
        } else if (control == QStyle::SC_TitleBarMinButton) {
            setWindowState(windowState() | Qt::WindowMinimized);
        } else if (control == QStyle::SC_TitleBarCloseButton) {
            close();
        } else if (control == QStyle::SC_TitleBarNormalButton) {
            setNormalMode();
            update();
        } else if (control == QStyle::SC_TitleBarSysMenu) {
            QPoint pos = mapToGlobal(layout()->contentsRect().topLeft());
            runSystemMenu(pos);
        } else {
            event->ignore();
            QWidget::mouseReleaseEvent(event);
            return;
        }
    }
    if (event->button() & Qt::RightButton) {
        QStyle::SubControl control = getSubControl(event->pos());
        if (control == QStyle::SC_TitleBarLabel) {
            runSystemMenu(mapToGlobal(event->pos()));
            event->accept();
        }
    }
    QWidget::mouseReleaseEvent(event);
}

void StyledDecoration::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() & Qt::LeftButton) {
        QStyle::SubControl control = getSubControl(event->pos());
        if (control == QStyle::SC_TitleBarLabel) {
            if (myWindowState & Qt::WindowMaximized) {
                setNormalMode();
            } else {
                setMaximizedMode();
            }
            update();
            event->accept();
            return;
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

void StyledDecoration::mouseMoveEvent(QMouseEvent *event)
{
    if (!(myWindowState & Qt::WindowMaximized)) {
        if (resizeAt != Nowhere) {
            resizeMe(event->pos());
            QWidget::mouseMoveEvent(event);
            return;
        } else {
            if (showResizeCursor(event->pos())) {
                QWidget::mouseMoveEvent(event);
                return;
            }
        }
    }
    if (!beginMovingPosition.isNull()) {
        QRect rect = geometry();
        QPoint currentPosition = mapToGlobal(event->pos());
        rect.translate(currentPosition - beginMovingPosition);
        setGeometry(rect);
        beginMovingPosition = currentPosition;
    }
    QStyle::SubControl control = getSubControl(event->pos());
    if (control != hoveredSubControl) {
        hoveredSubControl = control;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void StyledDecoration::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateMask();
}

void StyledDecoration::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStylePainter painter(this);
    if (!(myWindowState & Qt::WindowMaximized)) {
        QStyleOptionFrame frameOption;
        frameOption.initFrom(this);
        frameOption.lineWidth = style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, &frameOption, this) / 2;
        if (isActiveWindow()) {
            frameOption.state |= QStyle::State_Active;
        } else {
            frameOption.state ^= QStyle::State_Active;
        }
        painter.drawPrimitive(QStyle::PE_FrameWindow, frameOption);
    }
    QStyleOptionTitleBar option = getTitleBarOption();
    painter.drawComplexControl(QStyle::CC_TitleBar, option);
}

void StyledDecoration::closeEvent(QCloseEvent *event)
{
    if (childWindow) {
        QCoreApplication::sendEvent(childWindow, event);
        if (!event->isAccepted()) {
            return;
        }
    }
    QWidget::closeEvent(event);
}

bool StyledDecoration::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::StyleChange:
        updateMask();
        positChild();
        update();
        break;
    case QEvent::WindowIconChange:
    case QEvent::PaletteChange:
    case QEvent::FontChange:
        update();
        break;
    case QEvent::WindowStateChange:
        if (childWindow) {
            QCoreApplication::sendEvent(childWindow, event);
        }
        break;
    default:
        break;
    }
    return QWidget::event(event);
}

void StyledDecoration::runSystemMenu(const QPoint &globalPosition)
{
    QMenu menu;
    QIcon iconRestore, iconMinimize, iconMaximize, iconClose;
    QAction *actionRestore, *actionMinimize, *actionMaximize, *actionStayOnTop, *actionClose;
    iconRestore = style()->standardIcon(QStyle::SP_TitleBarNormalButton, NULL, NULL);
    iconMinimize = style()->standardIcon(QStyle::SP_TitleBarMinButton, NULL, NULL);
    iconMaximize = style()->standardIcon(QStyle::SP_TitleBarMaxButton, NULL, NULL);
    iconClose = style()->standardIcon(QStyle::SP_TitleBarCloseButton, NULL, NULL);
    actionRestore = menu.addAction(iconRestore, tr("&Restore"));
    actionRestore->setEnabled(myWindowState & Qt::WindowMaximized);
    actionMinimize = menu.addAction(iconMinimize, tr("Mi&nimize"));
    actionMinimize->setDisabled(windowState() & Qt::WindowMinimized);
    actionMaximize = menu.addAction(iconMaximize, tr("Ma&ximize"));
    actionMaximize->setDisabled(myWindowState & Qt::WindowMaximized);
    actionStayOnTop = menu.addAction(tr("Alway On &Top"));
    actionStayOnTop->setCheckable(true);
    actionStayOnTop->setChecked(windowFlags() & Qt::WindowStaysOnTopHint);
    menu.addSeparator();
    actionClose = menu.addAction(iconClose, tr("&Close"));
    actionClose->setShortcuts(QKeySequence::Close);
    QAction *action = menu.exec(globalPosition);
    if (action == actionRestore) {
        setNormalMode();
        update();
    } else if (action == actionMinimize) {
        setWindowState(windowState() | Qt::WindowMinimized);
    } else if (action == actionMaximize) {
        setMaximizedMode();
        update();
    } else if (action == actionStayOnTop) {
        setWindowFlags(windowFlags() ^ Qt::WindowStaysOnTopHint);
        show();
    } else if (action == actionClose) {
        close();
    }
}

QStyle::SubControl StyledDecoration::getSubControl(const QPoint &pos) const
{
    QStyleOptionTitleBar option = getTitleBarOption();
    return style()->hitTestComplexControl(QStyle::CC_TitleBar, &option, pos, this);
}

QStyleOptionTitleBar StyledDecoration::getTitleBarOption() const
{
    QStyleOptionTitleBar option;
    option.initFrom(this);
    option.titleBarFlags = Qt::Window | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint
            | Qt::WindowSystemMenuHint;
    option.titleBarState = myWindowState;
    int titleBarHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight, &option, this);
    bool noTitleBar = style()->styleHint(QStyle::SH_TitleBar_NoBorder, NULL, this);
    int borderWidth = style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, NULL, this) / 2;
    if (!noTitleBar && !(myWindowState & Qt::WindowMaximized)) {
        option.rect = QRect(borderWidth, borderWidth, width() - 2 * borderWidth, titleBarHeight);
    } else {
        option.rect = QRect(0, 0, width(), titleBarHeight);
    }
    option.subControls = QStyle::SC_All;
    if (activeSubControl != QStyle::SC_None) {
        option.activeSubControls = activeSubControl;
        option.state |= QStyle::State_Sunken;
    } else if (hoveredSubControl != QStyle::SC_None) {
        option.activeSubControls = hoveredSubControl;
        option.state |= QStyle::State_MouseOver;
    } else {
        option.activeSubControls = QStyle::SC_None;
        option.state ^= QStyle::State_MouseOver;
    }
    if (childWindow) {
        if (childWindow->property("_dolphin_no_qtcurve_icon").isValid()
            && style()->objectName() == QString::fromLatin1("qtcurve")) {
            option.icon = QIcon();
        } else {
            option.icon = childWindow->windowIcon();
        }
        option.text = childWindow->windowTitle();
    } else {
        option.icon = QIcon();
        option.text = QApplication::applicationName();
    }
    if (option.icon.isNull()) {
        option.icon = style()->standardIcon(QStyle::SP_TitleBarMenuButton, NULL, this);
    }
    if (option.text != "") {
        int labelWidth = style()->subControlRect(QStyle::CC_TitleBar, &option, QStyle::SC_TitleBarLabel, this).width();
        option.text = option.fontMetrics.elidedText(option.text, Qt::ElideRight, labelWidth - 20);
    }
    if (isActiveWindow()) {
        option.palette.setCurrentColorGroup(QPalette::Active);
        option.state |= QStyle::State_Active;
        option.titleBarState |= QStyle::State_Active;
    } else {
        option.palette.setCurrentColorGroup(QPalette::Inactive);
        option.state ^= QStyle::State_Active;
    }
    return option;
}

void StyledDecoration::setNormalMode()
{
    myWindowState = Qt::WindowNoState;
    if (originalGeometry.isValid()) {
        setGeometry(originalGeometry);
        originalGeometry = QRect();
        updateMask();
        positChild();
    }
}

void StyledDecoration::setMaximizedMode()
{
    originalGeometry = geometry();
    myWindowState = Qt::WindowMaximized;
    setGeometry(QApplication::desktop()->availableGeometry(this));
    updateMask();
    positChild();
}

void StyledDecoration::updateMask()
{
    QStyleOptionFrame frameOption;
    QStyleHintReturnMask frameMask;
    frameOption.initFrom(this);
    frameOption.lineWidth = style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, &frameOption, this) / 2;
    style()->styleHint(QStyle::SH_WindowFrame_Mask, &frameOption, this, &frameMask);
    setMask(frameMask.region);
}

bool StyledDecoration::showResizeCursor(const QPoint &pos)
{
    QRect r = rect();
    int frameWidth = style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, NULL, this) / 2;
    if (frameWidth < 2) {
        frameWidth = 2;
    }
    if (0 <= pos.x() && pos.x() <= frameWidth && 0 <= pos.y() && pos.y() <= frameWidth) {
        setCursor(Qt::SizeFDiagCursor);
    } else if (0 <= pos.x() && pos.x() <= frameWidth && (r.height() - frameWidth) <= pos.y() && pos.y() <= r.height()) {
        setCursor(Qt::SizeBDiagCursor);
    } else if ((r.width() - frameWidth) <= pos.x() && pos.x() <= r.width() && 0 <= pos.y() && pos.y() <= frameWidth) {
        setCursor(Qt::SizeBDiagCursor);
    } else if ((r.width() - frameWidth) <= pos.x() && pos.x() <= r.width() && (r.height() - frameWidth) <= pos.y()
               && pos.y() <= r.height()) {
        setCursor(Qt::SizeFDiagCursor);
    } else if (0 <= pos.x() && pos.x() <= frameWidth) {
        setCursor(Qt::SizeHorCursor);
    } else if (0 <= pos.y() && pos.y() <= frameWidth) {
        setCursor(Qt::SizeVerCursor);
    } else if ((r.width() - frameWidth) <= pos.x() && pos.x() <= r.width()) {
        setCursor(Qt::SizeHorCursor);
    } else if ((r.height() - frameWidth) <= pos.y() && pos.y() <= r.height()) {
        setCursor(Qt::SizeVerCursor);
    } else {
        unsetCursor();
        return false;
    }
    return true;
}

bool StyledDecoration::tryToResize(const QPoint &pos)
{
    QRect r = rect();
    int frameWidth = style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, NULL, this) / 2;
    if (frameWidth < 2)
        frameWidth = 2;
    if (0 <= pos.x() && pos.x() <= frameWidth && 0 <= pos.y() && pos.y() <= frameWidth) {
        resizeAt = TopLeft;
    } else if (0 <= pos.x() && pos.x() <= frameWidth && (r.height() - frameWidth) <= pos.y() && pos.y() <= r.height()) {
        resizeAt = BottomLeft;
    } else if ((r.width() - frameWidth) <= pos.x() && pos.x() <= r.width() && 0 <= pos.y() && pos.y() <= frameWidth) {
        resizeAt = TopRight;
    } else if ((r.width() - frameWidth) <= pos.x() && pos.x() <= r.width() && (r.height() - frameWidth) <= pos.y()
               && pos.y() <= r.height()) {
        resizeAt = BottomRight;
    } else if (0 <= pos.x() && pos.x() <= frameWidth) {
        resizeAt = Left;
    } else if (0 <= pos.y() && pos.y() <= frameWidth) {
        resizeAt = Top;
    } else if ((r.width() - frameWidth) <= pos.x() && pos.x() <= r.width()) {
        resizeAt = Right;
    } else if ((r.height() - frameWidth) <= pos.y() && pos.y() <= r.height()) {
        resizeAt = Bottom;
    } else {
        resizeAt = Nowhere;
        return false;
    }
    return true;
}

void StyledDecoration::resizeMe(const QPoint &pos0)
{
    QPoint pos = mapToGlobal(pos0);
    QRect r = geometry();
    QRect r0(r);
    QSize minSize;
    if (childWindow) {
        minSize = childWindow->minimumSizeHint();
        if (minSize.height() < 150) {
            minSize.setHeight(150);
        }
        if (minSize.width() < 200) {
            minSize.setWidth(200);
        }
    } else {
        minSize = QSize(200, 150);
    }
    if (resizeAt == TopLeft || resizeAt == TopRight || resizeAt == Top) {
        r.setTop(pos.y());
        if (r.height() < minSize.height()) {
            r.setTop(r0.top());
        }
    }
    if (resizeAt == TopLeft || resizeAt == BottomLeft || resizeAt == Left) {
        r.setLeft(pos.x());
        if (r.width() < minSize.width()) {
            r.setLeft(r0.left());
        }
    }
    if (resizeAt == BottomLeft || resizeAt == BottomRight || resizeAt == Bottom) {
        r.setBottom(pos.y());
        if (r.height() < minSize.height()) {
            r.setBottom(r0.bottom());
        }
    }
    if (resizeAt == TopRight || resizeAt == BottomRight || resizeAt == Right) {
        r.setRight(pos.x());
        if (r.width() < minSize.width()) {
            r.setRight(r0.right());
        }
    }
    setGeometry(r);
}

void StyledDecoration::positChild()
{
    int titleBarHeight, frameWidth;
    QMargins m;
    titleBarHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight, NULL, this);
    m.setTop(titleBarHeight);
    if (!(myWindowState & Qt::WindowMaximized)) {
        frameWidth = style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, NULL, this) / 2;
        if (frameWidth < 2) {
            frameWidth = 2;
        }
        m.setLeft(frameWidth);
        m.setRight(frameWidth);
        m.setBottom(frameWidth);
    }
    layout()->setContentsMargins(m);
}
