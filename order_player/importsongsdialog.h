#ifndef IMPORTSONGSDIALOG_H
#define IMPORTSONGSDIALOG_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class ImportSongsDialog;
}

class ImportSongsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportSongsDialog(QSettings* settings, QWidget *parent = nullptr);
    ~ImportSongsDialog();

signals:
    void importMusics(int format, const QStringList& lines);

private slots:
    void on_importButton_clicked();

    void on_typeCombo_currentIndexChanged(int index);

private:
    Ui::ImportSongsDialog *ui;
    QSettings* settings;
};

#endif // IMPORTSONGSDIALOG_H
