#include "fansarchivesservice.h"
#include "myjson.h"
#include "chatgptutil.h"

FansArchivesService::FansArchivesService(SqlService* sqlService, QObject* parent)
    : QObject(parent), sqlService(sqlService)
{
    timer = new QTimer(this);
    timer->setInterval(5000);
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
        qInfo() << "没有找到需要处理的粉丝档案，暂缓工作";
        timer->setInterval(60000); // 60秒判断一次
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
    QString prompt = R"(
你是一个专业的观众心理分析师，请根据用户所有历史弹幕内容，按以下维度生成中文字段的纯文本档案（请勿使用Markdown等）：
分析要求：
【基础信息】
* 用户类型：活跃粉丝/潜水观众/节奏带动者
* 互动热度：0-100分（根据发言频率和互动反馈计算）

【性格特征】
1. 核心性格：形容词+证据（如"社牛型（使用58个emoji/平均每句1.2个感叹号）"）  
2. 次要特质：带具体行为的关键词（如"细节控（常追问产品参数）"）  

【兴趣图谱】
- 近期高频兴趣：按时间衰减权重排序（如"3C数码→机械键盘（近7天讨论6次）"）  
- 潜在兴趣：根据关联话题推测（如"讨论键鼠时多次提及二次元角色→可能喜欢IP联名款"）  

【偏好触发点】
> 当主播做以下行为时，该用户互动率提升：
① 使用"卧槽"等标志性口头禅（触发概率+80%）
② 展示限定皮肤（停留时长提升3倍）

【特殊关注】
! 矛盾特征标注（如"70%倾向理性消费，但曾为限量款冲动消费3次"）
! 禁止使用真实个人信息\n\n
    )";

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
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(ba, &error);
        if (error.error == QJsonParseError::NoError)
        {
            QJsonObject json = document.object();
            if (json.contains("error") && json.value("error").isObject())
                json = json.value("error").toObject();
            if (json.contains("message"))
            {
                int code = json.value("code").toInt();
                QString type = json.value("type").toString();
                qCritical() << (json.value("message").toString() + "\n\n错误码：" + QString::number(code) + "  " + type);
            }
            else
            {
                qCritical() << QString(ba);
            }
        }
        else
        {
            qCritical() << QString(ba);
        }
    });
    connect(chatgpt, &ChatGPTUtil::finished, this, [=]{
        chatgpt->deleteLater();

        timer->setInterval(3000); // 3秒后继续处理下一个粉丝档案
        timer->start();
    });
    connect(chatgpt, &ChatGPTUtil::signalResponseText, this, [=](const QString& text) {
        qInfo() << "AI生成档案：" << text;
        // 保存档案
        sqlService->insertFansArchive(uid, lastUname, text);
    });

    // 生成data
    QList<ChatBean> chats;
    chats.append(ChatBean("system", prompt));
    if (!sendText.isEmpty())
        chats.append(ChatBean("user", sendText));
    chatgpt->getResponse(chats);
}
