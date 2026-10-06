#pragma once

#include <QWidget>

class DragAutoScroller;
class ScoreTableView;
class StudentTableSidebar;

class StudentTablePage : public QWidget
{
    Q_OBJECT

public:
    explicit StudentTablePage(QWidget* parent = nullptr);

    ScoreTableView* tableView() const;
    StudentTableSidebar* sidebar() const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    ScoreTableView* m_tableView = nullptr;
    StudentTableSidebar* m_sidebar = nullptr;
    DragAutoScroller* m_dragAutoScroller = nullptr;
};
