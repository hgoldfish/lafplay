#ifndef LAFPLAY_FLOW_VIEW_H
#define LAFPLAY_FLOW_VIEW_H

#include <QtWidgets/qscrollarea.h>
#include <QtCore/qpointer.h>

class FlowView : public QScrollArea
{
    Q_OBJECT
public:
    FlowView(QWidget *parent = nullptr);
    virtual ~FlowView() override;
public:
    QList<QWidget *> boxes;
    bool isEmpty() const { return boxes.isEmpty(); }
    void appendBox(QWidget *w);
    void clearBoxs();
    void setMiniumColumnWidth(int w);
signals:
    void requireLoading();
protected:
    bool viewportEvent(QEvent *event) override;
public slots:
    void posist();
    void scrollToTop();
    void scrollToBottom();
    void schedulePosist();
private:
    bool isLoadingTriggerVisible();
private slots:
    void checkLoadingTrigger();
    void toggleWidgets();
public:
    QPointer<QWidget> loadingTrigger;
    int minimumColumnWidth;
    QMargins viewMargins;
    int cellPadding;
    int columnPadding;
    int cachedWidth;
};

class OneFlowView : public QScrollArea
{
    Q_OBJECT
public:
    OneFlowView(QWidget *parent = nullptr);
    virtual ~OneFlowView() override;
public:
    QList<QWidget *> boxes;
    bool isEmpty() const { return boxes.isEmpty(); }
public slots:
    void posist();
    void scrollToTop();
    void scrollToBottom();
    void schedulePosist();
private slots:
    void toggleWidgets();
protected:
    bool viewportEvent(QEvent *event) override;
protected:
    QMargins viewMargins;
    int cellPadding;
    int cachedWidth;
};

#endif  // LAFPLAY_FLOW_VIEW_H
