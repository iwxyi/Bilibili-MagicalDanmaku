#ifndef CSVVIEWER_H
#define CSVVIEWER_H

#include <QDialog>
#include <QSettings>
#include <QTableView>
#include <QStandardItemModel>

class CSVViewer : public QDialog
{
    Q_OBJECT
public:
    explicit CSVViewer(QString filePath, QWidget *parent = nullptr);

signals:

public slots:
    void showTableMenu();
    void save();

private:
    QString filePath;
    QString fileCodec;
    QTableView* tableView;
    QStandardItemModel* model;
};

#endif // CSVVIEWER_H
