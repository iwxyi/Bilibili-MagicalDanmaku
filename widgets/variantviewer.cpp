#include <QDebug>
#include <QHeaderView>
#include "variantviewer.h"
#include "facilemenu.h"
#include "orderplayerwindow.h"

VariantViewer::VariantViewer(QString caption, QSettings *heaps, QString loopKeyStr, QStringList tableFileds, QWidget *parent)
    : QDialog(parent), heaps(heaps)
{
    setModal(false);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setAttribute(Qt::WA_DeleteOnClose, true);

    QRegularExpression loopKeyRe(loopKeyStr);
    QStringList keys = heaps->allKeys();
    QRegularExpressionMatch match;

    QVBoxLayout* lay = new QVBoxLayout(this);
    if (!caption.isEmpty())
    {
        QLabel* label = new QLabel(caption, this);
        label->setAlignment(Qt::AlignCenter);
        lay->addWidget(label);
    }
    tableView = new QTableView(this);
    lay->addWidget(tableView);

    model = new QStandardItemModel(tableView);

    // 获取标题
    QStringList titleNames, titleKeys;
    model->setColumnCount(tableFileds.size());
    int sortCol = -1;
    bool desc = false; // 从小到大false，从大到小true
    for (int tableCol = 0; tableCol < tableFileds.size(); tableCol++)
    {
        QString field = tableFileds.at(tableCol);
        // 分割标题和键
        QString title, key;
        int colonPos = field.indexOf(":");
        if (colonPos == -1)
        {
            title = "";
            key = field;
        }
        else
        {
            title = field.left(colonPos).trimmed();
            key = field.right(field.length() - colonPos - 1).trimmed();
        }
        if (field.isEmpty()) // 如果是空的，则全部忽略掉
            continue;

        model->setHeaderData(tableCol, Qt::Horizontal, title);
        titleNames.append(title);

        // 分割排序
        if (key.endsWith(":<"))
        {
            sortCol = tableCol;
            desc = false;
            key = key.left(key.length() - 2);
        }
        else if (key.endsWith(">"))
        {
            sortCol = tableCol;
            desc = true;
            key = key.left(key.length() - 2);
        }
        titleKeys.append(key);
    }

    // 获取变量值
    int tableRow = 0;
    foreach (auto key, keys)
    {
        if (key.indexOf(loopKeyRe, 0, &match) == -1)
            continue;
        // 匹配到这个key
        QStringList caps = match.capturedTexts();
        // 获取用来循环的KEY：如果有捕获组，则默认第一个；否则默认全部
        QString loopKey = caps.size() > 1 ? caps.at(1) : caps.at(0);
        for (int tableCol = 0; tableCol < titleKeys.size(); tableCol++)
        {
            QString keyExp = titleKeys.at(tableCol);
            bool plain = keyExp.length() >= 2 && keyExp.startsWith("\"") && keyExp.endsWith("\""); // 显示纯文本，而不是读取设置
            if (plain)
            {
                keyExp = keyExp.mid(1, keyExp.length() - 2);
            }
            else
            {
                if (!keyExp.contains("/"))
                    keyExp = "heaps/" + keyExp;
            }

            // 替换 _KEY_
            keyExp.replace("_KEY_", loopKey)
                    .replace("_ID_", loopKey);

            // 替换 _$1_ 等
            if (keyExp.contains("_$"))
            {
                for (int i = 0; i < caps.size(); i++)
                    keyExp.replace("_$" + snum(i) + "_", caps.at(i));
            }

            if (plain)
            {
                model->setItem(tableRow, tableCol, new QStandardItem(keyExp));
            }
            else
            {
                QString val = heaps->value(keyExp, "").toString();
                if (tableCol == sortCol) // 排序，肯定是数值
                {
                    auto item = new QStandardItem();
                    item->setData(val.toLongLong(), Qt::DisplayRole);
                    model->setItem(tableRow, tableCol, item);
                }
                else
                {
                    model->setItem(tableRow, tableCol, new QStandardItem(val));
                }

                model->setData(model->index(tableRow, tableCol), keyExp, Qt::UserRole); // 用来修改的
            }
        }

        tableRow++;
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

    // 排序
    if (sortCol >= 0)
    {
        tableView->sortByColumn(sortCol, desc ? Qt::DescendingOrder : Qt::AscendingOrder);
    }

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
    connect(model, &QStandardItemModel::itemChanged, tableView, [=](QStandardItem* item) {
        QString key = item->data(Qt::UserRole).toString();
        if (!key.isEmpty())
        {
            qInfo() << "修改heaps值:" << key << item->data(Qt::DisplayRole);
            heaps->setValue(key, item->data(Qt::DisplayRole));
        }
    });

    // 菜单事件
    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tableView, &QTableView::customContextMenuRequested, tableView, [=](const QPoint&) {
        showTableMenu();
    });
}

void VariantViewer::showTableMenu()
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

    menu->split()->addAction("删除值", [=]{
        int col = index.column();
        auto item = model->item(row, col);
        QString key = item->data(Qt::UserRole).toString();
        if (!key.isEmpty())
            heaps->remove(key);
        qInfo() << "删除heaps值:" << key << item->data(Qt::DisplayRole);
        item->setData("", Qt::UserRole); // 先取消键，否则下面的留空还是会引发修改值，剩下一个空值键
        item->setData("", Qt::DisplayRole);
    })->disable(row < 0);

    menu->addAction("删除选中行", [=]{
        auto selects = tableView->selectionModel()->selectedRows(0);
        QList<int> deletedRows;

        // 删除值
        for (int i = 0; i < selects.size(); i++)
        {
            int row = selects.at(i).row();
            for (int col = 0; col < model->columnCount(); col++)
            {
                auto item = model->item(row, col);
                QString key = item->data(Qt::UserRole).toString();
                if (!key.isEmpty())
                    heaps->remove(key);
            }
            deletedRows.append(row);
        }

        // 倒序删除项
        std::sort(deletedRows.begin(), deletedRows.end(), [=](int a, int b) { return a > b; });
        for (int i = 0; i < deletedRows.size(); i++)
            model->removeRow(deletedRows.at(i));

    })->disable(row < 0);

    menu->exec();
}
