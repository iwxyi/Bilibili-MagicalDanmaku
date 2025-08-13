#ifndef NOTICEMANAGERWINDOW_H
#define NOTICEMANAGERWINDOW_H

#include <QWidget>
#include "noticebean.h"

namespace Ui {
class NoticeManagerWindow;
}

class DouyinLiveService;
class SqlService;

/**
 * 抖音通知管理
 * - 连续获取最新的通知，直到用户指定的时间为止
 * - 判断通知的类型、内容
 * - 多种排序，按时间、类型、内容、关键字等
 * - 支持多种过滤条件，按时间、类型、内容、关键字等
 * - 支持多种批量选择、批量操作，包括设为已读、删除、回复、拉黑等等
 * - 将获取到的数据存入数据库，以及将自己的操作也记录到数据库
 */
class NoticeManagerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit NoticeManagerWindow(QWidget *parent = nullptr);
    ~NoticeManagerWindow();

    void setLiveService(DouyinLiveService *liveService);
    void setSqlService(SqlService *sqlService);

public slots:
    void getWebId();
    void getNotice(qint64 startTime);
    void addTableRow(const NoticeInfo &notice);
    void clear();
    void filter();

private slots:
    void on_startButton_clicked();

    void on_earliestTimeEdit_dateTimeChanged(const QDateTime &dateTime);

    void on_filterButton_clicked();

    void on_filterKeyEdit_editingFinished();

private:
    Ui::NoticeManagerWindow *ui;

    DouyinLiveService *liveService = nullptr;
    SqlService *sqlService = nullptr;

    QList<NoticeInfo> noticeList;
    QList<int> selectedRows;
    QList<int> filterTypes;
    QString filterKey;
};

#endif // NOTICEMANAGERWINDOW_H
