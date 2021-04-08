#ifndef GUARDONLINEDIALOG_H
#define GUARDONLINEDIALOG_H

#include <QDialog>
#include <QSettings>
#include "livedanmaku.h"

namespace Ui {
class GuardOnlineDialog;
}

class GuardOnlineDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GuardOnlineDialog(QSettings* settings, QString roomId, QString upUid, QWidget *parent = nullptr);
    ~GuardOnlineDialog();

private slots:
    void on_refreshButton_clicked();

private:
    void refreshOnlineGuards(int page);

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

private:
    Ui::GuardOnlineDialog *ui;
    QSettings* settings;
    QString roomId;
    QString upUid;
    QList<qint64> guardIds;
};

#endif // GUARDONLINEDIALOG_H
