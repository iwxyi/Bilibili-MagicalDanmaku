#ifndef DBBROWSER_H
#define DBBROWSER_H

#include <QWidget>
#include <QSettings>
#include <QSqlQueryModel>
#include "sqlservice.h"

namespace Ui {
class DBBrowser;
}

class DBBrowser : public QWidget
{
    Q_OBJECT

public:
    explicit DBBrowser(SqlService* service, QSettings* settings, QWidget *parent = nullptr);
    ~DBBrowser();

private slots:
    void on_execButton_clicked();

    void on_resultTable_customContextMenuRequested(const QPoint &pos);

private:
    Ui::DBBrowser *ui;
    SqlService* service = nullptr;
    QSettings* settings = nullptr;
    QSqlQueryModel* model = nullptr;
};

#endif // DBBROWSER_H
