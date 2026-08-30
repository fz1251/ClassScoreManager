#pragma once

#include <QWidget>

class QTableView;
class StudentTableSidebar;

class StudentTablePage : public QWidget
{
    Q_OBJECT

public:
    explicit StudentTablePage(QWidget *parent = nullptr);

    QTableView *tableView() const;
    StudentTableSidebar *sidebar() const;

private:
    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    QTableView *m_tableView = nullptr;
    StudentTableSidebar *m_sidebar = nullptr;
};
