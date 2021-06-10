#ifndef BUYVIPDIALOG_H
#define BUYVIPDIALOG_H

#include <QDialog>

namespace Ui {
class BuyVIPDialog;
}

class BuyVIPDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BuyVIPDialog(QString roomId, QString upId, QString userId, QString upName, QString username, QWidget *parent = nullptr);
    ~BuyVIPDialog();

    void updatePrice();

protected:
    void resizeEvent(QResizeEvent* e) override;
    void showEvent(QShowEvent *e) override;

private:
    Ui::BuyVIPDialog *ui;

    int vipLevel = 1;
    int vipType = 1;
    int vipMonth = 1;
};

#endif // BUYVIPDIALOG_H
