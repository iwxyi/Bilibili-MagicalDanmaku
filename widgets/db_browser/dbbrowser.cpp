#include <QTextBlock>
#include "dbbrowser.h"
#include "ui_dbbrowser.h"

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
        // TODO: 设置一些示例
    }
    ui->queryEdit->setPlainText(codes);
}

DBBrowser::~DBBrowser()
{
    settings->setValue("sql/view_splitter_state", ui->splitter->saveState());
    settings->setValue("sql/view_splitter_geometry", ui->splitter->saveGeometry());
    delete ui;
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

    code = code.trimmed();
    if (code.trimmed().isEmpty())
        return ;
    emit signalProcessVariant(code);
    qInfo() << "SQL查询：" << code;

    if (model)
        delete model;
    model = new QSqlQueryModel(this);
    model->setQuery(code);
    ui->resultTable->setModel(model);
    ui->resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void DBBrowser::on_resultTable_customContextMenuRequested(const QPoint &pos)
{

}
