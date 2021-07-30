#ifndef IMPORTSONGSDIALOG_H
#define IMPORTSONGSDIALOG_H

#include <QDialog>

namespace Ui {
class ImportSongsDialog;
}

class ImportSongsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportSongsDialog(QWidget *parent = nullptr);
    ~ImportSongsDialog();

signals:
    void importMusics(int format, const QStringList& lines);

private slots:
    void on_importButton_clicked();

private:
    Ui::ImportSongsDialog *ui;
};

#endif // IMPORTSONGSDIALOG_H
