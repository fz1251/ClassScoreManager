#include "WelcomePage.h"
#include "../widgets/HomeActionCard.h"
#include "../widgets/ShineLabel.h"
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QVBoxLayout>

WelcomePage::WelcomePage(QWidget *parent)
    : QWidget(parent)
{
    // 页面根布局：标题固定在顶部，其余空间全部给卡片区域
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(0);
    rootLayout->setContentsMargins(40, 40, 40, 40);

    // 标题：应用名称
    auto *titleLabel = new ShineLabel(this);
    QFont titleFont;
    titleFont.setPointSize(42);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setText(tr("班级积分管理系统"));
    rootLayout->addWidget(titleLabel, 0);

    // 底部区域：占据全部剩余空间，卡片组在其中横向、纵向居中
    auto *cardAreaLayout = new QVBoxLayout;
    cardAreaLayout->setSpacing(0);
    cardAreaLayout->addStretch();

    // 操作卡片横向布局：两侧弹簧保持卡片组水平居中
    auto *cardLayout = new QHBoxLayout;
    cardLayout->setSpacing(0);
    cardLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    // 打开文件卡片
    auto *openFileCard = new HomeActionCard(tr("打开文件"), this);
    openFileCard->setIcon(QIcon(QStringLiteral(":/icons/resources/icons/open-file.svg")));
    cardLayout->addWidget(openFileCard);

    // 两张卡片之间的固定间距
    cardLayout->addSpacing(80);

    // 新建文件卡片
    auto *newFileCard = new HomeActionCard(tr("新建文件"), this);
    newFileCard->setIcon(QIcon(QStringLiteral(":/icons/resources/icons/new-file.svg")));
    cardLayout->addWidget(newFileCard);

    cardLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    cardAreaLayout->addLayout(cardLayout);
    cardAreaLayout->addStretch();
    rootLayout->addLayout(cardAreaLayout, 1);

    connect(openFileCard, &QAbstractButton::clicked, this, &WelcomePage::openFileRequested);
    connect(newFileCard, &QAbstractButton::clicked, this, &WelcomePage::newFileRequested);
}
