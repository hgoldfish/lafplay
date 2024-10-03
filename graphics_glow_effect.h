#ifndef LAFPLAY_GRAPHIC_GLOW_EFFECT_H
#define LAFPLAY_GRAPHIC_GLOW_EFFECT_H

#include <QtWidgets/qgraphicseffect.h>

class QGraphicsGlowEffect : public QGraphicsEffect
{
public:
    explicit QGraphicsGlowEffect(QObject *parent = 0);
public:
    QRectF boundingRectFor(const QRectF &rect) const;
    void setColor(QColor value);
    void setStrength(int value);
    void setBlurRadius(qreal value);
    QColor color() const;
    int strength() const;
    qreal blurRadius() const;
protected:
    void draw(QPainter *painter);
private:
    static QPixmap applyEffectToPixmap(QPixmap src, QGraphicsEffect *effect, int extent);
    int _extent = 5;
    QColor _color = QColor(255, 255, 255);
    int _strength = 3;
    qreal _blurRadius = 5.0;
};

#endif // LAFPLAY_GRAPHIC_GLOW_EFFECT_H
