#pragma once

#include <QCoreApplication>
#include <QString>
#include <QStringList>

class QWidget;

// 列表选择对话框：标题即操作描述，内容区只有一个下拉框；返回选中项的索引，取消返回 -1
class ItemSelectionDialog
{
    Q_DECLARE_TR_FUNCTIONS(ItemSelectionDialog)

public:
    static int selectIndex(QWidget* parent, const QString& title, const QStringList& items, int current = 0);
};
