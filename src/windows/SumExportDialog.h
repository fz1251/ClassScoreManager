#pragma once

#include "../core/DataTypes.h"

#include <QDialog>

class QCheckBox;
class QLabel;
class QPushButton;
class QWidget;

class SumExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SumExportDialog(QWidget* parent = nullptr, SortSettings settings = SortSettings());

    SortSettings getCurrentConfig() const;

private:
    void setGroupModeVisibility(bool visible);

    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    QPushButton* studentModeButton = nullptr;
    QPushButton* groupModeButton = nullptr;
    QCheckBox* showStudentNumberBox = nullptr;
    QPushButton* studentNumberButton = nullptr;
    QPushButton* studentScoreButton = nullptr;
    QPushButton* studentAscButton = nullptr;
    QPushButton* studentDescButton = nullptr;
    QWidget* groupFieldRow = nullptr;
    QPushButton* groupNumberButton = nullptr;
    QPushButton* groupScoreButton = nullptr;
    QWidget* groupOrderRow = nullptr;
    QPushButton* groupAscButton = nullptr;
    QPushButton* groupDescButton = nullptr;
};
