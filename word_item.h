#ifndef LAFPLAY_WORD_ITEM_H
#define LAFPLAY_WORD_ITEM_H

#include <QtWidgets/qwidget.h>

class WordItem: public QWidget
{
    Q_OBJECT
public:
    enum Style {
        Style_X,
        Style__,
    };
    WordItem(const QString &wordText, bool canRemove, QWidget *parent);
signals:
    void deleting(const QString &wordText);
protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void enterEvent(QEvent *event) override;
    virtual void leaveEvent(QEvent *event) override;
protected:
    void paintStyleX();
    void paintStyle_();
public:
    virtual QSize sizeHint() const override;
private:
    QString wordText;
    Style style;
    bool canRemove;
    bool hover;
};

#endif  // LAFPLAY_WORD_ITEM_H
