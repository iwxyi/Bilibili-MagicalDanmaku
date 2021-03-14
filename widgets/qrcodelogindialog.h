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
    ~QRCodeLoginDialog();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void getLoginUrl();
    void error(QString msg, QString title = "");

private:
    Ui::QRCodeLoginDialog *ui;

    QString oauthKey;
};

#endif // QRCODELOGINDIALOG_H
