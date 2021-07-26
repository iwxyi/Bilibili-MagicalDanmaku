#ifndef VARIANTVIEWER_H
#define VARIANTVIEWER_H

#include <QObject>
#include <QDialog>
#include <QSettings>
#include <QTableView>
#include <QStandardItemModel>

#define SETTINGS_PREFIX QString("_settings/")
#define COUNTS_PREFIX QString("_counts/")
#define HEAPS_PREFIX QString("_heaps/")

class VariantViewer : public QDialog
{
    Q_OBJECT
public:
    explicit VariantViewer(QString caption, QSettings* settings, QString loopKeyStr, QStringList keys, QSettings* counts, QSettings* heaps, QWidget *parent = nullptr);

signals:

public slots:
    void showTableMenu();

private:
    QTableView* tableView;
    QStandardItemModel* model;
    QSettings* settings;
    QSettings* counts;
    QSettings* heaps;
};

#endif // VARIANTVIEWER_H
