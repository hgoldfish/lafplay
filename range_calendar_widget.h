#ifndef LAFPLAY_RANGECALENDARWIDGET_H
#define LAFPLAY_RANGECALENDARWIDGET_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCalendarWidget>

namespace Ui {
class RangeCalendarContainer;
}

class RangeCalendarWidget : public QCalendarWidget
{
    Q_OBJECT
public:
    explicit RangeCalendarWidget(QWidget *parent = nullptr);
    ~RangeCalendarWidget();
public:
    virtual void paintCell(QPainter *painter, const QRect &rect, const QDate &date) const override;

    QWidget *tableView;
    QDate anotherSelectedDate;
    QColor selectedBkColor;
    QColor selectedTextColor;
    QColor highlightBkColor;
    QColor highlightTextColor;
};

class RangeCalendarContainer : public QWidget
{
    Q_OBJECT
public:
    explicit RangeCalendarContainer(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~RangeCalendarContainer();

signals:
    void dateChanged(const QDate &start, const QDate &end);
    void cancel();
public:
    QDate dateStart() const;
    QDate dateEnd() const;

    void setDateStart(const QDate &start);
    void setDateEnd(const QDate &end);
    void setMinimumDate(const QDate &min);
    void setMaximumDate(const QDate &max);

    QDate currentDate() const;
    void setRedirectCurrentDateFunction(std::function<QDate()> func);
    void addToolBtn(const QString &text, std::function<void(QDate &dateStart, QDate &dateEnd)> func);
protected:
    virtual void hideEvent(QHideEvent *event) override;

    void emitDataChanged();
protected:
    Ui::RangeCalendarContainer *ui;
    std::function<QDate()> redirectCurrentDate;
    bool canceled;
};

class RangeDateEditPrivate;
class RangeDateEdit : public QLineEdit
{
    Q_OBJECT

    Q_PROPERTY(QDate dateStart READ dateStart WRITE setDateStart NOTIFY dateChanged)
    Q_PROPERTY(QDate dateEnd READ dateEnd WRITE setDateEnd NOTIFY dateChanged)
public:
    explicit RangeDateEdit(QWidget *parent = nullptr);
    ~RangeDateEdit();

    QDate dateStart() const;
    QDate dateEnd() const;

    QDate dateMinimum() const;
    QDate dateMaximum() const;

    void setDateStart(const QDate &start);
    void setDateEnd(const QDate &end);
    void setDate(const QDate &start, const QDate &end);
    void setRedirectCurrentDateFunction(std::function<QDate()> func);
    void setMinimumDate(const QDate &min);
    void setMaximumDate(const QDate &max);
    void setDateRange(const QDate &min, const QDate &max);
    void setOnlyShowDays();

    void addToolBtn(const QString &text, std::function<void(QDate &dateStart, QDate &dateEnd)> func);
signals:
    void dateChanged(const QDate &start, const QDate &end);
public:
    QSize sizeHint() const override;
protected:
    void showPopup();
    void hidePopup();

    void keyPressEvent(QKeyEvent *event) override;
private:
    void calendarDateChanged(const QDate &start, const QDate &end);
    void init();
    void updateText();
    void editingFinished();
protected:
    RangeDateEditPrivate * const d_ptr;
    Q_DECLARE_PRIVATE(RangeDateEdit)
};

#endif  // LAFPLAY_RANGECALENDARWIDGET_H
