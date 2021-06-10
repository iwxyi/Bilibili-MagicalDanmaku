#include <QGraphicsDropShadowEffect>
#include "buyvipdialog.h"
#include "ui_buyvipdialog.h"

BuyVIPDialog::BuyVIPDialog(QString roomId, QString upId, QString userId, QString upName, QString username, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BuyVIPDialog)
{
    ui->setupUi(this);

    setModal(false);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->payButton->setBgColor(QColor("#ffe5d9"));
    ui->payButton->setPaddings(36, 36, 9, 9);
    ui->payButton->adjustMinimumSize();
    ui->imageLabel->setStyleSheet("");

    QStringList funcs {
        "关键词回复",
        "监听直播间事件",
        "功能代码咨询",
        "弹幕过滤",
        "代码自动备份",
        "修改点歌回复",
        "弹幕切歌",
        "点歌过滤",
        "弹幕打卡",
        "设置专属昵称",
        "积分系统",
        "弹幕复读",
        "进场坐骑特效",
        "天气预报",
        "一键开播",
        "开播QQ通知",
        "上船发消息",
        "中奖播报",
        "词库过滤",
        "答谢特别关注",
        "感谢最佳助攻",
        "感谢分享直播间",
        "远程禁言",
        "大乱斗报时",
        "PK对面信息播报",
        "PK在线舰长数播报",
        "匿名黑听",
        "表情包回溯截图",
        "截图批量管理",
        "制作GIF",
        "勋章升级提示",
        "一些小游戏",
        "扩展·投票",
        "扩展·五子棋",
        "直播相关接口",
        "任意可编程操作",
        "未来更多扩展功能",
    };

    foreach (QString f, funcs)
    {
        QLabel* label = new QLabel(f, ui->scrollAreaWidgetContents);
        label->setCursor(Qt::PointingHandCursor);

        QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(label);
        effect->setColor(QColor("#fde0d4"));
        effect->setBlurRadius(36);
        effect->setXOffset(1);
        effect->setYOffset(1);
        label->setGraphicsEffect(effect);
    }

    ui->scrollArea->setItemMargin(9, 0);
    ui->scrollArea->initFixedChildren();
    ui->scrollArea->resizeWidgetsToSizeHint();
    ui->scrollArea->setAllowDifferentWidth(true);
}

BuyVIPDialog::~BuyVIPDialog()
{
    delete ui;
}

void BuyVIPDialog::resizeEvent(QResizeEvent *e)
{
    QDialog::resizeEvent(e);

    QPixmap pixmap(":/illustrations/bear");
    pixmap = pixmap.scaled(ui->imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->imageLabel->setPixmap(pixmap);
    ui->imageLabel->setMinimumSize(QSize(64, 64));
}
