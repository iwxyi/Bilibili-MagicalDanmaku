#ifndef TEXTINPUTDIALOG_H
#define TEXTINPUTDIALOG_H

#include <QDialog>

namespace Ui {
class TextInputDialog;
}

class TextInputDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TextInputDialog(QWidget *parent = nullptr);
    ~TextInputDialog();

    static QString getText(QWidget* parent, QString title, QString label, QString def = "", bool* ok = nullptr);

private:
    Ui::TextInputDialog *ui;
};

#endif // TEXTINPUTDIALOG_H
