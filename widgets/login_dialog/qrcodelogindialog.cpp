#include <QMessageBox>
#include <QPainter>
#include <QTimer>
#include "qrcodelogindialog.h"
#include "ui_qrcodelogindialog.h"
#include "netutil.h"
#include "qrencode/qrencode.h"

QRCodeLoginDialog::QRCodeLoginDialog(QWidget *parent) :
    QDialog(parent),
    NetInterface(this),
    ui(new Ui::QRCodeLoginDialog),
    queryTimer(new QTimer(this))
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    getLoginUrl();

    queryTimer->setInterval(3000);
    connect(queryTimer, SIGNAL(timeout()), this, SLOT(getLoginInfo()));

    QTimer::singleShot(1000, this, [=]{
        getBuvid();
    });
}

QRCodeLoginDialog::~QRCodeLoginDialog()
{
    delete ui;
}

void QRCodeLoginDialog::getLoginUrl()
{
    get("https://passport.bilibili.com/x/passport-login/web/qrcode/generate", [=](MyJson json){
        if (json.code())
            return error("code不为0：" + QString::number(json.code()));
        MyJson data = json.data();
        QString jsons(data, url);
        this->jsons(data, qrcode_key);

        // 生成QRCode
        QRcode* qrcode = QRcode_encodeString(url.toUtf8(), 2, QR_ECLEVEL_Q, QR_MODE_8, 1);
        qint32 temp_width = 400, temp_height = 400; //  显示大小
        qint32 qrcode_width = qMax(qrcode->width, 1);
        double scale_x = (double)temp_width / (double)qrcode_width; // 二维码图片的缩放比例
        double scale_y = (double)temp_height / (double)qrcode_width;
        QImage img = QImage(temp_width, temp_height, QImage::Format_ARGB32);

        QPainter painter(&img);
        painter.setBrush(Qt::white);
        painter.setPen(Qt::NoPen);
        painter.drawRect(0, 0, temp_width, temp_height);

        painter.setBrush(Qt::black);
        for( qint32 y = 0; y < qrcode_width; y ++)
        {
            for(qint32 x = 0; x < qrcode_width; x++)
            {
                if(qrcode->data[y * qrcode_width + x] & 0x01)
                {
                    QRectF r(x * scale_x, y * scale_y, scale_x, scale_y);
                    painter.drawRects(&r, 1);
                }
            }
        }

        QPixmap pixmap = QPixmap::fromImage(img);
        ui->qrcodeLabel->setPixmap(pixmap);
        queryTimer->start();
        delete qrcode;
    });
}

void QRCodeLoginDialog::getLoginInfo()
{
    get("https://passport.bilibili.com/x/passport-login/web/qrcode/poll?qrcode_key=" + qrcode_key, [=](QNetworkReply* reply){
        MyJson json(reply->readAll());
        if (json.code())
            return error("code不为0：" + QString::number(json.code()));

        MyJson data = json.data();
        int jsoni(data, code);
        if (code == 0) // 成功
        {
            ui->statusLabel->setText("扫描成功，即将登录");
            QVariant variantCookies = reply->header(QNetworkRequest::SetCookieHeader);
            QList<QNetworkCookie> cookies = qvariant_cast<QList<QNetworkCookie> >(variantCookies);
            QStringList sl;
            foreach (QNetworkCookie cookie, cookies)
            {
                sl << cookie.toRawForm();
            }

            // 添加buvid
            if (!b3.isEmpty() && !(sl.join(";").contains("buvid3=")))
                sl << "buvid3=" + b3;
            if (!b4.isEmpty() && !(sl.join(";").contains("buvid4=")))
                sl << "buvid4=" + b4;

            // 添加刷新token
            QString refresh_token = data.s("refresh_token");
            sl << "ac_time_value=" + refresh_token;
            qDebug() << "设置refresh_token:" << refresh_token;

            QTimer::singleShot(1000, this, [=]{
                emit logined(sl.join(";"));
                this->close();
            });
        }
        else if(code == 86038) // 二维码已失效
        {
            ui->statusLabel->setText("二维码已失效");
        }
        else if(code == 86090) // 已扫描但未确认
        {
            ui->statusLabel->setText("等待确认");
        }
        else if(code == 86101) // 未扫描
        {
            ui->statusLabel->setText("等待扫描");
        }
    });
}

/**
 * 登录设置buvid
 * https://github.com/SocialSisterYi/bilibili-API-collect/blob/master/docs/misc/buvid3_4.md
 */
void QRCodeLoginDialog::getBuvid()
{
    get("https://api.bilibili.com/x/frontend/finger/spi", [=](MyJson json) {
        MyJson data = json.data();
        this->b3 = data.s("b_3");
        this->b4 = data.s("b_4");
        qDebug() << "获取到BUVID:" << b3 << b4;
    });
}

void QRCodeLoginDialog::error(QString msg, QString title)
{
    QMessageBox::warning(this, title.isEmpty() ? "error" : title, msg);
}
