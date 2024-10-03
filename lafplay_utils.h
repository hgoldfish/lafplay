#ifndef LAFPLAY_UTILS_H
#define LAFPLAY_UTILS_H

#include <QtCore/qdatetime.h>
#include <QtCore/qsettings.h>
#include <QtWidgets/qdialog.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlineedit.h>

class CtrlEnterFilter : public QObject
{
public:
    CtrlEnterFilter(QDialog *parent)
        : QObject(parent)
    {
    }
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

class WaterMask : public QWidget
{
public:
    WaterMask(const QString &text, bool tiled = false, QWidget *parent = nullptr);
public:
    void posist();
protected:
    virtual void paintEvent(QPaintEvent *event) override;
public:
    const QString text;

    QPixmap cachePixmap;
    double cacheDpr;
    bool tiled;
};

class ScrollLabel : public QLabel
{
public:
    explicit ScrollLabel(QWidget *parent = nullptr);
    ~ScrollLabel();

    void startScroll();
    void stopScroll(bool pause = false);
    void setText(const QString &text, bool scroll = false);
protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void timerEvent(QTimerEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;
    virtual void hideEvent(QHideEvent *event) override;
    virtual void enterEvent(QEvent *event) override;
    virtual void leaveEvent(QEvent *event) override;
    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
protected:
    QPixmap cachePixmap;
    double cacheDpr;
    int textWidth;
    int textHeight;
    int curPos;
    int unit;
    int tm;
    bool pause;
    bool scroll;
};

class KeyLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    KeyLineEdit(QWidget *parent = nullptr);
    ~KeyLineEdit();
public:
    void setKey(const QKeySequence &key);
    QKeySequence getKey() const;

signals:
    void keyChanged(const QKeySequence &key);
protected:
    void keyPressEvent(QKeyEvent *e) override;
private:
    QKeySequence key;
};


QString toHumanRead(qint64 size);
QString toHumanRead(const QDateTime &dt, const QDateTime &now = QDateTime::currentDateTime(), bool compat = true);
void moveToCenter(QWidget * const widget);
void restorePosition(QSettings &settings, QWidget * const widget, bool center = true);
void savePosition(QSettings &settings, QWidget * const widget);
void retainSizeWhenHidden(QWidget *w);
QIcon toIcon(const QString &normalImg, const QString &hoverImg, const QString &disableImg);
bool openFolderAndSelectedFile(const QString &filePath);
class QCompleter *installComboxCompleter(class QComboBox *combo);
void resetComboxText(class QComboBox *combox, const QStringList &items, bool addCurrentTextIfNotExist = false);
QByteArray saveWidgetGeometry(QWidget *w);
bool restoreWidgetGeometry(QWidget *w, const QByteArray &encoded, bool reCheck = true);

#endif  // LAFPLAY_UTILS_H
