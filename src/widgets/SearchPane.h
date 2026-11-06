#pragma once

#include <QString>
#include <QWidget>

class CloseButton;
class QAbstractItemModel;
class QLabel;
class QLineEdit;
class QListView;
class QStackedWidget;

// 左侧搜索窗格：一个搜索框、一个结果列表；搜索范围由 MainWindow 切换，窗格本身不感知页面
class SearchPane : public QWidget
{
    Q_OBJECT
public:
    explicit SearchPane(QWidget* parent = nullptr);

    void setModel(QAbstractItemModel* model);
    void setSearchContext(const QString& title, const QString& emptyText, const QString& noMatchText);
    void clearSearch();
    void focusSearch();

signals:
    void searchTextChanged(const QString& text);
    void resultActivated(int row);
    void closeRequested();

private:
    void updateEmptyState();
    void activateCurrentResult();

    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    QLabel* titleLabel = nullptr;
    CloseButton* closeButton = nullptr;
    QLineEdit* searchEdit = nullptr;
    QListView* resultView = nullptr;
    QStackedWidget* resultStack = nullptr;
    QLabel* emptyLabel = nullptr;

    QString emptyText;
    QString noMatchText;
};
