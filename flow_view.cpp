#include <QtCore/qcoreevent.h>
#include <QtCore/qtimer.h>
#include <QtCore/qpropertyanimation.h>
#include <QtWidgets/qscrollbar.h>
#include "flow_view.h"

class LoadingTrigger : public QWidget
{
public:
    LoadingTrigger(QWidget *parent = nullptr)
        : QWidget(parent)
    {
    }
    virtual ~LoadingTrigger() override;

    virtual int heightForWidth(int) const override { return 1; }

    virtual bool hasHeightForWidth() const override { return true; }

    bool visible;
};

LoadingTrigger::~LoadingTrigger() { }

FlowView::FlowView(QWidget *parent)
    : QScrollArea(parent)
    , minimumColumnWidth(382)
    , viewMargins(8, 8, 14, 8)
    , cellPadding(8)
    , columnPadding(8)
    , cachedWidth(0)
{
    if (!widget()) {
        setWidget(new QWidget());
    }
    widget()->setAutoFillBackground(false);
    // 如果不设置成 ScrollBarAlwaysOn，那有可能导致一个
    // BUG，当滚动条出现时，条目按比例缩小，于是乎最底部也跟着缩小，此时又不需要滚动条。
    // 可是没有滚动条以后，条目又按比例放大。来回折腾，死循环了。所以这行语句特别重要。
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // connect(verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(checkLoadingTrigger()));
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(toggleWidgets()));
}

FlowView::~FlowView()
{
    delete loadingTrigger;
    qDeleteAll(boxes);
}

void FlowView::appendBox(QWidget *w)
{
    boxes.append(w);
}

void FlowView::clearBoxs()
{
    qDeleteAll(boxes);
    boxes.clear();
}

void FlowView::setMiniumColumnWidth(int w)
{
    minimumColumnWidth = w;
}

bool FlowView::viewportEvent(QEvent *event)
{
    bool success = QScrollArea::viewportEvent(event);
    if (event->type() == QEvent::Resize && cachedWidth != viewport()->width()) {
        schedulePosist();
    }
    return success;
}

void FlowView::scrollToTop()
{
    QScrollBar *vb = verticalScrollBar();
    if (vb) {
        vb->setValue(0);
    }
}

void FlowView::scrollToBottom()
{
    QScrollBar *vb = verticalScrollBar();
    if (vb) {
        vb->setValue(vb->maximum());
    }
}

class UpdateBox : public QObject
{
public:
    UpdateBox(QWidget *box)
        : box(box)
    {
    }
    void update()
    {
        if (!box.isNull() && box->isVisible()) {
            box->update();
        }
        deleteLater();
    }
    QPointer<QWidget> box;
};

void smoothlyMove(QWidget *box, const QRect &r)
{
    box->setProperty("dstGeometry", r);
    if (box->geometry() == r) {
        return;
    }
    if (box->isVisible()) {
        QList<QPropertyAnimation *> olds = box->findChildren<QPropertyAnimation *>();
        for (QPropertyAnimation *old : olds) {
            old->stop();
            old->deleteLater();
        }
        QPropertyAnimation *animation = new QPropertyAnimation(box);
        animation->setTargetObject(box);
        animation->setPropertyName("geometry");
        animation->setStartValue(box->geometry());
        animation->setEndValue(r);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
        QObject::connect(animation, &QPropertyAnimation::finished, new UpdateBox(box), &UpdateBox::update);
        QObject::connect(animation, &QPropertyAnimation::finished, animation, &QObject::deleteLater);
    } else {
        box->setGeometry(r);
    }
}

void FlowView::posist()
{
    QList<int> columns;
    cachedWidth = viewport()->width();
    int availbleWidth = cachedWidth - viewMargins.left() - viewMargins.right();
    int n = availbleWidth / minimumColumnWidth;
    if (n < 1) {
        n = 1;
    }
    for (int i = 0; i < n; ++i) {
        columns.append(viewMargins.top());
    }

    int cellWidth = (availbleWidth - columnPadding * (columns.size() - 1)) / columns.size();
    int viewportHeight = viewMargins.top();

    QList<QWidget *> boxes = this->boxes;
    if (loadingTrigger.isNull()) {
        LoadingTrigger *t = new LoadingTrigger(widget());
        t->visible = true;
        loadingTrigger = t;
    }
    boxes.append(loadingTrigger);

    for (int i = 0; i < boxes.size(); ++i) {
        QWidget *box = boxes[i];
        int col = 0;
        for (int j = 1; j < columns.size(); ++j) {
            if (columns[col] > columns[j]) {
                col = j;
            }
        }
        QRect r;
        r.setTop(columns[col]);
        r.setLeft(viewMargins.left() + cellWidth * col + columnPadding * col);
        int cellHeight;
        if (box->hasHeightForWidth()) {
            cellHeight = box->heightForWidth(cellWidth);
        } else {
            cellHeight = 100;
        }
        r.setHeight(cellHeight);
        r.setWidth(cellWidth);
        smoothlyMove(box, r);
        columns[col] += (cellPadding + cellHeight);
        if (viewportHeight < columns[col]) {
            viewportHeight = columns[col];
        }
    }

    widget()->resize(cachedWidth, viewportHeight + viewMargins.bottom());
    toggleWidgets();
}

