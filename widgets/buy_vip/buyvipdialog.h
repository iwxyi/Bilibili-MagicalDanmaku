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

protected:
    void resizeEvent(QResizeEvent* e) override;

private:
    Ui::BuyVIPDialog *ui;
};

#endif // BUYVIPDIALOG_H
