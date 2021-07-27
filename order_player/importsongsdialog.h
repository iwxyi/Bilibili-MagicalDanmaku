#ifndef IMPORTSONGSDIALOG_H
#define IMPORTSONGSDIALOG_H

#include <QWidget>

namespace Ui {
class ImportSongsDialog;
}

class ImportSongsDialog : public QWidget
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
