#ifndef LAFPLAY_NAVIGATETITLE_H
#define LAFPLAY_NAVIGATETITLE_H

#include <QtWidgets/QWidget>
#include <QtCore/QSet>

class NavigateTitlePrivate;
class NavigateTitle : public QWidget
{
    Q_OBJECT
public:
    explicit NavigateTitle(QWidget *parent = nullptr);
    virtual ~NavigateTitle();

    void setSeparator(const QChar &c);
    void setText(const QString &text, const QSet<int> &noHover = QSet<int>());
    QString getText(int level) const;

    void showPopup(int level, const QStringList &items);
    void hidePopup();
signals:
    // level start from 0;
    void clicked(int level);
    void clicked(const QStringList &titles);
    void popupItemSelected(const QString &item);
private:
    NavigateTitlePrivate * const dd_ptr;
    Q_DECLARE_PRIVATE_D(dd_ptr, NavigateTitle)
};

#endif  // LAFPLAY_NAVIGATETITLE_H
