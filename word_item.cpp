#include <QtGui/qpainter.h>
#include "word_item.h"

WordItem::WordItem(const QString &wordText, bool canRemove, QWidget *parent)
    : QWidget(parent)
    , wordText(wordText)
    , style(Style__)
    , canRemove(canRemove)
    , hover(false)
{
}

void WordItem::paintEvent(QPaintEvent *)
{
    switch (style) {
    case Style_X:
        paintStyleX();
        break;
    case Style__:
        paintStyle_();
        break;
    }
}

void WordItem::mousePressEvent(QMouseEvent *)
{
    if (!canRemove) {
        return;
    }
    emit deleting(wordText);
}

void WordItem::enterEvent(QEvent *)
{
    hover = true;
    update();
}

void WordItem::leaveEvent(QEvent *)
{
    hover = false;
    update();
}

void WordItem::paintStyleX()
{
    QPainter painter(this);
    QColor background = palette().color(QPalette::Text);
    QBrush foreground = palette().brush(QPalette::Base);
    QPen pen(foreground, 1.0, Qt::SolidLine);
    QRect r = rect();
    r.setWidth(r.width() - 1);
    r.setHeight(r.height() - 1);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing, true);
#else
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing,
                           true);
#endif
    painter.setBrush(hover && canRemove
                             ? (background.lightness() >= 200 ? background.darker(200) : background.lighter(140))
                             : background);
    painter.drawRoundedRect(r, 8.0, 8.0);

    painter.setBrush(foreground);
    painter.setPen(pen);
    if (r.width() > 24 && canRemove) {
        int x = r.width() - 24;
        painter.drawLine(x, 1, x, r.height() - 1);
        QRect closeRect(x + 1, 1, 23, r.height() - 1);
        QFont f(QString::fromLatin1("monospace"));
        f.setPixelSize(18);
        painter.setFont(f);
        painter.drawText(closeRect, Qt::AlignCenter, QString::fromUtf8("\xe2\x9c\x95"));
        QRect textRect(0, 0, r.width() - 24, r.height());
        painter.setFont(font());
        painter.drawText(textRect, Qt::AlignCenter, wordText);
    } else {
        painter.drawText(rect(), Qt::AlignCenter, wordText);
    }
}

void WordItem::paintStyle_()
{
    QPainter painter(this);
    QColor background = palette().color(QPalette::Base);
    QColor foreground = palette().color(QPalette::Text);

    QRect r = rect();
    r.setWidth(r.width() - 1);
    r.setHeight(r.height() - 1);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing, true);
#else
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::HighQualityAntialiasing,
                           true);
#endif
    background = hover && canRemove ? (background.lightness() >= 200 ? background.darker(150) : background.lighter(140))
                                    : background;
    painter.setBrush(background);
    painter.setPen(background);
    painter.drawRoundedRect(r, r.height() / 2, r.height() / 2);

    painter.setBrush(foreground);
    painter.setPen(foreground);
    QFontMetrics fontMetrics(font());
    QSize hint = fontMetrics.size(Qt::TextSingleLine | Qt::AlignCenter, wordText);
    painter.drawText(rect(), Qt::AlignCenter, wordText);

    if (canRemove && hint.width() < width()) {
        painter.setPen(Qt::white);
        // 在字的左边画图-
#define IconSize 10
#define IconMargin 6
        QSize sz(IconSize, IconSize);

        QRect roundRt(r.center().x() - hint.width() / 2 - IconMargin - sz.width(), r.center().y() - sz.height() / 2 + 1,
                      sz.width(), sz.height());
        painter.setBrush(QColor("#F51C02"));
        painter.drawRoundedRect(roundRt, roundRt.width() / 2, roundRt.height() / 2);

        QRect rt_(0, 0, 4, 1);
        rt_.moveCenter(roundRt.center());
        painter.fillRect(rt_, Qt::white);
    }
}

QSize WordItem::sizeHint() const
{
    QFontMetrics fontMetrics(font());
    QSize hint = fontMetrics.size(Qt::TextSingleLine | Qt::AlignCenter, wordText + QString(4, ' '));
    hint.setWidth(hint.width() + 24);
    if (hint.height() < 24) {
        hint.setHeight(24);
    }
    switch (style) {
    case Style_X:
        if (canRemove) {
            hint.setWidth(hint.width() + IconSize + IconMargin * 3 - 24);
        }
        return hint;
    default:
        return hint;
    }
}
