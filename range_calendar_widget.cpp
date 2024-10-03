#include <QtWidgets/QTableWidget>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QAction>
#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>
#include <QtGui/qscreen.h>
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#include <QtGui/qscreen.h>
#else
#include <QtWidgets/qdesktopwidget.h>
#endif
#include "range_calendar_widget.h"
#include "ui_range_calendar_widget.h"

RangeCalendarWidget::RangeCalendarWidget(QWidget *parent /*= nullptr*/)
    : QCalendarWidget(parent)
    , selectedBkColor(QColor("#CD0B00"))
    , selectedTextColor(Qt::white)
    , highlightBkColor(QColor("#EBF6FF"))
    , highlightTextColor(QColor("#016DC1"))
{
    setSelectionMode(SingleSelection);

    tableView = findChild<QWidget *>(QLatin1String("qt_calendar_calendarview"));
}

RangeCalendarWidget::~RangeCalendarWidget() { }

#include <QtCore/QDebug>
void RangeCalendarWidget::paintCell(QPainter *painter, const QRect &rect, const QDate &date) const
{
    const QString &text = QString::number(date.day());

    QPalette pal = tableView->palette();

    QTextCharFormat fmt;
    fmt.setFont(tableView->font());
    fmt.setBackground(pal.brush(QPalette::Base));
    fmt.setForeground(pal.brush(QPalette::Text));
    // fmt.merge(weekdayTextFormat(Qt::DayOfWeek(date.dayOfWeek())));

    const QDate &selectedDate = this->selectedDate();
    if (!selectedDate.isNull() && !anotherSelectedDate.isNull()) {
        if (date != selectedDate) {
            QDate start, end;
            if (anotherSelectedDate < selectedDate) {
                start = anotherSelectedDate;
                end = selectedDate;
            } else {
                start = selectedDate;
                end = anotherSelectedDate;
            }

            if (start <= date && date <= end) {
                painter->save();
                painter->setPen(Qt::NoPen);
                painter->fillRect(rect.adjusted(0, 2, 0, -2), highlightBkColor);

                painter->setFont(fmt.font());
                painter->setPen(highlightTextColor);
                painter->drawText(rect, Qt::AlignCenter, text);
                painter->restore();
                return;
            }
        } else {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->fillRect(rect, palette().window());

            if (selectedDate < anotherSelectedDate) {
                // paint right
                painter->fillRect(rect.adjusted(rect.width() >> 1, 2, 0, -2), highlightBkColor);
            } else if (selectedDate > anotherSelectedDate) {
                // paint left
                painter->fillRect(rect.adjusted(0, 2, -(rect.width() >> 1), -2), highlightBkColor);
            }

            painter->setBrush(selectedBkColor);
            QPoint center = rect.center();
            int d = qMin(rect.height(), rect.width());
            painter->drawEllipse(center, d >> 1, d >> 1);

            painter->setFont(fmt.font());
            painter->setPen(selectedTextColor);
            painter->drawText(rect, Qt::AlignCenter, text);
            painter->restore();
            return;
        }
    }

    painter->save();
    painter->setPen(Qt::NoPen);
    painter->fillRect(rect, fmt.background());

    painter->setFont(fmt.font());
    if (monthShown() == date.month()) {
        painter->setPen(fmt.foreground().color());
    } else {
        painter->setPen(pal.color(QPalette::Disabled, QPalette::Text));
    }

    painter->drawText(rect, Qt::AlignCenter, text);
    painter->restore();
}

