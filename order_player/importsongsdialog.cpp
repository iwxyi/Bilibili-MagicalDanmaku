#include "importsongsdialog.h"
#include "ui_importsongsdialog.h"

ImportSongsDialog::ImportSongsDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImportSongsDialog)
{
    ui->setupUi(this);
}

ImportSongsDialog::~ImportSongsDialog()
{
    delete ui;
}

void ImportSongsDialog::on_importButton_clicked()
{
    QString text = ui->plainTextEdit->toPlainText();
    emit importMusics(ui->typeCombo->currentIndex(), text.split("\n", QString::SkipEmptyParts));
}
