#include "noticemanagerwindow.h"
#include "ui_noticemanagerwindow.h"
#include "runtimeinfo.h"
#include "accountinfo.h"
#include "usersettings.h"
#include "douyin_liveservice.h"
#include "facilemenu.h"
#include <QTableWidget>

NoticeManagerWindow::NoticeManagerWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NoticeManagerWindow)
{
    ui->setupUi(this);

    // 设置窗口
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("抖音通知管理");
    resize(1000, 600);
    
    // 读取设置
    QString s = us->value("notice_manager/earliest_time").toString();
    if (!s.isEmpty())
    {
        QDateTime dateTime = QDateTime::fromString(s, "yyyy-MM-dd HH:mm:ss");
        ui->earliestTimeEdit->setDateTime(dateTime);
    }
    else // 默认7天前
    {
        ui->earliestTimeEdit->setDateTime(QDateTime::currentDateTime().addDays(-7));
    }

    QVariantList filterTypesVariant = us->value("notice_manager/filter_types").toList();
    for (QVariant v : filterTypesVariant) {
        filterTypes.append(v.toInt());
    }
    filterKey = us->value("notice_manager/filter_key").toString();
    ui->filterKeyEdit->setText(filterKey);

    // 设置表头
    QString headers = "ID,时间,类型,描述,作品,操作";
    ui->noticeTable->setColumnCount(headers.split(",").size());
    ui->noticeTable->setHorizontalHeaderLabels(headers.split(","));
    ui->noticeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->noticeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->noticeTable->setSelectionMode(QAbstractItemView::MultiSelection);
}

NoticeManagerWindow::~NoticeManagerWindow()
{
    delete ui;
}

void NoticeManagerWindow::setLiveService(DouyinLiveService *liveService)
{
    this->liveService = liveService;
}

void NoticeManagerWindow::setSqlService(SqlService *sqlService)
{
    this->sqlService = sqlService;
}

void NoticeManagerWindow::on_startButton_clicked()
{
    clear();

    if (ac->cookieWebId.isEmpty())
    {
        getWebId();
    }

    if (!ac->cookieWebId.isEmpty())
    {
        getNotice(QDateTime::currentSecsSinceEpoch());
    }
}

void NoticeManagerWindow::on_earliestTimeEdit_dateTimeChanged(const QDateTime &dateTime)
{
    QString s = dateTime.toString("yyyy-MM-dd HH:mm:ss");
    us->setValue("notice_manager/earliest_time", s);
}

void NoticeManagerWindow::getWebId()
{
    if (ac->browserCookie.isEmpty())
    {
        qWarning() << "未登录，无法获取webid";
        return;
    }

    qInfo() << "获取webid";
    ui->statusLabel->setText("获取WebID...");

    QString url = "https://live.douyin.com";
    QString html = liveService->getToBytes(url);
    QRegularExpression re("\\\\\"user_unique_id\\\\\":\\s*\\\\\"([^\"]+)\\\\\"");
    QRegularExpressionMatch match = re.match(html);
    if (match.hasMatch())
    {
        ac->cookieWebId = match.captured(1);
        qInfo() << "获取到的WebId:" << ac->cookieWebId;
    }
    else
    {
        qWarning() << "获取webid失败" << html.left(500) + "...";
    }
}

/**
 * https://www.douyin.com/aweme/v1/web/notice/?device_platform=webapp&aid=6383&channel=channel_pc_web&is_new_notice=1&is_mark_read=1&notice_group=960&count=10&min_time=0&max_time=0&update_version_code=170400&pc_client_type=1&pc_libra_divert=Mac&support_h265=1&support_dash=1&cpu_core_num=10&version_code=170400&version_name=17.4.0&cookie_enabled=true&screen_width=1512&screen_height=982&browser_language=zh-CN&browser_platform=MacIntel&browser_name=Chrome&browser_version=138.0.0.0&browser_online=true&engine_name=Blink&engine_version=138.0.0.0&os_name=Mac+OS&os_version=10.15.7&device_memory=8&platform=PC&downlink=10&effective_type=4g&round_trip_time=150&webid=7520906524509259318
 * @param startTime 时间戳，单位秒。如果是下一页，这是上一页的最后一条的时间戳
 */
void NoticeManagerWindow::getNotice(qint64 startTime)
{
    qInfo() << "获取通知：startTime=" << startTime;
    ui->statusLabel->setText("正在获取通知：" + QDateTime::fromSecsSinceEpoch(startTime).toString("yyyy-MM-dd HH:mm:ss"));
    const int count = 10;
    QString url = QString("https://www.douyin.com/aweme/v1/web/notice/?device_platform=webapp&aid=6383&channel=channel_pc_web&is_new_notice=1&is_mark_read=1&notice_group=960&count=%1&min_time=%2&max_time=%3&update_version_code=170400&pc_client_type=1&pc_libra_divert=Mac&support_h265=1&support_dash=1&cpu_core_num=10&version_code=170400&version_name=17.4.0&cookie_enabled=true&screen_width=1512&screen_height=982&browser_language=zh-CN&browser_platform=MacIntel&browser_name=Chrome&browser_version=138.0.0.0&browser_online=true&engine_name=Blink&engine_version=138.0.0.0&os_name=Mac+OS&os_version=10.15.7&device_memory=8&platform=PC&downlink=10&effective_type=4g&round_trip_time=150&webid=" + ac->cookieUniqueId)
                    .arg(count).arg(0).arg(startTime);

    // 开始联网
    liveService->get(url, [=](MyJson json){
        qint64 minTime = startTime;
        QJsonArray array = json.a("notice_list_v2");

        for (int i = 0; i < array.size(); i++)
        {
            MyJson item = array[i].toObject();
            qint64 nid = item.l("nid");
            int type = item.i("type"); // 41点赞，33关注，45被@，31评论
            qint64 create_time = item.l("create_time"); // 10位时间戳

            // 更新最小时间
            if (create_time < minTime)
            {
                minTime = create_time;
                if (minTime <= ui->earliestTimeEdit->dateTime().toSecsSinceEpoch())
                {
                    qInfo() << "已获取到最早的时间，结束";
                    break;
                }
            }

            NoticeInfo notice(item);
            noticeList.append(notice);
            addTableRow(notice);
        }

        ui->statusLabel->setText("时间：" + QDateTime::fromSecsSinceEpoch(startTime).toString("yyyy-MM-dd HH:mm:ss") + " 之后的通知获取完毕");
        // 下一页
        if (minTime > ui->earliestTimeEdit->dateTime().toSecsSinceEpoch())
        {
            QTimer::singleShot(1000, this, [=]{
                getNotice(minTime);
            });
        }
    });
}