RangeCalendarContainer::RangeCalendarContainer(QWidget *parent /*= nullptr*/, Qt::WindowFlags f /*= Qt::WindowFlags()*/)
    : QWidget(parent, f)
    , ui(new Ui::RangeCalendarContainer)
    , canceled(false)
{
    ui->setupUi(this);
    connect(ui->btnCancel, &QPushButton::clicked, this, [this] {
        canceled = true;
        hide();
    });
    connect(ui->btnYes, &QPushButton::clicked, this, &RangeCalendarContainer::emitDataChanged);

    connect(ui->btnLastDay, &QAbstractButton::clicked, this, [this] {
        const QDate &day = currentDate();
        setDateStart(day);
        setDateEnd(day);
        emitDataChanged();
    });
    connect(ui->btnLastWeek, &QAbstractButton::clicked, this, [this] {
        const QDate &day = currentDate();
        setDateStart(day.addDays(-6));
        setDateEnd(day);
        emitDataChanged();
    });
    connect(ui->btnLastFifteenDays, &QAbstractButton::clicked, this, [this] {
        const QDate &day = currentDate();
        setDateStart(day.addDays(-14));
        setDateEnd(day);
        emitDataChanged();
    });
    connect(ui->btnLastMonth, &QAbstractButton::clicked, this, [this] {
        const QDate &day = currentDate();
        setDateStart(day.addMonths(-1).addDays(1));
        setDateEnd(day);
        emitDataChanged();
    });
    connect(ui->btnLastThreeMonth, &QAbstractButton::clicked, this, [this] {
        const QDate &day = currentDate();
        setDateStart(day.addMonths(-3).addDays(1));
        setDateEnd(day);
        emitDataChanged();
    });

    connect(ui->calendarEnd, &QCalendarWidget::selectionChanged, this, [this]() {
        ui->calendarStart->anotherSelectedDate = ui->calendarEnd->selectedDate();
        ui->calendarStart->update();
        ui->calendarEnd->update();
    });
    connect(ui->calendarStart, &QCalendarWidget::selectionChanged, this, [this]() {
        ui->calendarEnd->anotherSelectedDate = ui->calendarStart->selectedDate();
        ui->calendarStart->update();
        ui->calendarEnd->update();
    });

    const QDate &day = currentDate();
    setDateStart(day);
    setDateEnd(day);
}

RangeCalendarContainer::~RangeCalendarContainer() { }

QDate RangeCalendarContainer::dateStart() const
{
    return ui->calendarStart->selectedDate();
}

QDate RangeCalendarContainer::dateEnd() const
{
    return ui->calendarEnd->selectedDate();
}

void RangeCalendarContainer::setDateStart(const QDate &start)
{
    ui->calendarStart->setSelectedDate(start);
    ui->calendarEnd->anotherSelectedDate = start;
    ui->calendarStart->update();
    ui->calendarEnd->update();
}

void RangeCalendarContainer::setDateEnd(const QDate &end)
{
    ui->calendarEnd->setSelectedDate(end);
    ui->calendarStart->anotherSelectedDate = end;
    ui->calendarStart->update();
    ui->calendarEnd->update();
}

void RangeCalendarContainer::setMinimumDate(const QDate &min)
{
    ui->calendarStart->setMinimumDate(min);
    ui->calendarEnd->setMinimumDate(min);
}

void RangeCalendarContainer::setMaximumDate(const QDate &max)
{
    ui->calendarStart->setMaximumDate(max);
    ui->calendarEnd->setMaximumDate(max);
}

QDate RangeCalendarContainer::currentDate() const
{
    if (redirectCurrentDate) {
        return redirectCurrentDate();
    }
    return QDate::currentDate();
}

void RangeCalendarContainer::setRedirectCurrentDateFunction(std::function<QDate()> func)
{
    redirectCurrentDate = func;
}

void RangeCalendarContainer::addToolBtn(const QString &text, std::function<void(QDate &dateStart, QDate &dateEnd)> func)
{
    QToolButton *btn = new QToolButton(this);
    btn->setText(text);
    if (func) {
        connect(btn, &QAbstractButton::clicked, this, [this, func] {
            QDate start, end;
            func(start, end);

            setDateStart(start);
            setDateEnd(end);
            emitDataChanged();
        });
    }
    ui->horizontalTopMore->addWidget(btn);
}

void RangeCalendarContainer::hideEvent(QHideEvent *event)
{
    if (canceled) {
        emit cancel();
    } else {
        emitDataChanged();
    }
    QWidget::hideEvent(event);
}

void RangeCalendarContainer::emitDataChanged()
{
    const QDate &start = dateStart();
    const QDate &end = dateEnd();
    if (start < end) {
        emit dateChanged(start, end);
    } else {
        emit dateChanged(end, start);
    }
}

class DateTimeRangeValidator : public QValidator
{
public:
    DateTimeRangeValidator(const QString &sep, RangeDateEdit *edit);
    virtual State validate(QString &input, int &pos) const;
public:
    const QString sep;
    QDate dateMax;
    QDate dateMin;
    bool onlyShowDays;

    RangeDateEdit *edit;
};

DateTimeRangeValidator::DateTimeRangeValidator(const QString &sep, RangeDateEdit *edit)
    : QValidator()
    , sep(sep)
    , onlyShowDays(false)
    , edit(edit)
{
}

