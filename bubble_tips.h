#ifndef LAFPLAY_BUBBLETIPS_H
#define LAFPLAY_BUBBLETIPS_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>

class BubbleTips : public QWidget
{
    Q_OBJECT
public:
    enum DIRECT {
        DIRECT_NONE = 0,  // 没有三角
        DIRECT_LEFT,
        DIRECT_TOP,
        DIRECT_RIGHT,
        DIRECT_BOTTOM
    };

    explicit BubbleTips(QWidget *parent = nullptr);
    ~BubbleTips();

    void setBackColor(int r, int g, int b, int a = 255);
    void setBackColor(const QColor &c);
    // 设置三角方向(左上右下)，位置系数(宽度 * 系数)
    void setDirect(DIRECT direct = DIRECT::DIRECT_TOP, double size = 0.75);
    void setContentFont(QFont font = {});
    void setContent(const QString &content, QColor color = {});
    void setLeftTopMargin(int leftMargin, int topMargin);
    void setWordWrap(bool on);
public:
    static QPointer<BubbleTips> showTips(const QString &message, QWidget *posFromWidget, const QPoint &diff = QPoint());
    static QPointer<BubbleTips> showTips(const QString &message, const QPoint &center);
protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

    void onTimer();
private:
    QLabel *label;
    QTimer *tm;
    QColor backColor = { 255, 0, 255, 255 };
    DIRECT direct;

    double posSize = 0.75;  // 三角形位置
    int leftMargin = 5;  // 文字左边距
    int topMargin = 5;  // 文字上边距

    int display = 6;  // 默认8s自动消失,鼠标移动后顺移
    int triangleHeight = 5;  // 三角形的高度
    int transparentLength = 5;  // 透明宽度
    bool enter;
};

#endif  // LAFPLAY_BUBBLETIPS_H
