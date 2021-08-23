#include "qsseditdialog.h"
#include "ui_qsseditdialog.h"

QssEditDialog::QssEditDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::QssEditDialog)
{
    ui->setupUi(this);
    if (parent)
        this->setGeometry(this->parentWidget()->geometry());
}

QssEditDialog::~QssEditDialog()
{
    delete ui;
}

QString QssEditDialog::getText(QWidget *parent, QString title, QString label, QString def, bool *ok)
{
    QssEditDialog tid(parent);
    tid.setWindowTitle(title);
    tid.ui->label->setText(label);
    tid.ui->plainTextEdit->setPlainText(def);
    *ok = (tid.exec() == QDialog::Accepted);
    return tid.ui->plainTextEdit->toPlainText();
}
