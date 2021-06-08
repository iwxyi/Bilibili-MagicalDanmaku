#include <QHeaderView>
#include "csvviewer.h"
#include "facilemenu.h"
#include "orderplayerwindow.h"
#include "fileutil.h"

CSVViewer::CSVViewer(QString filePath, QWidget *parent) : QDialog(parent)
{
    setModal(false);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(filePath);

    QVBoxLayout* lay = new QVBoxLayout(this);
    tableView = new QTableView(this);
    lay->addWidget(tableView);

    // 设置表格
    this->filePath = filePath;
    model = new QStandardItemModel(tableView);
    tableView->setModel(model);
    tableView->setItemDelegate(new NoFocusDelegate(tableView, model->columnCount()));
    tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 读取数据
    read();

    // 自动调整宽高
    int titleBarHeight = style()->pixelMetric(QStyle::PM_TitleBarHeight);
    int needHeight = tableView->height();
    if (model->rowCount())
    {
        int rowHeight = tableView->rowHeight(0);
        int shouldHeight = rowHeight * model->rowCount();
        shouldHeight = qMax(shouldHeight, needHeight);
        needHeight = qMin(parent->height() - titleBarHeight, shouldHeight);
    }

    int needWidth = tableView->width();
    int shouldWidth = 0;
    for (int i = 0; i < model->columnCount(); i++)
        shouldWidth += tableView->columnWidth(i);
    shouldWidth += tableView->verticalHeader()->width() + tableView->verticalScrollBar()->width();
    shouldWidth = qMax(needWidth, shouldWidth);
    needWidth = qMin(shouldWidth, parent->width());

    this->resize(needWidth, needHeight);
    this->move(parent->geometry().center() - QPoint(needWidth / 2, needHeight / 2 + titleBarHeight));
    tableView->show();

    // 菜单事件
    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tableView, &QTableView::customContextMenuRequested, tableView, [=](const QPoint&) {
        showTableMenu();
    });
}

void CSVViewer::read()
{
    model->clear();
    disconnect(model, &QStandardItemModel::itemChanged, this, &CSVViewer::save);
    QString content = readTextFileAutoCodec(filePath, &fileCodec);
    QStringList lines = content.split("\n", QString::SkipEmptyParts);
    if (lines.size())
    {
        QStringList cells = lines.first().split(",");
        model->setColumnCount(cells.size());
    }
    for (int i = 0; i < lines.size(); i++)
    {
        QStringList cells = lines.at(i).split(",");
        for (int j = 0; j < cells.size(); j++)
        {
            model->setItem(i, j, new QStandardItem(cells.at(j)));
        }
    }
    connect(model, &QStandardItemModel::itemChanged, this, &CSVViewer::save);

    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    QTimer::singleShot(0, [=]{
        tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    });
}

void CSVViewer::save()
{
    QStringList sl;
    for (int row = 0; row < model->rowCount(); row++)
    {
        QStringList l;
        for (int col = 0; col < model->columnCount(); col++)
        {
            auto item = model->item(row, col);
            l.append(item->data(Qt::DisplayRole).toString());
        }
        sl.append(l.join(","));
    }
    QString ss = sl.join("\n");
    writeTextFile(filePath, ss, fileCodec);
}

void CSVViewer::showTableMenu()
{
    QModelIndex index = tableView->currentIndex();
    int row = index.row();

    FacileMenu* menu = new FacileMenu(this);

    menu->addAction("复制选中行", [=]{
        QStringList sl;
        auto selects = tableView->selectionModel()->selectedRows(0);
        for (int i = 0; i < selects.size(); i++)
        {
            int row = selects.at(i).row();
            QStringList l;
            for (int col = 0; col < model->columnCount(); col++)
            {
                auto item = model->item(row, col);
                l.append(item->data(Qt::DisplayRole).toString());
            }
            sl.append(l.join("\t"));
        }
        QString ss = sl.join("\n");
        QApplication::clipboard()->setText(ss);
    });

    menu->addAction("刷新", [=]{
        read();
    });

    menu->split()->addAction("删除行", [=]{
        auto selects = tableView->selectionModel()->selectedRows(0);
        QList<int> deletedRows;

        // 删除值
        for (int i = 0; i < selects.size(); i++)
        {
            int row = selects.at(i).row();
            deletedRows.append(row);
        }

        // 倒序删除项
        std::sort(deletedRows.begin(), deletedRows.end(), [=](int a, int b) { return a > b; });
        for (int i = 0; i < deletedRows.size(); i++)
            model->removeRow(deletedRows.at(i));

    })->disable(row < 0);

    menu->exec();
}

