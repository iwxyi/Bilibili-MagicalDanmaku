#include "eternalblockdialog.h"
#include "ui_eternalblockdialog.h"

EternalBlockDialog::EternalBlockDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EternalBlockDialog)
{
    ui->setupUi(this);
}

EternalBlockDialog::~EternalBlockDialog()
{
    delete ui;
}
