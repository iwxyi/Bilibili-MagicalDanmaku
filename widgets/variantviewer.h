#ifndef VARIANTVIEWER_H
#define VARIANTVIEWER_H

#include <QObject>
#include <QDialog>
#include <QSettings>
#include <QTableView>
#include <QStandardItemModel>

class VariantViewer : public QDialog
{
    Q_OBJECT
public:
    explicit VariantViewer(QString caption, QSettings* heaps, QString loopKeyStr, QStringList keys, QWidget *parent = nullptr);

signals:

public slots:
    void showTableMenu();

private:
    QTableView* tableView;
    QStandardItemModel* model;
    QSettings* heaps;
};

#endif // VARIANTVIEWER_H
