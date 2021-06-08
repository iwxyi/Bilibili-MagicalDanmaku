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

    // 设置数据
    QString content = readTextFileAutoCodec(filePath, &fileCodec);

    model = new QStandardItemModel(tableView);
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

    // 设置数据，调整列宽
    tableView->setModel(model);
    tableView->setItemDelegate(new NoFocusDelegate(tableView, model->columnCount()));
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    QTimer::singleShot(0, [=]{
        tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    });

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

    // 修改事件
    connect(model, &QStandardItemModel::itemChanged, tableView, [=](QStandardItem*) {
        save();
    });

    // 菜单事件
    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tableView, &QTableView::customContextMenuRequested, tableView, [=](const QPoint&) {
        showTableMenu();
    });
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

    menu->addAction("删除行", [=]{
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

void CSVViewer::save()
{

}