void FlowView::schedulePosist()
{
    QTimer::singleShot(0, this, SLOT(posist()));
}

void FlowView::checkLoadingTrigger()
{
    if (loadingTrigger.isNull()) {
        return;
    }
    bool visible = isLoadingTriggerVisible();
    LoadingTrigger *t = static_cast<LoadingTrigger *>(loadingTrigger.data());
    if (!t->visible && visible) {
        emit requireLoading();
    }
    t->visible = visible;
}

void FlowView::toggleWidgets()
{
    QRect viewportRect;
    viewportRect.setX(0);
    viewportRect.setY(verticalScrollBar()->value());
    viewportRect.setWidth(viewport()->width());
    viewportRect.setHeight(viewport()->height());
    for (QWidget *box : boxes) {
        // 防止box正在animation动作中而判断出错
        QRect rt = box->property("dstGeometry").toRect();
        if (viewportRect.intersects(rt)) {
            if (!box->isVisible()) {
                box->show();
            }
        } else {
            if (box->isVisible()) {
                box->hide();
            }
        }
    }
    if (isLoadingTriggerVisible()) {
        emit requireLoading();
    }
}

bool FlowView::isLoadingTriggerVisible()
{
    if (loadingTrigger.isNull()) {
        return false;
    }
    QRect r = loadingTrigger->geometry();
    QScrollBar *vb = verticalScrollBar();
    return r.top() > vb->value() && r.bottom() < (vb->value() + viewport()->height());
}

OneFlowView::OneFlowView(QWidget *parent)
    : QScrollArea(parent)
    , viewMargins(8, 8, 14, 8)
    , cellPadding(8)
    , cachedWidth(0)
{
    if (!widget()) {
        setWidget(new QWidget());
    }
    widget()->setAutoFillBackground(false);
    // setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), SLOT(toggleWidgets()));
}

OneFlowView::~OneFlowView()
{
    qDeleteAll(boxes);
}

bool OneFlowView::viewportEvent(QEvent *event)
{
    bool success = QScrollArea::viewportEvent(event);
    if (event->type() == QEvent::Resize && cachedWidth != viewport()->width()) {
        QTimer::singleShot(0, this, SLOT(posist()));
    }
    return success;
}

void OneFlowView::scrollToTop()
{
    QScrollBar *vb = verticalScrollBar();
    if (vb) {
        vb->setValue(0);
    }
}

void OneFlowView::scrollToBottom()
{
    QScrollBar *vb = verticalScrollBar();
    if (vb) {
        vb->setValue(vb->maximum());
    }
}

void OneFlowView::posist()
{
    cachedWidth = viewport()->width();

    int cellWidth = cachedWidth - viewMargins.left() - viewMargins.right();
    int viewportHeight = viewMargins.top();

    for (int i = 0; i < boxes.size(); ++i) {
        QRect r;
        r.setTop(viewportHeight);
        r.setLeft(viewMargins.left());
        int cellHeight;
        if (boxes[i]->hasHeightForWidth()) {
            cellHeight = boxes[i]->heightForWidth(cellWidth);
        } else {
            cellHeight = 100;
        }
        r.setHeight(cellHeight);
        r.setWidth(cellWidth);
        smoothlyMove(boxes[i], r);
        viewportHeight += cellPadding + cellHeight;
    }

    widget()->resize(cachedWidth, viewportHeight + viewMargins.bottom());
    toggleWidgets();
}

void OneFlowView::schedulePosist()
{
    QTimer::singleShot(0, this, SLOT(posist()));
}

void OneFlowView::toggleWidgets()
{
    QRect viewportRect;
    viewportRect.setX(0);
    viewportRect.setY(verticalScrollBar()->value());
    viewportRect.setWidth(viewport()->width());
    viewportRect.setHeight(viewport()->height());
    for (QWidget *box : boxes) {
        QRect rt = box->property("dstGeometry").toRect();
        if (viewportRect.intersects(rt)) {
            if (!box->isVisible()) {
                box->show();
            }
        } else {
            if (box->isVisible()) {
                box->hide();
            }
        }
    }
}
