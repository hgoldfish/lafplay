#include <QtCore/QVariant>
#include <QtCore/QStringListModel>
#include <QtGui/QScreen>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QListView>
#include "navigate_title.h"

class NavigateTitlePrivate : public QObject
{
public:
    NavigateTitlePrivate(NavigateTitle *q);
    ~NavigateTitlePrivate();
public:
    QChar separator;
    QString title;
    QSet<int> noHoverLevel;

    QHBoxLayout *layout;
    QWidget *popupWidget;
    QListView *popupView;
    QStringListModel *popupModel;

    void updatelayout();
    void btnClicked();
    void popupItemSelected(const QModelIndex &index);
protected:
    NavigateTitle * const q_ptr;
    Q_DECLARE_PUBLIC(NavigateTitle)
};

NavigateTitlePrivate::NavigateTitlePrivate(NavigateTitle *q)
    : separator(QLatin1Char('>'))
    , layout(new QHBoxLayout(q))
    , popupWidget(nullptr)
    , popupView(nullptr)
    , popupModel(nullptr)
    , q_ptr(q)
{
    layout->setContentsMargins(0, 0, 0, 0);
}

NavigateTitlePrivate::~NavigateTitlePrivate()
{
    delete layout;
    delete popupModel;
    delete popupView;
    delete popupWidget;
}

void NavigateTitlePrivate::updatelayout()
{
    Q_Q(NavigateTitle);
    // 这个写法是 Qt 文档里面推荐的。见 QLayoutItem *QLayout::takeAt(int index) 的函数文档。
    QLayoutItem *child;
    while ((child = layout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QStringList titles = title.split(separator, Qt::SkipEmptyParts);
    QFont f = q->font();
    for (int i = 0; i < titles.size(); i++) {
        const QString &t = titles.at(i).trimmed();
        QPushButton *btn = new QPushButton(i == titles.size() - 1 ? t : t + " " + separator, q);
        btn->setFont(f);
        btn->setProperty("title", t);
        if (noHoverLevel.contains(i)) {
            btn->setStyleSheet("QPushButton{border:none; padding:1px 3px 1px 3px;}");
        } else {
            btn->setStyleSheet("QPushButton{border:none; padding:1px 3px 1px 3px;}"
                               "QPushButton:hover{color:#FF7D00;}");
        }

        layout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, &NavigateTitlePrivate::btnClicked);
    }

    layout->addStretch();
}

void NavigateTitlePrivate::btnClicked()
{
    QPushButton *btn = dynamic_cast<QPushButton *>(sender());
    if (!btn) {
        return;
    }
    int count = layout->count();
    int index = -1;
    for (int i = count - 1; i >= 0; i--) {
        QLayoutItem *item = layout->itemAt(i);
        QWidget *w = item->widget();
        if (btn == w) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        return;
    }
    QStringList titles;
    for (int i = 0; i <= index; i++) {
        QLayoutItem *item = layout->itemAt(i);
        QWidget *w = item->widget();
        titles.append(w->property("title").toString());
    }
    Q_Q(NavigateTitle);
    emit q->clicked(index);
    emit q->clicked(titles);
}

void NavigateTitlePrivate::popupItemSelected(const QModelIndex &index)
{
    Q_Q(NavigateTitle);
    q->hidePopup();
    const QString &item = index.data(Qt::DisplayRole).toString();
    emit q->popupItemSelected(item);
}

NavigateTitle::NavigateTitle(QWidget *parent)
    : QWidget(parent)
    , dd_ptr(new NavigateTitlePrivate(this))
{
}

NavigateTitle::~NavigateTitle()
{
    delete dd_ptr;
}

void NavigateTitle::setSeparator(const QChar &c)
{
    Q_D(NavigateTitle);
    d->separator = c;
    d->updatelayout();
}

void NavigateTitle::setText(const QString &text, const QSet<int> &noHover /* = QSet<int>()*/)
{
    Q_D(NavigateTitle);
    if (d->title == text && d->noHoverLevel == noHover) {
        return;
    }
    d->title = text;
    d->noHoverLevel = noHover;
    d->updatelayout();
}

QString NavigateTitle::getText(int level) const
{
    Q_D(const NavigateTitle);
    int count = d->layout->count();
    if (level >= count - 1) {
        return QString();
    }
    QLayoutItem *item = d->layout->itemAt(level);
    QPushButton *btn = static_cast<QPushButton *>(item->widget());
    if (!btn) {
        return QString();
    }
    return btn->property("title").toString();
}

void NavigateTitle::showPopup(int level, const QStringList &items)
{
    Q_D(NavigateTitle);
    if (items.isEmpty()) {
        return;
    }
    int count = d->layout->count();
    if (level >= count - 1) {
        return;
    }
    QLayoutItem *item = d->layout->itemAt(level);
    QPushButton *btn = static_cast<QPushButton *>(item->widget());
    if (!btn) {
        return;
    }
    const QString &title = btn->property("title").toString();

    if (!d->popupModel) {
        d->popupModel = new QStringListModel();
        if (!d->popupModel) {
            return;
        }
    }
    if (!d->popupWidget) {
        d->popupWidget = new QWidget(this->window(), Qt::Popup);
        if (!d->popupWidget) {
            return;
        }
        QVBoxLayout *layout = new QVBoxLayout(d->popupWidget);
        layout->setSizeConstraint(QLayout::SetMaximumSize);
        layout->setContentsMargins(0, 0, 0, 0);
        d->popupView = new QListView(d->popupWidget);
        d->popupView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        d->popupView->setObjectName("navigateTitlePopupView");
        layout->addWidget(d->popupView);
        d->popupView->setModel(d->popupModel);

        connect(d->popupView, &QListView::clicked, d, &NavigateTitlePrivate::popupItemSelected);
    }
    d->popupModel->setStringList(items);
    d->popupWidget->show();

    QPoint below = mapToGlobal(btn->geometry().bottomLeft());
    below += QPoint(0, 2);

    int h = d->popupView->visualRect(d->popupModel->index(0)).height() * items.size();
    int w = 0;
    const QFontMetrics &fontMetrics = d->popupView->fontMetrics();

    for (int i = 0; i < items.size(); i++) {
        const QString &text = items.at(i);
        const int textWidth = fontMetrics.horizontalAdvance(text);
        w = qMax(w, textWidth);
    }

    QMargins margins = d->popupView->contentsMargins();
    w += margins.left() + margins.right() + 10;
    h += margins.top() + margins.bottom();

    QSize sz = QSize(w, qMin(h, 300));

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QScreen *screen = this->screen();
#else
    QScreen *screen = QApplication::screenAt(this->geometry().center());
#endif
    if (screen) {
        QRect rt = screen->geometry();

        if (below.y() + sz.height() > rt.bottom()) {
            below.setY(rt.bottom() - sz.height());
        }
        if (below.x() + sz.width() > rt.right()) {
            below.setX(rt.right() - sz.width());
        }
    }
    d->popupWidget->setGeometry(QRect(below, sz));
    int indexCurrent = items.indexOf(title);
    if (indexCurrent == -1) {
        // 尝试再查找下级？
        indexCurrent = items.indexOf(getText(level + 1));
    }
    if (indexCurrent != -1) {
        const QModelIndex &index = d->popupModel->index(indexCurrent);
        d->popupView->scrollTo(index);
        d->popupView->setCurrentIndex(index);
    } else if (items.count() > 0) {
        d->popupView->setCurrentIndex(d->popupModel->index(0));
    }
}

void NavigateTitle::hidePopup()
{
    Q_D(NavigateTitle);
    if (d->popupWidget) {
        d->popupWidget->hide();
    }
}