QValidator::State DateTimeRangeValidator::validate(QString &input, int &) const
{
    if (onlyShowDays) {
        if (!edit->dateEnd().isValid()) {
            return QValidator::Invalid;
        }
        bool ok = false;
        int days = input.toInt(&ok);
        if (!ok) {
            return QValidator::Intermediate;
        }
        const QDate &start = edit->dateEnd().addDays(1 - days);
        if (!dateMin.isNull() && start < dateMin) {
            return QValidator::Intermediate;
        }
        return QValidator::Acceptable;
    }

    int mid = input.indexOf(sep);
    if (mid == -1) {
        return QValidator::Invalid;
    }
    const QString &left = input.left(mid);
    if (left.isEmpty()) {
        return QValidator::Invalid;
    }
    const QString &right = input.mid(mid + 1);
    if (right.isEmpty()) {
        return QValidator::Invalid;
    }
    const QDate &start = QDate::fromString(left, "yyyy-MM-dd");
    if (start.isNull()) {
        return QValidator::Intermediate;
    }
    const QDate &end = QDate::fromString(right, "yyyy-MM-dd");
    if (end.isNull()) {
        return QValidator::Intermediate;
    }
    if (start > end) {
        return QValidator::Intermediate;
    }
    if (!dateMax.isNull() && end > dateMax) {
        return QValidator::Intermediate;
    }
    if (!dateMin.isNull() && start < dateMin) {
        return QValidator::Intermediate;
    }
    return QValidator::Acceptable;
}

class RangeDateEditPrivate
{
public:
    RangeDateEditPrivate(RangeDateEdit *q);
    virtual ~RangeDateEditPrivate();
public:
    QString sep;
    QDate dateStart;
    QDate dateEnd;
    QDate dateMin;
    QDate dateMax;
    RangeCalendarContainer *calendar;
    QAction *actionPopCalendar;
    DateTimeRangeValidator *validator;
    std::function<QDate()> redirectCurrentDate;
    QList<QPair<QString, std::function<void(QDate &dateStart, QDate &dateEnd)>>> moreBtns;
    bool onlyShowDays;
protected:
    friend class DateTimeRangeValidator;
    RangeDateEdit * const q_ptr;
    Q_DECLARE_PUBLIC(RangeDateEdit)
};

RangeDateEditPrivate::RangeDateEditPrivate(RangeDateEdit *q)
    : sep(QString::fromUtf8("/"))
    , calendar(nullptr)
    , actionPopCalendar(new QAction(QApplication::style()->standardIcon(QStyle::StandardPixmap(QStyle::SP_TitleBarUnshadeButton)), QString(), q))
    , validator(new DateTimeRangeValidator(sep, q))
    , onlyShowDays(false)
    , q_ptr(q)
{
}

RangeDateEditPrivate::~RangeDateEditPrivate()
{
    delete validator;
    delete actionPopCalendar;
    if (calendar) {
        delete calendar;
        calendar = nullptr;
    }
}

RangeDateEdit::RangeDateEdit(QWidget *parent /*= nullptr*/)
    : QLineEdit(parent)
    , d_ptr(new RangeDateEditPrivate(this))
{
    init();
}

RangeDateEdit::~RangeDateEdit()
{
    delete d_ptr;
}

QDate RangeDateEdit::dateStart() const
{
    Q_D(const RangeDateEdit);
    return d->dateStart;
}

QDate RangeDateEdit::dateEnd() const
{
    Q_D(const RangeDateEdit);
    return d->dateEnd;
}

QDate RangeDateEdit::dateMinimum() const
{
    Q_D(const RangeDateEdit);
    return d->dateMin;
}

QDate RangeDateEdit::dateMaximum() const
{
    Q_D(const RangeDateEdit);
    return d->dateMax;
}

void RangeDateEdit::setDateStart(const QDate &start)
{
    Q_D(RangeDateEdit);
    if (d->dateStart != start) {
        d->dateStart = start;
        if (d->calendar) {
            d->calendar->setDateStart(start);
        }
        updateText();
        emit dateChanged(start, d->dateEnd);
    }
}

void RangeDateEdit::setDateEnd(const QDate &end)
{
    Q_D(RangeDateEdit);
    if (d->dateEnd != end) {
        d->dateEnd = end;
        if (d->calendar) {
            d->calendar->setDateEnd(end);
        }
        updateText();
        emit dateChanged(d->dateStart, end);
    }
}

