#include <QMessageBox>
#include <QPainter>
#include "qrcodelogindialog.h"
#include "ui_qrcodelogindialog.h"
#include "netutil.h"
#include "qrencode/qrencode.h"

QRCodeLoginDialog::QRCodeLoginDialog(QWidget *parent) :
    QDialog(parent),
    NetInterface(this),
    ui(new Ui::QRCodeLoginDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);
}

QRCodeLoginDialog::~QRCodeLoginDialog()
{
    delete ui;
}

void QRCodeLoginDialog::showEvent(QShowEvent *event)
{
    getLoginUrl();

    QDialog::showEvent(event);
}

void QRCodeLoginDialog::getLoginUrl()
{
    get("http://passport.bilibili.com/qrcode/getLoginUrl", [=](MyJson json){
        qDebug() << json;
        if (json.code())
            return error("code不为0：" + QString::number(json.code()));
        MyJson data = json.data();
        QString jsons(data, url);
        this->jsons(data, oauthKey);

        // 生成QRCode
        qDebug() << url.toUtf8();
        QRcode* qrcode = QRcode_encodeString(url.toUtf8(), 2, QR_ECLEVEL_Q, QR_MODE_8, 1);
        qint32 temp_width = 400; //二维码图片的大小
        qint32 temp_height = 400;
        qint32 qrcode_width = qMax(qrcode->width, 1);
        double scale_x = (double)temp_width / (double)qrcode_width; //二维码图片的缩放比例
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
                unsigned char b = qrcode->data[y * qrcode_width + x];
                if(b & 0x01)
                {
                    QRectF r(x * scale_x, y * scale_y, scale_x, scale_y);
                    painter.drawRects(&r, 1);
                }
            }
        }

        QPixmap pixmap = QPixmap::fromImage(img);
        ui->qrcodeLabel->setPixmap(pixmap);
        delete qrcode;
    });
}

void QRCodeLoginDialog::error(QString msg, QString title)
{
    QMessageBox::warning(this, title.isEmpty() ? "error" : title, msg);
}