void NoticeManagerWindow::addTableRow(const NoticeInfo &notice)
{
    // 添加数据
    ui->noticeTable->insertRow(ui->noticeTable->rowCount());
    int row = ui->noticeTable->rowCount() - 1;
    int column = 0;
    auto setItem = [&](const QString &text) {
        QTableWidgetItem *item = new QTableWidgetItem(text.length() > 20 ? text.left(16) + "..." : text);
        ui->noticeTable->setItem(row, column++, item);
    };
    setItem(QString::number(notice.nid));
    setItem(QDateTime::fromSecsSinceEpoch(notice.create_time).toString("MM-dd HH:mm:ss"));
    setItem(notice.getTypeString());
    setItem(notice.toString());
    setItem(notice.aweme.desc);
    setItem("");

    // 是否显示
    if (!filterTypes.isEmpty() && !filterTypes.contains(notice.type))
    {
        ui->noticeTable->setRowHidden(row, true);
    }
}

void NoticeManagerWindow::clear()
{
    noticeList.clear();
    ui->noticeTable->clearContents();
    ui->noticeTable->setRowCount(0);
}

void NoticeManagerWindow::filter()
{
    // 全部显示
    if (filterTypes.isEmpty() && filterKey.isEmpty())
    {
        for (int i = 0; i < ui->noticeTable->rowCount(); i++)
        {
            ui->noticeTable->setRowHidden(i, false);
        }
        return;
    }

    // 隐藏没有选中的类型
    QRegularExpression key(filterKey);
    for (int i = 0; i < ui->noticeTable->rowCount(); i++)
    {
        bool hidden = false;
        if (!filterTypes.isEmpty())
            hidden = hidden || !filterTypes.contains(noticeList[i].type);
        if (!filterKey.isEmpty())
            hidden = hidden || !noticeList[i].contains(key);
        ui->noticeTable->setRowHidden(i, hidden);
    }
}

void NoticeManagerWindow::on_filterButton_clicked()
{
    auto saveFilterTypes = [=]{
        QVariantList filterTypesVariant;
        for (int type : filterTypes) {
            filterTypesVariant.append(type);
        }
        us->setValue("notice_manager/filter_types", filterTypesVariant);
    };
    
    FacileMenu* menu = new FacileMenu(this);
    menu->addAction("全部", [=]{
        filterTypes.clear();
        saveFilterTypes();
    });
    menu->split()->addAction("评论", [=]{
        if (filterTypes.contains(NoticeType_Comment))
            filterTypes.removeAll(NoticeType_Comment);
        else
            filterTypes.append(NoticeType_Comment);
        filter();
        saveFilterTypes();
    })->linger()->autoToggle()->check(filterTypes.contains(NoticeType_Comment));
    menu->addAction("点赞", [=]{
        if (filterTypes.contains(NoticeType_Digg))
            filterTypes.removeAll(NoticeType_Digg);
        else
            filterTypes.append(NoticeType_Digg);
        filter();
        saveFilterTypes();
    })->linger()->autoToggle()->check(filterTypes.contains(NoticeType_Digg));
    menu->addAction("关注", [=]{
        if (filterTypes.contains(NoticeType_Follow))
            filterTypes.removeAll(NoticeType_Follow);
        else
            filterTypes.append(NoticeType_Follow);
        filter();
        saveFilterTypes();
    })->linger()->autoToggle()->check(filterTypes.contains(NoticeType_Follow));
    menu->addAction("被@", [=]{
        if (filterTypes.contains(NoticeType_At))
            filterTypes.removeAll(NoticeType_At);
        else
            filterTypes.append(NoticeType_At);
        filter();
        saveFilterTypes();
    })->linger()->autoToggle()->check(filterTypes.contains(NoticeType_At));
    menu->addAction("推荐", [=]{
        if (filterTypes.contains(NoticeType_Recommend))
            filterTypes.removeAll(NoticeType_Recommend);
        else
            filterTypes.append(NoticeType_Recommend);
        filter();
        saveFilterTypes();
    })->linger()->autoToggle()->check(filterTypes.contains(NoticeType_Recommend));
    menu->exec();
}


void NoticeManagerWindow::on_filterKeyEdit_editingFinished()
{
    filterKey = ui->filterKeyEdit->text();
    filter();
    us->setValue("notice_manager/filter_key", filterKey);
}

