#include "fansarchivesservice.h"
#include "myjson.h"
#include "chatgptutil.h"
#include "fileutil.h"
#include "signaltransfer.h"
#include "accountinfo.h"

#define INTERVAL_WORK 3000
#define INTERVAL_WAIT 60000

FansArchivesService::FansArchivesService(SqlService* sqlService, QObject* parent)
    : QObject(parent), sqlService(sqlService)
{
    timer = new QTimer(this);
    timer->setInterval(INTERVAL_WORK + 3000);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &FansArchivesService::onTimer);
}

void FansArchivesService::start()
{
    timer->setInterval(INTERVAL_WORK);
    timer->start();
}

void FansArchivesService::onTimer()
{
    if (!sqlService || !sqlService->isOpen())
    {
        qWarning() << "数据库未打开，停止处理粉丝档案";
        emit signalError("数据库未打开，停止处理粉丝档案");
        return;
    }
    
    /// 当前进程
    QString uid = sqlService->getNextFansArchive();
    if (uid.isEmpty())
    {
        if (timer->interval() != INTERVAL_WAIT)
        {
            qInfo() << "没有找到需要处理的粉丝档案，暂缓工作";
            emit signalFansArchivesLoadingStatusChanged("档案生成完毕");
        }
        else
        {
            emit signalFansArchivesLoadingStatusChanged("");
        }
        timer->setInterval(INTERVAL_WAIT); // 60秒判断一次
        timer->start();
        return;
    }
    if (uid == "0")
    {
        qWarning() << "使用了'0'作为uid的粉丝档案";
        return;
    }
    qInfo() << "准备处理粉丝档案：" << uid;

    /// 组合输入

    // 获取现有粉丝档案
    MyJson archive = sqlService->getFansArchives(uid);
    QString currentArchive;
    if (!archive.isEmpty())
        currentArchive = archive.s("archive");
    qint64 startTime = 0;
    if (!currentArchive.isEmpty())
        startTime = archive.l("update_time");

    // 获取未处理的弹幕
    QList<MyJson> danmakuList = sqlService->getUserDanmakuList(uid, startTime, 100);
    QList<MyJson> giftList = sqlService->getUserGiftList(uid, startTime, 100);
    if (danmakuList.isEmpty() && giftList.isEmpty())
    {
        qWarning() << "没有找到未处理的弹幕";
        return;
    }

    // 获取身份
    LiveDanmaku lastDanmaku = danmakuList.first();
    QString lastUname = danmakuList.first().s("uname");
    QString identity;
    if (uid == ac->upUid)
        identity = "主播";
    else if (lastDanmaku.isGuard()) // 目前都没有舰长信息的
        identity = lastDanmaku.getGuardName();
    else if (ac->currentGuards.contains(uid))
        identity = "舰长";
    else
        identity = "观众";

    // 去掉时间的秒（后3位）
    // time示例：2024-03-14T16:59:50，去掉秒
    auto toSimpleTime = [](const QString& time) -> QString {
        if (time.length() > 16)
            return time.left(16);
        return time;
    };
    auto toSimpleAmount = [](const QString& amount) -> QString {
        if (amount.endsWith(".0"))
            return amount.left(amount.length() - 2);
        return amount;
    };
    
    QStringList danmakuListStr;
    foreach (MyJson danmaku, danmakuList)
    {
        danmakuListStr.append(toSimpleTime(danmaku.s("create_time")) + " 说：" + danmaku.s("msg"));
    }
    QStringList giftListStr;
    foreach (MyJson gift, giftList)
    {
        giftListStr.append(toSimpleTime(gift.s("create_time")) + " 送了 " + gift.s("gift_name") + "x" + toSimpleAmount(QString::number(gift.i("number"))) + " 共" + toSimpleAmount(QString::number(gift.i("total_coin") / 1000, 'f', 1)) + "元");
    }
    qInfo() << ("参与处理的" + identity + "信息：") << lastUname << "弹幕数量：" << danmakuList.size() << "，礼物数量：" << giftList.size();
    emit signalFansArchivesLoadingStatusChanged("正在生成【" + lastUname + "】的档案，弹幕" + QString::number(danmakuList.size()) + "条，礼物" + QString::number(giftList.size()) + "项");

    // 组合发送的文本
    QString prompt = readTextFile(":/documents/archives_prompt");
    emit st->replaceDanmakuVariables(&prompt, lastDanmaku);

    QString sendText;
    if (!archive.isEmpty())
    {
        sendText += "以下是" + identity + "【" + lastUname + "】当前已有的档案，请根据该档案，结合近期的弹幕内容和送礼信息，更新档案内容并生成新的档案：\n\n```\n" + archive.s("archive") + "```\n\n";
    }

    if (!danmakuListStr.isEmpty())
    {
        sendText += "以下是" + identity + "【" + lastUname + "】近期的弹幕内容，请结合弹幕更新档案内容：\n\n```\n" + danmakuListStr.join("\n") + "\n```\n\n";
    }
    if (!giftListStr.isEmpty())
    {
        sendText += "以下是" + identity + "【" + lastUname + "】近期的礼物内容，请结合礼物更新档案内容：\n\n```\n" + giftListStr.join("\n") + "\n```\n\n";
    }

    /// AI实例
    ChatGPTUtil* chatgpt = new ChatGPTUtil(this);
    chatgpt->setStream(false);
    connect(chatgpt, &ChatGPTUtil::signalResponseError, this, [=](const QByteArray& ba) {
        qCritical() << QString(ba);
    });
    connect(chatgpt, &ChatGPTUtil::finished, this, [=]{
        chatgpt->deleteLater();

        if (!us->fansArchives)
        {
            qInfo() << "粉丝档案功能未开启，停止处理粉丝档案";
            emit signalFansArchivesLoadingStatusChanged("粉丝档案功能已关闭");
            return;
        }

        timer->setInterval(INTERVAL_WORK); // 3秒后继续处理下一个粉丝档案
        timer->start();
    });
    connect(chatgpt, &ChatGPTUtil::signalResponseText, this, [=](const QString& text) {
        qInfo() << "AI生成档案：" << text;
        QString formatText = text;
        if (formatText.startsWith("```") && formatText.endsWith("```"))
        {
            int index = formatText.indexOf("\n");
            if (index != -1)
                formatText = formatText.mid(index + 1);
            index = formatText.lastIndexOf("\n");
            if (index != -1)
                formatText = formatText.left(index);
        }
        // 保存档案
        sqlService->insertFansArchive(uid, lastUname, formatText);
        emit signalFansArchivesUpdated(uid);
        emit signalFansArchivesLoadingStatusChanged("粉丝【" + lastUname + "】档案生成完毕");
    });

    // 生成data
    QList<ChatBean> chats;
    chats.append(ChatBean("system", prompt));
    if (!sendText.isEmpty())
        chats.append(ChatBean("user", sendText));
    chatgpt->getResponse(chats);
}

void FansArchivesService::clearFansArchivesAll()
{
    sqlService->clearFansArchivesAll();
    emit signalFansArchivesUpdated("");
    emit signalFansArchivesLoadingStatusChanged("所有档案已清除");
}

void FansArchivesService::clearFansArchivesByRoomId(const QString& roomId)
{
    sqlService->clearFansArchivesByRoomId(roomId);
    emit signalFansArchivesUpdated("");
    emit signalFansArchivesLoadingStatusChanged("指定直播间档案已清除");
}

void FansArchivesService::clearFansArchivesByNoRoom()
{
    sqlService->clearFansArchivesByNoRoom();
    emit signalFansArchivesUpdated("");
    emit signalFansArchivesLoadingStatusChanged("未指定直播间档案已清除");
}
