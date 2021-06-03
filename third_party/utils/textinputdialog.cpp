#include "textinputdialog.h"
#include "ui_textinputdialog.h"

TextInputDialog::TextInputDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TextInputDialog)
{
    ui->setupUi(this);

    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
}

TextInputDialog::~TextInputDialog()
{
    delete ui;
}

QString TextInputDialog::getText(QWidget *parent, QString title, QString label, QString def, bool *ok)
{
    TextInputDialog tid(parent);
    tid.setWindowTitle(title);
    tid.ui->label->setText(label);
    tid.ui->plainTextEdit->setPlainText(def);
    *ok = (tid.exec() == QDialog::Accepted);
    return tid.ui->plainTextEdit->toPlainText();
}
