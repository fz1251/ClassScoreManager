#pragma once

#include <QDialog>
#include <QString>
#include <QStringList>

class QComboBox;

class ItemSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ItemSelectionDialog(QWidget *parent, const QString &title, const QString &label,
                                 const QStringList &items, int current = 0);
    int selectedIndex() const;

private:
    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    QComboBox *selectionComboBox = nullptr;
};
