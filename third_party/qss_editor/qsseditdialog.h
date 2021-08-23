#ifndef QssEditDialog_H
#define QssEditDialog_H

#include <QDialog>

namespace Ui {
class QssEditDialog;
}

class QssEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit QssEditDialog(QWidget *parent = nullptr);
    ~QssEditDialog();

    static QString getText(QWidget* parent, QString title, QString label, QString def = "", bool* ok = nullptr);

private:
    Ui::QssEditDialog *ui;
};

#endif // QssEditDialog_H
