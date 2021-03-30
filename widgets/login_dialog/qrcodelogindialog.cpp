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
    getLoginUrl();

    queryTimer->setInterval(3000);
    connect(queryTimer, SIGNAL(timeout()), this, SLOT(getLoginInfo()));
}

QRCodeLoginDialog::~QRCodeLoginDialog()
{
    delete ui;
}

void QRCodeLoginDialog::getLoginUrl()
{
    get("http://passport.bilibili.com/qrcode/getLoginUrl", [=](MyJson json){
        if (json.code())
            return error("code不为0：" + QString::number(json.code()));
        MyJson data = json.data();
        QString jsons(data, url);
        this->jsons(data, oauthKey);

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
    post("http://passport.bilibili.com/qrcode/getLoginInfo", QStringList{"oauthKey", oauthKey}, [=](QNetworkReply* reply){
        MyJson json(reply->readAll());
        int jsonb(json, status);
        QString jsons(json, message);

        if (!status)
        {
            int jsoni(json, data);
            if (data == -1)
                ui->statusLabel->setText("秘钥错误");
            else if (data == -4) // 秘钥正确但未扫描
                ui->statusLabel->setText("等待扫描");
            else if (data == -5) // 扫描成功但未确认
                ui->statusLabel->setText("等待确认");
        }
        else // 成功
        {
            ui->statusLabel->setText("扫描成功，5秒钟后登录");
            QVariant variantCookies = reply->header(QNetworkRequest::SetCookieHeader);
            QList<QNetworkCookie> cookies = qvariant_cast<QList<QNetworkCookie> >(variantCookies);
            QStringList sl;
            foreach (QNetworkCookie cookie, cookies)
            {
                QString s = cookie.toRawForm();
                if (s.contains("expires"))
                    sl << s;
            }

            QTimer::singleShot(1000, [=]{
                emit logined(sl.join(";"));
                this->close();
            });
        }
    });
}

void QRCodeLoginDialog::error(QString msg, QString title)
{
    QMessageBox::warning(this, title.isEmpty() ? "error" : title, msg);
}
