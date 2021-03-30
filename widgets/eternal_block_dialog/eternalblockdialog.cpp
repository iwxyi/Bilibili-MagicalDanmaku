#include <QDateTime>
#include <QMenu>
#include <QAction>
#include "eternalblockdialog.h"
#include "ui_eternalblockdialog.h"

EternalBlockDialog::EternalBlockDialog(QList<EternalBlockUser> *users, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EternalBlockDialog),
    users(users)
{
    ui->setupUi(this);

    this->setModal(true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    QStringList sl;
    for (int i = 0; i < users->size(); i++)
    {
        EternalBlockUser user = users->at(i);
        QString text = QString("%2 (%1) [%4] %3").arg(user.uid).arg(user.uname)
                .arg(QDateTime::fromSecsSinceEpoch(user.time).toString("yy-MM-dd hh:mm"))
                .arg(user.upName);
        ui->listWidget->addItem(text);
    }
}

EternalBlockDialog::~EternalBlockDialog()
{
    delete ui;
}

void EternalBlockDialog::on_listWidget_activated(const QModelIndex &index)
{
    if (!index.isValid())
        return ;
    int row = index.row();
    emit signalCancelEternalBlock(users->at(row).uid);
    ui->listWidget->takeItem(row);
}

void EternalBlockDialog::on_listWidget_customContextMenuRequested(const QPoint &)
{
    QListWidgetItem* item = ui->listWidget->currentItem();

    QMenu* menu = new QMenu(this);
    QAction* actionCancelEternalBlock = new QAction("仅取消永久禁言", this);
    QAction* actionCancelBlock = new QAction("取消永久并解除禁言", this);
    int row = ui->listWidget->currentRow();

    if (!item)
    {
        actionCancelEternalBlock->setEnabled(false);
        actionCancelBlock->setEnabled(false);
    }

    menu->addAction(actionCancelEternalBlock);
    menu->addAction(actionCancelBlock);

    connect(actionCancelEternalBlock, &QAction::triggered, this, [=]{
        emit signalCancelEternalBlock(users->at(row).uid);
        ui->listWidget->takeItem(row);
    });
    connect(actionCancelBlock, &QAction::triggered, this, [=]{
        emit signalCancelBlock(users->at(row).uid);
        ui->listWidget->takeItem(row);
    });

    menu->exec(QCursor::pos());
}
