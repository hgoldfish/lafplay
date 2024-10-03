#ifndef LAFPLAY_VERTICALNAVIGATOR_H
#define LAFPLAY_VERTICALNAVIGATOR_H

#include <QtGui/qicon.h>
#include <QtWidgets/qframe.h>

class VerticalNavigatorPrivate;
class VerticalNavigator : public QFrame
{
    Q_OBJECT
public:
    explicit VerticalNavigator(QWidget *parent = nullptr);
    virtual ~VerticalNavigator() override;
signals:
    void triggered(int pageId);
protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
public:
    int addButton(int pageId, const QString &text, const QString &on, const QString &off, const QString &error, bool hasFocus);
    void updateButtonNumber(int pageId, int number);
    void updateButtonText(int pageId, const QString &text);
    void setMargins(const QMargins &margins);
    QMargins margins() const;
    void setPadding(int padding);
    int padding() const;
    void setIconSize(const QSize &iconSize);
    QSize iconSize() const;
    int addStretch();
    int currentButton() const;
    void setCurrentButton(int pageId);
    void hideButton(int pageId);
    void reloadIcon();
    int preferredHeight() const;
private:
    VerticalNavigatorPrivate * const dd_ptr;
    Q_DECLARE_PRIVATE_D(dd_ptr, VerticalNavigator)
    Q_PRIVATE_SLOT(d_func(), void _q_doAnimation(const QVariant &value))
};

#endif  // LAFPLAY_VERTICALNAVIGATOR_H
