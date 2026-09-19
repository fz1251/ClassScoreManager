#pragma once

#include <QVariantAnimation>
#include <QWidget>

class QMimeData;
class QMouseEvent;
class QPainter;
class QPaintEvent;
class CardHost;

//学生卡片：自绘胶囊；单击修改小组号，按住拖动参与分组
class StudentCard : public QWidget
{
    Q_OBJECT
public:
    static const inline QString MimeType = "application/x-csm-student";

    StudentCard(int studentIndex, const QString& name, QWidget* parent = nullptr);

    int studentIndex() const;
    CardHost* host() const;
    void setHost(CardHost* host);
    void playEnterAnimation();

    //解析拖动负载：输出学生索引与卡片尺寸，非本页卡片返回false
    static bool decodeDragPayload(const QMimeData* mimeData, int& studentIndex, QSize& cardSize);

signals:
    void groupNumberEditRequested(int studentIndex);
    void dragStarted();
    void dragFinished();

protected:
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    QSize sizeHint() const override;

private:
    void startDrag();
    void startHoverAnimation(qreal endValue);
    void drawCapsule(QPainter& painter, qreal scale, qreal opacity, qreal hoverProgress) const;
    QSize capsuleSize() const;
    QRect capsuleRect() const;
    QPixmap renderCard() const;

    int m_studentIndex;
    QString m_name;
    CardHost* m_host = nullptr;
    QVariantAnimation* m_hoverAnimation = nullptr;
    QVariantAnimation* m_enterAnimation = nullptr;
    qreal m_hoverProgress = 0.0;
    qreal m_enterProgress = 1.0;
    QPoint m_pressPos;
    bool m_pressed = false;
    bool m_dragging = false;
};
