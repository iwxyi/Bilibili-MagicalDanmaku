#include "buyvipdialog.h"
#include "ui_buyvipdialog.h"

BuyVIPDialog::BuyVIPDialog(QString roomId, QString upId, QString userId, QString upName, QString username, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BuyVIPDialog)
{
    ui->setupUi(this);

    setModal(false);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setAttribute(Qt::WA_DeleteOnClose, true);


}

BuyVIPDialog::~BuyVIPDialog()
{
    delete ui;
}
