#ifndef HOMEACTIONCARD_H
#define HOMEACTIONCARD_H

#include <QAbstractButton>
#include <QPixmap>
#include <QSize>
#include <QString>

class QColor;
class QFocusEvent;
class QMouseEvent;
class QPaintEvent;
class QRectF;
class QVariantAnimation;

class HomeActionCard : public QAbstractButton
{
    Q_OBJECT

public:
    explicit HomeActionCard(QWidget *parent = nullptr);
    explicit HomeActionCard(const QString &text, QWidget *parent);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    void startHoverAnimation(qreal endProgress);
    void ensureShadowCache();
    QRectF cardRect() const;
    QColor fillColor() const;

    qreal m_hoverProgress{0.0};
    QVariantAnimation* m_hoverAnimation{nullptr};
    QPixmap m_shadowCache;
};

#endif // HOMEACTIONCARD_H
