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
        shouldHeight += tableView->horizontalHeader()->height() + tableView->horizontalScrollBar()->height();
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
    startModify();
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
    endModify();

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
            if (item)
                l.append(item->data(Qt::DisplayRole).toString());
            else
                l.append("");
        }
        sl.append(l.join(","));
    }
    QString ss = sl.join("\n");
    ss += "\n";
    writeTextFile(filePath, ss, fileCodec);
}

void CSVViewer::showTableMenu()
{
    QModelIndex index = tableView->currentIndex();
    int row = index.row();

    FacileMenu* menu = new FacileMenu(this);

    menu->addAction("上方插入行 (&W)", [=]{
        auto selects = tableView->selectionModel()->selectedRows(0);
        QList<int> insertRows;
        for (int i = 0; i < selects.size(); i++)
            insertRows.append(selects.at(i).row());
        std::sort(insertRows.begin(), insertRows.end(), [=](int a, int b) { return a > b; });
        startModify();
        for (int i = 0; i < insertRows.size(); i++)
            model->insertRow(insertRows.at(i));
        endModify();
        save();
    });

    menu->addAction("下方插入行 (&S)", [=]{
        auto selects = tableView->selectionModel()->selectedRows(0);
        QList<int> insertRows;
        for (int i = 0; i < selects.size(); i++)
            insertRows.append(selects.at(i).row());
        std::sort(insertRows.begin(), insertRows.end(), [=](int a, int b) { return a > b; });
        startModify();
        for (int i = 0; i < insertRows.size(); i++)
            model->insertRow(insertRows.at(i) + 1);
        endModify();
        save();
    });

    menu->addAction("复制选中行 (&C)", [=]{
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

    menu->addAction("删除选中行 (&D)", [=]{
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
        startModify();
        for (int i = 0; i < deletedRows.size(); i++)
            model->removeRow(deletedRows.at(i));
        endModify();

        save();
    })->disable(row < 0);

    auto colMenu = menu->addMenu("列操作");

    colMenu->addAction("左边插入列", [=]{
        int currentCol = tableView->currentIndex().column();
        if (currentCol < 0)
            return ;
        startModify();
        model->insertColumn(currentCol);
        endModify();
        save();
    });

    colMenu->addAction("右边插入列", [=]{
        int currentCol = tableView->currentIndex().column();
        if (currentCol < 0)
            return ;
        startModify();
        model->insertColumn(currentCol + 1);
        endModify();
        save();
    });

    colMenu->split()->addAction("删除列", [=]{
        int currentCol = tableView->currentIndex().column();
        if (currentCol < 0)
            return ;
        startModify();
        model->removeColumn(currentCol);
        endModify();
        save();
    });

    menu->split()->addAction("刷新 (&R)", [=]{
        read();
    });

    menu->exec();
}

void CSVViewer::startModify()
{
    disconnect(model, &QStandardItemModel::itemChanged, this, &CSVViewer::save);
}

void CSVViewer::endModify()
{
    connect(model, &QStandardItemModel::itemChanged, this, &CSVViewer::save);
}

