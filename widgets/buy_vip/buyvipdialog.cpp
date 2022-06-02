#include <QGraphicsDropShadowEffect>
#include <QDesktopServices>
#include <QInputDialog>
#include <QClipboard>
#include "buyvipdialog.h"
#include "ui_buyvipdialog.h"
#include "clickablewidget.h"
#include "fileutil.h"

BuyVIPDialog::BuyVIPDialog(QString dataPath, QString roomId, QString upId, QString userId, QString roomTitle, QString upName, QString username, qint64 deadline, bool *types, QWidget *parent) :
    QDialog(parent), NetInterface(this), ui(new Ui::BuyVIPDialog),
    dataPath(dataPath), roomId(roomId), upId(upId), userId(userId), roomTitle(roomTitle), upName(upName), username(username), deadline(deadline), types(types)
{
    ui->setupUi(this);

    setModal(false);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->payButton->setBgColor(QColor("#fcaf94"));
    ui->payButton->setPaddings(36, 36, 9, 9);
    ui->payButton->adjustMinimumSize();
    ui->imageLabel->setStyleSheet("");
    ui->couponButton->setBgColor(Qt::white);
    ui->couponButton->adjustMinimumSize();
    ui->couponButton->setLeaveAfterClick(true);

    qint64 currTime = QDateTime::currentSecsSinceEpoch();
    if (currTime < deadline)
    {
        ui->label_4->setText("有效期：" + QString::number((deadline - currTime) / (24 * 3600)) + " 天");
    }

    QStringList funcs {
        "关键词回复",
        "监听直播间事件",
        "功能代码咨询",
        "弹幕过滤",
        "观众氪金排行",
        "大乱斗赛季记录",
        "最佳助攻统计",
        "修改点歌回复",
        "弹幕切歌",
        "点歌黑白名单",
        "弹幕打卡",
        "设置专属昵称",
        "积分系统",
        "弹幕复读",
        "进场坐骑系统",
        "天气预报",
        "一键开播",
        "开播QQ通知",
        "上船发消息",
        "中奖播报",
        "自定义词库",
        "答谢特别关注",
        "感谢最佳助攻",
        "感谢分享直播间",
        "大乱斗报时",
        "大乱斗对面信息播报",
        "在线舰长数播报",
        "连胜播报",
        "远程禁言",
        "键鼠模拟点击",
        "匿名黑听",
        "表情包回溯截图",
        "截图批量管理",
        "制作GIF",
        "勋章升级提示",
        "一些小游戏",
        "扩展·投票",
        "扩展·五子棋",
        "直播相关接口开发",
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

    ui->roomIdLabel->setText("房号: " + roomId);
    ui->robotIdLabel->setText("账号: " + userId);
    ui->roomIdLabel->setToolTip("主播：" + upName);
    ui->robotIdLabel->setToolTip("用户：" + username);

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
    // 从网络获取真实价格
    auto snum = [=](qint64 val){ return QString::number(val); };
    get(serverPath + "pay/getPrice",
        {"vip_type", snum(vipType), "vip_level", snum(vipLevel),
         "month", snum(vipMonth), "coupon", couponCode},
        [=](MyJson json) {
        if (json.code() != 0)
        {
            if (!isDefaultCoupon)
            {
                QMessageBox::warning(this, "获取价格", json.msg() + "\n优惠券：" + couponCode);
                isDefaultCoupon = false;
            }
            couponCode = "";
            ui->couponButton->setText("无优惠券");
            ui->couponButton->adjustMinimumSize();
            updatePrice();
            return ;
        }

        MyJson data = json.data();

        // 设置价格
        double price = data.d("totalPrice");
        ui->priceLabel->setText("￥" + QString::number(price, 'f', 2));

        // 设置折扣
        double maximumPrice = data.d("maximumPrice");
        if (maximumPrice > 0.01)
        {
            double discount = price / maximumPrice;
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
        }

        // 设置单价折扣
        {
            double discount = data.d("coupon_discount");
            if (discount > 0 && discount < 1)
            {
                this->couponDiscount = discount;
                ui->typeRRPriceLabel->setText(QString("%1<sub><s><font color='gray'>%2</font></s>元/月</sub>")
                                              .arg(snum(int(unit1 * discount)))
                                              .arg(snum(int(unit1))));
                ui->typeRoomPriceLabel->setText(QString("%1<sub><s><font color='gray'>%2</font></s>元/月</sub>")
                                              .arg(snum(int(unit2 * discount)))
                                              .arg(snum(int(unit2))));
                ui->typeRobotPriceLabel->setText(QString("%1<sub><s><font color='gray'>%2</font></s>元/月</sub>")
                                              .arg(snum(int(unit3 * discount)))
                                              .arg(snum(int(unit3))));

                {
                    int val = int((discount + 1e-4) * 100);
                    int a = val / 10;
                    int b = val % 10;
                    if (b == 0)
                        ui->couponButton->setToolTip(QString("%1折").arg(a));
                    else
                        ui->couponButton->setToolTip(QString("%1.%2折").arg(a).arg(b));
                }
            }
            else
            {
                ui->typeRRPriceLabel->setText(snum(int(unit1)) + "元/月");
                ui->typeRoomPriceLabel->setText(snum(int(unit2)) + "元/月");
                ui->typeRobotPriceLabel->setText(snum(int(unit3)) + "元/月");
                ui->couponButton->setToolTip("");
            }
        }
    });
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
        QList<QWidget*> typeIndications {
            ui->typeRRIndication,
                    ui->typeRoomIndication,
                    ui->typeRobotIndication
        };
        foreach (auto w, typeIndications)
        {
            w->setStyleSheet("");
        }
        QList<InteractiveButtonBase*> btns;
        foreach (QWidget* w, typeCards)
        {
            InteractiveButtonBase* btn = new InteractiveButtonBase(w);
            btns.append(btn);
            btn->setRadius(10);
            btn->setFixedSize(w->size());
            btn->show();
            connect(btn, &InteractiveButtonBase::clicked, this, [=]{
                vipType = typeCards.indexOf(w) + 1;
                updatePrice();

                foreach (auto w, typeIndications)
                {
                    w->setStyleSheet("");
                }
                typeIndications.at(vipType - 1)->setStyleSheet("background: white; margin: 35px;");
            });
        }

        if (types[1])
            btns[0]->simulateStatePress();
        else if (types[2])
            btns[1]->simulateStatePress();
        else if (types[3])
            btns[2]->simulateStatePress();
        else
            btns.first()->simulateStatePress();
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
    // 判断直播间
    if (vipType == VIP_TYPE_RR || vipType == VIP_TYPE_ROOM)
    {
        if (roomId.isEmpty())
        {
            QMessageBox::information(this, "购买", "感谢您的支持！\n但是，付款前需要先打开对应直播间哦~\n主界面左上角输入房号即可。");
            return ;
        }
    }

    // 判断机器人
    if (vipType == VIP_TYPE_RR || vipType == VIP_TYPE_ROBOT)
    {
        if (userId.isEmpty())
        {
            QMessageBox::information(this, "购买", "感谢您的支持！\n但是，付款前是不是得先登录账号？");
            return ;
        }
    }

    auto snum = [=](qint64 val){ return QString::number(val); };
    mayPayed = true;

    get(serverPath + "pay/getWebPay",
        { "room_id", roomId, "up_id", upId, "user_id", userId,
         "room_title", roomTitle, "up_name", upName, "username", username,
         "vip_type", snum(vipType), "vip_level", snum(vipLevel),
         "month", snum(vipMonth), "coupon", couponCode },
        [=](MyJson json){

        QString html = json.s("data");
        QString path = dataPath + "pay.html";
        if (!writeTextFile(path, html))
        {
            QMessageBox::warning(this, "写入文件失败", "无法写入支付文件：" + path);
        }
        // QMessageBox::information(this, "前往浏览器", "即将使用浏览器打开：" + path);
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path)))
        {
            QApplication::clipboard()->setText(path);
            QMessageBox::warning(this, "打开页面失败", "已复制路径：" + path + "\n可粘贴到浏览器打开");
            return ;
        }

        QTimer::singleShot(10000, this, [=]{
            deleteFile(path);
        });
    });
}

void BuyVIPDialog::on_couponButton_clicked()
{
    QString coupon;
    bool ok = false;
    coupon = QInputDialog::getText(this, "神奇弹幕", "请输入优惠券码，将会在最终支付时折扣", QLineEdit::Normal, this->couponCode, &ok);
    if (!ok)
        return ;
    this->isDefaultCoupon = false;
    this->couponCode = coupon;
    ui->couponButton->setText(coupon);
    ui->couponButton->adjustMinimumSize();
    if (coupon.isEmpty())
        ui->couponButton->setText("优惠券");
    updatePrice();
}
