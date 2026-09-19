#pragma once

#include <QHash>
#include <QLayout>
#include <QList>
#include <QPointer>

class QPropertyAnimation;

//流式布局：子控件从左到右排列、放不下就换行；重排时用动画把子控件平滑移到新位置
class FlowLayout : public QLayout
{
    Q_OBJECT
public:
    explicit FlowLayout(QWidget* parent = nullptr, int margin = 0, int hSpacing = 8, int vSpacing = 8);
    ~FlowLayout() override;

    void addItem(QLayoutItem* item) override;
    int count() const override;
    QLayoutItem* itemAt(int index) const override;
    QLayoutItem* takeAt(int index) override;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect& rect) override;

    int horizontalSpacing() const;
    int verticalSpacing() const;

    //每行只放一个子控件
    void setSingleColumn(bool enabled);
    void insertWidget(int index, QWidget* widget);
    void removeWidget(QWidget* widget);

private:
    int doLayout(const QRect& rect, bool moveItems) const;
    void moveItemTo(QLayoutItem* item, const QRect& target) const;
    void stopAnimationFor(QWidget* widget) const;

    QList<QLayoutItem*> m_items;
    mutable QList<QLayoutItem*> m_placed; //已经落过位的项：首次落位直接到位，不做动画
    mutable QHash<QWidget*, QPointer<QPropertyAnimation>> m_animations;
    int m_hSpacing;
    int m_vSpacing;
    int m_duration = 220;
    bool m_singleColumn = false;
};
