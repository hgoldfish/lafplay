#ifndef LAFPLAY_STYLED_DECORATION_H
#define LAFPLAY_STYLED_DECORATION_H
#include <QWidget>
#include <QStyle>
#include <QStyleOptionTitleBar>

class Interaction : public QObject
{
    Q_OBJECT
public:
    Interaction(QObject *child, QObject *parent);
    ~Interaction();
private slots:
    void clear();
private:
    QObject *child;
};

class StyledDecoration : public QWidget
{
    Q_OBJECT
private:
    Q_DISABLE_COPY(StyledDecoration)
public:
    explicit StyledDecoration(QWidget *childWindow);
    ~StyledDecoration();
public:
    static void installTo(QWidget *widget);
protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
    void closeEvent(QCloseEvent *event);
    bool event(QEvent *event);
private:
    void runSystemMenu(const QPoint &globalPosition);
    QStyle::SubControl getSubControl(const QPoint &pos) const;
    QStyleOptionTitleBar getTitleBarOption() const;
    void setNormalMode();
    void setMaximizedMode();
    void updateMask();
    bool showResizeCursor(const QPoint &pos);
    bool tryToResize(const QPoint &pos);
    void resizeMe(const QPoint &pos);
    void positChild();
private slots:
    void deleteMyself();
private:
    QWidget *childWindow;
    Qt::WindowStates myWindowState;
    QStyle::SubControl activeSubControl;
    QStyle::SubControl hoveredSubControl;
    QPoint beginMovingPosition;
    QRect originalGeometry;
    enum ResizeAtCorner { Nowhere, TopLeft, BottomLeft, TopRight, BottomRight, Left, Top, Right, Bottom } resizeAt;
};

#endif  // LAFPLAY_STYLED_DECORATION_H
