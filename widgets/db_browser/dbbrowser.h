#ifndef DBBROWSER_H
#define DBBROWSER_H

#include <QWidget>
#include <QSettings>
#include <QSqlQueryModel>
#include <QListWidgetItem>
#include "sqlservice.h"

namespace Ui {
class DBBrowser;
}

class DBBrowser : public QWidget
{
    Q_OBJECT

public:
    explicit DBBrowser(QWidget *parent = nullptr);
    ~DBBrowser();
    void setService(SqlService* service);

    void showQueryResult(QString sql);

protected:
    void showEvent(QShowEvent* event) override;

signals:
    void signalProcessVariant(QString& code);

private slots:
    void on_execButton_clicked();

    void on_resultTable_customContextMenuRequested(const QPoint &pos);

    void on_tabWidget_currentChanged(int index);

    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

    void on_askAIButton_clicked();

    void on_runSqlButton_clicked();

private:
    void initVisualList();

private:
    Ui::DBBrowser *ui;
    SqlService* service = nullptr;
    QSqlQueryModel* model = nullptr;
    QString cacheCode;
    bool inited = false;
};

#endif // DBBROWSER_H
