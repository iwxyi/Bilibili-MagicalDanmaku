#include "fansarchivesservice.h"
#include "myjson.h"
#include "chatgptutil.h"
#include "fileutil.h"

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
    timer->start();
}

void FansArchivesService::onTimer()
{
    if (!sqlService || !sqlService->isOpen())
    {
        qWarning() << "数据库未打开，无法处理粉丝档案";
        emit signalError("数据库未打开，无法处理粉丝档案");
        return;
    }
    
    QString uid = sqlService->getNextFansArchive();
    if (uid.isEmpty())
    {
        if (timer->interval() != INTERVAL_WAIT)
            qInfo() << "没有找到需要处理的粉丝档案，暂缓工作";
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
    QString lastUname;
    if (danmakuList.isEmpty())
    {
        qWarning() << "没有找到未处理的弹幕";
        return;
    }
    QStringList danmakuListStr;
    foreach (MyJson danmaku, danmakuList)
    {
        if (lastUname.isEmpty())
            lastUname = danmaku.s("uname");
        danmakuListStr.append(danmaku.s("create_time") + "  " + danmaku.s("uname") + "：" + danmaku.s("msg"));
    }
    qInfo() << "待处理的用户信息：" << lastUname << "弹幕数量：" << danmakuList.size();

    // 组合发送的文本
    QString prompt = readTextFile(":/documents/archives_prompt");

    QString sendText;
    if (!archive.isEmpty())
    {
        sendText += "以下是当前已有的档案，请根据该档案，结合近期的弹幕内容，更新档案内容并生成新的档案：\n\n" + archive.s("archive");
    }

    sendText += "以下是近期的弹幕内容，请结合弹幕更新档案内容：\n\n```\n" + danmakuListStr.join("\n") + "\n```";

    // AI实例
    ChatGPTUtil* chatgpt = new ChatGPTUtil(this);
    chatgpt->setStream(false);
    connect(chatgpt, &ChatGPTUtil::signalResponseError, this, [=](const QByteArray& ba) {
        qCritical() << QString(ba);
    });
    connect(chatgpt, &ChatGPTUtil::finished, this, [=]{
        chatgpt->deleteLater();

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
    });

    // 生成data
    QList<ChatBean> chats;
    chats.append(ChatBean("system", prompt));
    if (!sendText.isEmpty())
        chats.append(ChatBean("user", sendText));
    chatgpt->getResponse(chats);
}
