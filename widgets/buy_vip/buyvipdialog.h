#ifndef BUYVIPDIALOG_H
#define BUYVIPDIALOG_H

#include <QDialog>
#include "netinterface.h"

namespace Ui {
class BuyVIPDialog;
}

class BuyVIPDialog : public QDialog, public NetInterface
{
    Q_OBJECT

public:
    explicit BuyVIPDialog(QString roomId, QString upId, QString userId, QString roomTitle, QString upName, QString username, QWidget *parent = nullptr);
    ~BuyVIPDialog() override;

    void updatePrice();

protected:
    void resizeEvent(QResizeEvent* e) override;
    void showEvent(QShowEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

private slots:
    void on_payButton_clicked();

    void on_couponButton_clicked();

signals:
    void refreshVIP();

private:
    Ui::BuyVIPDialog *ui;

    QString roomId;
    QString upId;
    QString userId;
    QString roomTitle;
    QString upName;
    QString username;

    int vipLevel = 1;
    int vipType = 1;
    int vipMonth = 1;
    QString couponCode;

    bool firstShow = true;
    bool mayPayed = false;
    const QString serverPath = "http://iwxyi.com:8102/server/";
};

#endif // BUYVIPDIALOG_H
