#include <QTextBlock>
#include <QLabel>
#include <QPushButton>
#include "dbbrowser.h"
#include "ui_dbbrowser.h"
#include "fileutil.h"
#include "usersettings.h"
#include "interactivebuttonbase.h"

DBBrowser::DBBrowser(SqlService *service, QSettings* settings, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DBBrowser),
    service(service),
    settings(settings)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose, true);
    ui->splitter->restoreState(settings->value("sql/view_splitter_state").toByteArray());
    ui->splitter->restoreGeometry(settings->value("sql/view_splitter_geometry").toByteArray());

    QString codes = settings->value("sql/codes").toString();
    if (codes.isEmpty())
    {
        // 设置一些示例
        codes = readTextFile(":/documents/sql");
    }
    ui->queryEdit->setPlainText(codes);

    int recentTabIndex = us->value("recent/sql_tab", 1).toInt();
    ui->tabWidget->setCurrentIndex(recentTabIndex);
    if (recentTabIndex == 1)
        initVisualList();
}

DBBrowser::~DBBrowser()
{
    // 保存代码
    QString full = ui->queryEdit->toPlainText();
    settings->setValue("sql/codes", full);

    // 保存状态
    settings->setValue("sql/view_splitter_state", ui->splitter->saveState());
    settings->setValue("sql/view_splitter_geometry", ui->splitter->saveGeometry());
    delete ui;
}

void DBBrowser::showQueryResult(QString sql)
{
    sql = sql.trimmed();
    if (sql.trimmed().isEmpty())
        return ;
    qInfo() << "SQL查询：" << sql;

    if (model)
        delete model;
    model = new QSqlQueryModel(this);
    QSqlQuery query(sql);
    model->setQuery(query);
    ui->resultTable->setModel(model);
    ui->resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    if (query.lastError().type() != QSqlError::NoError)
    {
        qWarning() << query.lastError().text();
        QMessageBox::critical(this, "查询出错", query.lastError().text());
    }
}

void DBBrowser::on_execButton_clicked()
{
    QString full = ui->queryEdit->toPlainText();
    settings->setValue("sql/codes", full);

    QString code;
    QTextCursor cursor = ui->queryEdit->textCursor();
    if (cursor.hasSelection()) // 选中代码
    {
        code = cursor.selectedText();
        code = code.replace(" \n", " ").replace("\n", " "); // 允许多行
    }
    else // 当前行
    {
        int pos = cursor.position();
        int left = full.lastIndexOf("\n", pos - 1) + 1;
        int right = full.indexOf("\n", pos);
        if (right == -1)
            right = full.length();
        code = full.mid(left, right - left);
    }
    emit signalProcessVariant(code);

    showQueryResult(code);
}

void DBBrowser::on_resultTable_customContextMenuRequested(const QPoint &pos)
{

}

void DBBrowser::on_tabWidget_currentChanged(int index)
{
    if (index == 1)
    {
        initVisualList();
    }
    us->setValue("recent/sql_tab", index);
}

void DBBrowser::initVisualList()
{
    QString text = ui->queryEdit->toPlainText();
    qInfo() << "初始化可视化列表，缓存：" << (text == cacheCode);
    if (text == cacheCode)
        return;
    cacheCode = text;

    // 清空之前所有的item机器widget
    while (ui->listWidget->count() > 0)
    {
        auto item = ui->listWidget->item(0);
        auto widget = ui->listWidget->itemWidget(item);
        ui->listWidget->takeItem(0);
        widget->deleteLater();
    }

    // 添加单个item及其widget
    auto generalWidget = [&](const QString& title, const QString& code) {
        QWidget* widget = new QWidget(ui->listWidget);
        QLabel* label = new QLabel(title, widget);
        InteractiveButtonBase* btn = new InteractiveButtonBase(QIcon(":/icons/run"), widget);
        btn->setSquareSize();
        QHBoxLayout* layout = new QHBoxLayout(widget);
        layout->addWidget(label);
        layout->addWidget(btn);

        QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
        ui->listWidget->addItem(item);
        ui->listWidget->setItemWidget(item, widget);
        item->setSizeHint(widget->sizeHint());

        connect(btn, &InteractiveButtonBase::clicked, widget, [&, code]{
            QString c = code;
            emit signalProcessVariant(c);
            showQueryResult(c);
        });
        item->setData(Qt::UserRole, code);
    };

    // 遍历代码生成可视化界面
    QStringList sl = text.split("\n", QString::SkipEmptyParts);
    QString title, code;
    for (int i = 0; i < sl.size(); i++)
    {
        QString line = sl.at(i);
        if (line.startsWith("--")) // 注释
        {
            // 上面是代码，添加widget
            if (!code.isEmpty())
            {
                generalWidget(title, code);
                title.clear();
                code.clear();
            }

            if (line.endsWith("--")) // 分割线大标题
            {
                title.clear();
                code.clear();
            }
            else // 下一个注释
            {

                // 连接注释
                if (!title.isEmpty())
                    title += "\n";
                title += line.right(line.length() - 2).trimmed();
            }
        }
        else // 代码
        {
            code += line;
        }
    }

    // 最后一串代码
    if (!code.isEmpty())
    {
        generalWidget(title, code);
        title.clear();
        code.clear();
    }
}

void DBBrowser::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    QString code = item->data(Qt::UserRole).toString();
    if (code.isEmpty())
        return;

    const QString& full = ui->queryEdit->toPlainText();
    int pos = full.indexOf(code);
    if (pos > -1)
    {
        // 切换到源码页面
        ui->tabWidget->setCurrentIndex(0);
        QTextCursor cursor = ui->queryEdit->textCursor();

        //  选中文字
        cursor.setPosition(pos);
        cursor.setPosition(pos + code.length(), QTextCursor::KeepAnchor);
        ui->queryEdit->setTextCursor(cursor);

        // 确保鼠标在中间
        ui->queryEdit->ensureCursorVisible();
        ui->queryEdit->centerCursor();

        // 自动聚焦输入框
        QTimer::singleShot(100, this, [=]{
           ui->queryEdit->setFocus();
        });
    }

}
