#include <QTextBlock>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QLineEdit>
#include <QFormLayout>
#include "dbbrowser.h"
#include "ui_dbbrowser.h"
#include "fileutil.h"
#include "usersettings.h"
#include "interactivebuttonbase.h"
#include "chatgptutil.h"

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

    // 设置字符集
    QSqlQuery charset(service->getDb());
    charset.exec("SET NAMES utf8mb4");
    
    // 获取数据库
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
        // 替换每一行--结尾的注释
        code = code.replace(QRegularExpression("--.*$"), "");
        // 替换 Unicode 段落分隔符 (U+2029) 为空格，以及可能存在的其他换行符
        code = code.replace(QRegularExpression("\\s*[\\n\\r\u2029]\\s*"), " ");
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

/**
 * 为SQL源码列表创建一个可视化的可直接点击的列表
 */
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
    // 并且给code中的[]变量（多个）添加对应的输入框
    auto generalWidget = [&](const QString& title, const QString& code) {
        QWidget* widget = new QWidget(ui->listWidget);
        QLabel* label = new QLabel(title, widget);
        InteractiveButtonBase* btn = new InteractiveButtonBase(QIcon(":/icons/run"), widget);
        btn->setSquareSize();
        btn->setRadius(5);

        QHBoxLayout* main_hlayout = new QHBoxLayout(widget);

        // 标题
        QVBoxLayout* content_vlayout = new QVBoxLayout();
        content_vlayout->addWidget(label);
        content_vlayout->setAlignment(Qt::AlignLeft);

        // 遍历code，找到[]变量，添加输入框
        // 格式：[变量名:string(默认)/number]
        QRegularExpression re("\\[([^:]+)(?::([^]]+))?\\]");
        QRegularExpressionMatchIterator matchIterator = re.globalMatch(code);
        while (matchIterator.hasNext())
        {
            // 获取下一个变量
            QRegularExpressionMatch match = matchIterator.next();
            QString variable = match.captured(1);
            QString type = match.captured(2);
            if (type.isEmpty())
                type = "string";

            // 添加输入框
            QLabel* label = new QLabel(variable, widget);
            label->setStyleSheet("color: #888888;"); // 设置为灰色字体
            QLineEdit* lineEdit = new QLineEdit(widget);
            QHBoxLayout* hlayout = new QHBoxLayout();
            hlayout->addWidget(label);
            hlayout->addWidget(lineEdit);
            hlayout->addStretch();
            content_vlayout->addLayout(hlayout);

            // 恢复与保存数据
            QString key = "sql_variable/" + variable;
            lineEdit->setText(us->value(key).toString());
            connect(lineEdit, &QLineEdit::textChanged, widget, [&, key, type]{
                us->setValue(key, lineEdit->text());
            });
            lineEdit->setMaximumWidth(200);
        }

        main_hlayout->addLayout(content_vlayout);
        main_hlayout->addStretch();
        main_hlayout->addWidget(btn);

        // 添加到列表
        QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
        ui->listWidget->addItem(item);
        ui->listWidget->setItemWidget(item, widget);
        item->setSizeHint(widget->sizeHint());

        // 点击按钮
        connect(btn, &InteractiveButtonBase::clicked, widget, [&, code]{
            QString c = code;
            // 替换[]变量为输入框的值
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

/**
 * 点击按钮进行询问
 */
void DBBrowser::on_askAIButton_clicked()
{
    QString askText = ui->questionTextEdit->toPlainText();
    if (askText.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请输入要询问的问题");
        return;
    }
    
    // 调用ChatGPTUtil进行询问
    ChatGPTUtil* chatGPTUtil = new ChatGPTUtil(this);
    connect(chatGPTUtil, &ChatGPTUtil::signalResponseText, this, [=](const QString& text){
        QString formatedText = text;
        // 提取```之间的内容
        if (formatedText.contains("```"))
        {
            int start = formatedText.indexOf("```");
            if (start != -1)
            {
                int line = formatedText.indexOf("\n", start + 1);
                if (line != -1)
                    start = line + 1;
            }
            int end = formatedText.indexOf("```", start + 1);
            if (start != -1 && end != -1)
            {
                formatedText = formatedText.mid(start, end - start);
                qInfo() << "设置SQL：" << formatedText;
            }
            else
            {
                qWarning() << "无法提取回答的SQL";
            }
        }
        ui->answerTextEdit->setPlainText(formatedText);

        // 如果是select，则直接执行；否则需要手动执行
        if (formatedText.trimmed().toUpper().startsWith("SELECT"))
        {
            showQueryResult(formatedText);
        }
    });
    connect(chatGPTUtil, &ChatGPTUtil::signalResponseError, this, [=](const QByteArray& error){
        QMessageBox::warning(this, "提示", "查询失败：" + error);
    });
    connect(chatGPTUtil, &ChatGPTUtil::finished, this, [=]{
        ui->askAIButton->setEnabled(true);
        ui->askAIButton->setText("询问AI");
    });
    QList<ChatBean> chats;
    chats.append(ChatBean("system", readTextFile(":/documents/ask_sql_prompt")));
    chats.append(ChatBean("user", askText));
    chatGPTUtil->getResponse(chats);
    ui->askAIButton->setEnabled(false);
    ui->askAIButton->setText("询问中...");
}

