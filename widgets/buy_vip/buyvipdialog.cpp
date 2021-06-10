#include <QGraphicsDropShadowEffect>
#include <QDesktopServices>
#include <QInputDialog>
#include "buyvipdialog.h"
#include "ui_buyvipdialog.h"
#include "clickablewidget.h"
#include "fileutil.h"

BuyVIPDialog::BuyVIPDialog(QString roomId, QString upId, QString userId, QString roomTitle, QString upName, QString username, QWidget *parent) :
    QDialog(parent), NetInterface(this), ui(new Ui::BuyVIPDialog),
    roomId(roomId), upId(upId), userId(userId), roomTitle(roomTitle), upName(upName), username(username)

{
    ui->setupUi(this);

    setModal(false);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->payButton->setBgColor(QColor("#ffe5d9"));
    ui->payButton->setPaddings(36, 36, 9, 9);
    ui->payButton->adjustMinimumSize();
    ui->imageLabel->setStyleSheet("");
    ui->couponButton->setBgColor(Qt::white);
    ui->couponButton->adjustMinimumSize();
    ui->couponButton->setLeaveAfterClick(true);

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

    ui->scrollArea->clearWidgets();
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

    ui->scrollArea->setItemMargin(0, 0);
    ui->scrollArea->initFixedChildren();
    ui->scrollArea->resizeWidgetsToSizeHint();
    ui->scrollArea->setAllowDifferentWidth(true);

    QList<QWidget*> typeWidgets{
        ui->typeRRBgWidget,
                ui->typeRoomBgWidget,
                ui->typeRobotBgWidget
    };
    foreach (QWidget* w, typeWidgets)
    {
        QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(w);
        effect->setColor(QColor("#e0e0e0"));
        effect->setBlurRadius(24);
        effect->setXOffset(4);
        effect->setYOffset(4);
        w->setGraphicsEffect(effect);
    }

    auto setMonth = [=](int month) {
        this->vipMonth = month;
        updatePrice();
    };
    connect(ui->monthRadio1, &QRadioButton::clicked, this, [=]{ setMonth(1); });
    connect(ui->monthRadio2, &QRadioButton::clicked, this, [=]{ setMonth(3); });
    connect(ui->monthRadio3, &QRadioButton::clicked, this, [=]{ setMonth(6); });
    connect(ui->monthRadio4, &QRadioButton::clicked, this, [=]{ setMonth(12); });
    connect(ui->monthRadio5, &QRadioButton::clicked, this, [=]{ setMonth(1200); });

    ui->monthRadio1->setChecked(true);
    updatePrice();

    ui->roomIdLabel->setText("房号: " + roomId);
    ui->robotIdLabel->setText("账号: " + userId);

    {
        QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(ui->payButton);
        effect->setColor(QColor("#fde0d4"));
        effect->setBlurRadius(24);
        effect->setXOffset(2);
        effect->setYOffset(2);
        ui->payButton->setGraphicsEffect(effect);
    }
}

BuyVIPDialog::~BuyVIPDialog()
{
    delete ui;
}

void BuyVIPDialog::updatePrice()
{
    // TODO: 从网络获取真实价格

    // 单价
    int single = 49;
    if (vipType == 1)
        single = 49;
    else if (vipType == 2)
        single = 69;
    else if (vipType == 3)
        single = 99;

    // 折扣
    double discount = 1;
    int usedMonth = vipMonth;
    if (vipMonth >= 24) // 永久算两年
    {
        usedMonth = 24;
        discount = 0.8;
    }
    else if (vipMonth >= 12)
        discount = 0.9;
    else if (vipMonth >= 6)
        discount = 0.95;
    else if (vipMonth >= 3)
        discount = 0.98;

    if (discount < 0.99)
    {
        int val = int((discount + 1e-4) * 100);
        int a = val / 10;
        int b = val % 10;
        if (b == 0)
            ui->discountLabel->setText(QString("%1折").arg(a));
        else
            ui->discountLabel->setText(QString("%1.%2折").arg(a).arg(b));
    }
    else
        ui->discountLabel->setText("");

    int total = int(single * usedMonth * discount);

    ui->priceLabel->setText("￥" + QString::number(total));
}

void BuyVIPDialog::resizeEvent(QResizeEvent *e)
{
    QDialog::resizeEvent(e);

    QPixmap pixmap(":/illustrations/bear");
    pixmap = pixmap.scaled(ui->imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->imageLabel->setPixmap(pixmap);
    ui->imageLabel->setMinimumSize(QSize(64, 64));
}

void BuyVIPDialog::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);

    if (firstShow)
    {
        firstShow = false;

        QList<QWidget*> typeCards{
            ui->typeRRBgCard,
                    ui->typeRoomBgCard,
                    ui->typeRobotBgCard
        };
        foreach (QWidget* w, typeCards)
        {
            InteractiveButtonBase* btn = new InteractiveButtonBase(w);
            btn->setRadius(10);
            btn->setFixedSize(w->size());
            btn->show();
            connect(btn, &InteractiveButtonBase::clicked, this, [=]{
                vipType = typeCards.indexOf(w) + 1;
                updatePrice();
            });
        }
    }
}

void BuyVIPDialog::closeEvent(QCloseEvent *e)
{
    if (mayPayed)
        emit refreshVIP();

    QDialog::closeEvent(e);
}

void BuyVIPDialog::on_payButton_clicked()
{
    auto snum = [=](qint64 val){ return QString::number(val); };
    mayPayed = true;

    get("http://iwxyi.com:8102/server/pay/getWebPay",
        { "room_id", roomId, "up_id", upId, "user_id", userId,
         "room_title", roomTitle, "up_name", upName, "username", username,
         "vip_type", snum(vipType), "vip_level", snum(vipLevel),
         "month", snum(vipMonth), "coupon", couponCode },
        [=](MyJson json){

        QString html = json.s("data");
        QString path = "pay.html";
        writeTextFile(path, html);
        QDesktopServices::openUrl(path);

        QTimer::singleShot(10000, this, [=]{
            deleteFile(path);
        });
    });
}

void BuyVIPDialog::on_couponButton_clicked()
{
    QString coupon;
    bool ok = false;
    coupon = QInputDialog::getText(this, "神奇弹幕", "请输入优惠券码", QLineEdit::Normal, this->couponCode, &ok);
    if (!ok)
        return ;
    this->couponCode = coupon;
    ui->couponButton->setText(coupon);
    ui->couponButton->adjustMinimumSize();
}