void RangeDateEdit::setDate(const QDate &start, const QDate &end)
{
    Q_D(RangeDateEdit);
    if (d->dateStart != start || d->dateEnd != end) {
        if (!d->dateMin.isNull() && start < d->dateMin) {
            d->dateStart = d->dateMin;
        } else {
            d->dateStart = start;
        }
        if (!d->dateMax.isNull() && end > d->dateMax) {
            d->dateEnd = d->dateMax;
        } else {
            d->dateEnd = end;
        }
        if (d->calendar) {
            d->calendar->setDateStart(d->dateStart);
            d->calendar->setDateEnd(d->dateEnd);
        }
        updateText();
        emit dateChanged(d->dateStart, d->dateEnd);
    }
}

void RangeDateEdit::setRedirectCurrentDateFunction(std::function<QDate()> func)
{
    Q_D(RangeDateEdit);
    d->redirectCurrentDate = func;
    if (d->calendar) {
        d->calendar->setRedirectCurrentDateFunction(func);
    }
}

void RangeDateEdit::setMinimumDate(const QDate &min)
{
    Q_D(RangeDateEdit);
    d->dateMin = min;
    d->validator->dateMin = min;
    if (d->calendar) {
        d->calendar->setMinimumDate(min);
    }
}

void RangeDateEdit::setMaximumDate(const QDate &max)
{
    Q_D(RangeDateEdit);
    d->dateMax = max;
    d->validator->dateMax = max;
    if (d->calendar) {
        d->calendar->setMaximumDate(max);
    }
}

void RangeDateEdit::setDateRange(const QDate &min, const QDate &max)
{
    Q_D(RangeDateEdit);
    d->dateMin = min;
    d->validator->dateMin = min;
    d->dateMax = max;
    d->validator->dateMax = max;
    if (d->calendar) {
        d->calendar->setMinimumDate(min);
        d->calendar->setMaximumDate(max);
    }
}

void RangeDateEdit::setOnlyShowDays()
{
    Q_D(RangeDateEdit);
    d->onlyShowDays = true;
    d->validator->onlyShowDays = true;
    updateText();
}

void RangeDateEdit::addToolBtn(const QString &text, std::function<void(QDate &dateStart, QDate &dateEnd)> func)
{
    Q_D(RangeDateEdit);
    d->moreBtns.append(qMakePair(text, func));
}

QSize RangeDateEdit::sizeHint() const
{
    Q_D(const RangeDateEdit);
    const QFontMetrics &fm = fontMetrics();
    int w = fm.horizontalAdvance(d->onlyShowDays ? QString::number(10)
                                                 : QString::fromUtf8("2020-01-01%12020-01-01").arg(d->sep));
    int h = fontMetrics().ascent() + fontMetrics().descent();

    QStyleOptionFrame opt;
    initStyleOption(&opt);
    QSize sh(w + style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this), h);

    sh = style()->sizeFromContents(QStyle::CT_LineEdit, &opt, sh, this);
    return sh;
}

void RangeDateEdit::showPopup()
{
    Q_D(RangeDateEdit);
    if (!d->calendar) {
        // mac 模态窗口打开popup windows时，不设置窗口收不了焦点
        // void QCompleter::setPopup(QAbstractItemView *popup)中的popup没有设置Parent，也有这个问题
        d->calendar = new RangeCalendarContainer(
#ifdef Q_OS_MAC
                this,
#else
                nullptr,
#endif
                Qt::Popup);
        if (!d->calendar) {
            return;
        }
        if (!d->dateMin.isNull()) {
            d->calendar->setMinimumDate(d->dateMin);
        }
        if (!d->dateMax.isNull()) {
            d->calendar->setMaximumDate(d->dateMax);
        }
        d->calendar->setRedirectCurrentDateFunction(d->redirectCurrentDate);
        d->calendar->setDateStart(d->dateStart);
        d->calendar->setDateEnd(d->dateEnd);
        for (const QPair<QString, std::function<void(QDate & dateStart, QDate & dateEnd)>> &btn : d->moreBtns) {
            d->calendar->addToolBtn(btn.first, btn.second);
        }

        connect(d->calendar, &RangeCalendarContainer::dateChanged, this, &RangeDateEdit::calendarDateChanged);
        connect(d->calendar, &RangeCalendarContainer::cancel, this, &RangeDateEdit::hidePopup);
    }
    QRect rt(mapToGlobal(QPoint(0, height() + 2)), d->calendar->size());

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QScreen *screen = this->screen();
#else
    QScreen *screen = QApplication::screenAt(mapToGlobal(geometry().center()));
#endif
    if (screen) {
        QRect scRt = screen->geometry();
        if (rt.width() > scRt.width()) {
            rt.setWidth(scRt.width());
        }
        if (rt.height() > scRt.height()) {
            rt.setHeight(scRt.height());
        }
        if (rt.top() < scRt.top()) {
            rt.moveTop(scRt.top());
        } else if (rt.bottom() > scRt.bottom()) {
            rt.moveBottom(scRt.bottom());
        }
        if (rt.left() < scRt.left()) {
            rt.moveLeft(scRt.left());
        } else if (rt.right() > scRt.right()) {
            rt.moveRight(scRt.right());
        }
    }
    d->calendar->setGeometry(rt);
    d->calendar->raise();
    d->calendar->show();
    d->calendar->setFocus();
}

