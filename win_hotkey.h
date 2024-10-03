#ifndef WINHOTKEY_H
#define WINHOTKEY_H
#include <QtGui>

void notify(WId hwnd, unsigned int modifier, unsigned int raw_key);

class WinHotKey :public QObject
{
    Q_OBJECT
        friend void notify(WId hwnd, unsigned int modifier, unsigned int raw_key);
public:
    WinHotKey(QObject *parent = 0);
    ~WinHotKey();
public:
    /**
     * 添加一个系统热键
     * 返回-1时表示失败
     * 否则返回一个热键ID，可以利用此ID删除热键，同时此ID是catched信号的参数
     */
    int addHotKey(QKeySequence key);

    /**
     * 删除一个系统热键
     * 参数ID 添加系统热键时返回的ID
     */
    void removeHotKey(int id);
signals:
    /**
     * 当热键被按下的时候激活此信号
     */
    void catched(int hotkeyid);
private:
    WId hwnd;
    int lastid;
    QMap<int, QKeySequence> hotkeys;
};
#endif
