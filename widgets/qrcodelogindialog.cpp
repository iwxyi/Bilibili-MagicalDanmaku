#include <QMessageBox>
#include "qrcodelogindialog.h"
#include "ui_qrcodelogindialog.h"
#include "netutil.h"

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
        if (!json.code())
            return error("code不为0：" + QString::number(json.code()));
        MyJson data = json.data();
        QString jsons(data, url);
        QString jsons(data, oauthKey);

        // 生成QRCode

    });
}

void QRCodeLoginDialog::error(QString msg, QString title)
{
    QMessageBox::warning(this, title.isEmpty() ? "error" : title, msg);
}
