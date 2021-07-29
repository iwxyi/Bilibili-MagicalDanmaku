#include "importsongsdialog.h"
#include "ui_importsongsdialog.h"

ImportSongsDialog::ImportSongsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImportSongsDialog)
{
    ui->setupUi(this);
    setWindowModality(Qt::ApplicationModal);
    setAttribute(Qt::WA_DeleteOnClose, true);
}

ImportSongsDialog::~ImportSongsDialog()
{
    delete ui;
}

void ImportSongsDialog::on_importButton_clicked()
{
    QString text = ui->plainTextEdit->toPlainText();
    emit importMusics(ui->typeCombo->currentIndex(), text.split("\n", QString::SkipEmptyParts));
    this->close();
}
