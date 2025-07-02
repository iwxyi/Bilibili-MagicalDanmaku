#include "importsongsdialog.h"
#include "ui_importsongsdialog.h"
#include "qt_compat.h"

ImportSongsDialog::ImportSongsDialog(QSettings *settings, QWidget *parent) :
    settings(settings), QDialog(parent),
    ui(new Ui::ImportSongsDialog)
{
    ui->setupUi(this);
    setWindowModality(Qt::ApplicationModal);
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->typeCombo->setCurrentIndex(settings->value("import/formatType", 0).toInt());
}

ImportSongsDialog::~ImportSongsDialog()
{
    delete ui;
}

void ImportSongsDialog::on_importButton_clicked()
{
    QString text = ui->plainTextEdit->toPlainText();
    emit importMusics(ui->typeCombo->currentIndex(), text.split("\n", SKIP_EMPTY_PARTS));
    this->close();
}

void ImportSongsDialog::on_typeCombo_currentIndexChanged(int index)
{
    settings->setValue("import/formatType", index);
}
