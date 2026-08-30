#pragma once

#include <QWidget>

class QListView;

class TemplateListPage : public QWidget
{
    Q_OBJECT

public:
    explicit TemplateListPage(QWidget *parent = nullptr);

    QListView *listView() const;

private:
    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    QListView *m_listView = nullptr;
};
