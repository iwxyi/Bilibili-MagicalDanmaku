#ifndef QRCODELOGINDIALOG_H
#define QRCODELOGINDIALOG_H

#include <QDialog>
#include "netinterface.h"

namespace Ui {
class QRCodeLoginDialog;
}

class QRCodeLoginDialog : public QDialog, public NetInterface
{
    Q_OBJECT

public:
    explicit QRCodeLoginDialog(QWidget *parent = nullptr);
    ~QRCodeLoginDialog() override;

private slots:
    void getLoginUrl();
    void getLoginInfo();
    void getBuvid();

private:
    void error(QString msg, QString title = "");

signals:
    void logined(QString cookie);

private:
    Ui::QRCodeLoginDialog *ui;
    
    QString qrcode_key;
    QTimer* queryTimer;

    QString b3, b4;
};

#endif // QRCODELOGINDIALOG_H