void RangeDateEdit::hidePopup()
{
    Q_D(RangeDateEdit);
    if (d->calendar) {
        d->calendar->deleteLater();
        d->calendar = nullptr;
    }
}

void RangeDateEdit::keyPressEvent(QKeyEvent *event)
{
    QLineEdit::keyPressEvent(event);
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        event->accept();
    }
}

void RangeDateEdit::calendarDateChanged(const QDate &start, const QDate &end)
{
    Q_D(RangeDateEdit);
    if (start <= end) {
        if (d->dateStart != start || d->dateEnd != end) {
            d->dateStart = start;
            d->dateEnd = end;

            updateText();
            emit dateChanged(start, end);
        }
    }
    hidePopup();
}

void RangeDateEdit::init()
{
    Q_D(RangeDateEdit);

    setValidator(d->validator);

    addAction(d->actionPopCalendar, QLineEdit::TrailingPosition);
    connect(d->actionPopCalendar, &QAction::triggered, this, &RangeDateEdit::showPopup);

    d->dateStart = d->dateEnd = QDate::currentDate();
    updateText();

    connect(this, &QLineEdit::editingFinished, this, &RangeDateEdit::editingFinished);
}

void RangeDateEdit::updateText()
{
    Q_D(RangeDateEdit);
    QString text;
    QString tip = QString::fromUtf8("%1%2%3").arg(d->dateStart.toString("yyyy-MM-dd"), d->sep,
                                                  d->dateEnd.toString("yyyy-MM-dd"));
    if (d->onlyShowDays) {
        text = QString::number(d->dateStart.daysTo(d->dateEnd) + 1);
    } else {
        text = tip;
    }

    setText(text);
    setToolTip(tip);
}

void RangeDateEdit::editingFinished()
{
    Q_D(RangeDateEdit);
    const QString &input = text();

    if (d->onlyShowDays) {
        if (!d->dateEnd.isValid()) {
            return;
        }
        bool ok = false;
        int days = input.toInt(&ok);
        if (!ok) {
            return;
        }
        const QDate &start = d->dateEnd.addDays(1 - days);
        if (!d->dateMin.isNull() && start < d->dateMin) {
            return;
        }
        if (d->dateStart != start) {
            d->dateStart = start;
            if (d->calendar) {
                d->calendar->setDateStart(start);
            }
            updateText();
            emit dateChanged(start, d->dateEnd);
        }
        return;
    }

    int mid = input.indexOf(d->sep);
    if (mid == -1) {
        return;
    }
    const QString &left = input.left(mid);
    if (left.isEmpty()) {
        return;
    }
    const QString &right = input.mid(mid + 1);
    if (right.isEmpty()) {
        return;
    }
    const QDate &start = QDate::fromString(left, "yyyy-MM-dd");
    if (start.isNull()) {
        return;
    }
    const QDate &end = QDate::fromString(right, "yyyy-MM-dd");
    if (end.isNull()) {
        return;
    }
    if (start > end) {
        return;
    }
    if (!d->dateMax.isNull() && end > d->dateMax) {
        return;
    }
    if (!d->dateMin.isNull() && start < d->dateMin) {
        return;
    }
    if (d->dateStart != start || d->dateEnd != end) {
        d->dateStart = start;
        d->dateEnd = end;
        if (d->calendar) {
            d->calendar->setDateStart(start);
            d->calendar->setDateEnd(end);
        }
        updateText();
        emit dateChanged(start, end);
    }
}
