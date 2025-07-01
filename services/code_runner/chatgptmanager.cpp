#include "chatgptmanager.h"
#include "runtimeinfo.h"
#include <QMessageBox>

ChatGPTManager::ChatGPTManager(QObject *parent) : QObject(parent)
{
}

void ChatGPTManager::setLiveService(LiveRoomService *service)
{
    this->liveService = service;
}

void ChatGPTManager::chat(UIDT uid, QString text, NetStringFunc func)
{
    /// 初始化ChatGPT
    ChatGPTUtil* chatgpt = new ChatGPTUtil(this);
    chatgpt->setStream(false);
    QString label = "ChatGPT弹幕版";

    connect(chatgpt, &ChatGPTUtil::signalResponseError, this, [=](const QByteArray& ba) {
        qCritical() << QString(ba);
    });

    connect(chatgpt, &ChatGPTUtil::signalRequestStarted, this, [=]{

    });
    connect(chatgpt, &ChatGPTUtil::signalResponseFinished, this, [=]{

    });
    connect(chatgpt, &ChatGPTUtil::finished, this, [=]{
        chatgpt->deleteLater();
    });
    connect(chatgpt, &ChatGPTUtil::signalResponseText, this, [=](const QString& text) {
        func(text);
        if (!usersChats.contains(uid))
            usersChats.insert(uid, QList<ChatBean>());
        usersChats[uid].append(ChatBean("assistant", text));
    });
    connect(chatgpt, &ChatGPTUtil::signalStreamText, this, [=](const QString& text) {

    });

    /// 获取上下文
    QList<ChatBean>& userChats = usersChats[uid];
    userChats.append(ChatBean("user", text));

    QList<ChatBean> chats;
    // 提示词
    if (us->chatgpt_analysis)
    {
        if (!us->chatgpt_analysis_prompt.isEmpty())
        {
            QString rep = us->chatgpt_analysis_prompt;
            rep.replace("%danmu_longest%", snum(ac->danmuLongest));
            chats.append(ChatBean("system", rep));
        }
    }
    else
    {
        if (!us->chatgpt_prompt.isEmpty())
        {
            QString rep = us->chatgpt_prompt;
            rep.replace("%danmu_longest%", snum(ac->danmuLongest));
            chats.append(ChatBean("system", rep));
        }
    }

    // 用户信息
    chats.append(ChatBean("system", QString("用户信息：\nUID=%1").arg(uid)));

    // 历史弹幕
    int count = us->chatgpt_max_context_count;
    for (int i = userChats.size() - 1; i >= 0; i--)
    {
        const ChatBean& chat = userChats.at(i);
        if (chat.role.contains("error")) // 错误
            continue;
        if (us->chatgpt_history_input && chat.role != "user") // 仅包含输入
            continue;
        if (--count <= 0)
            break;
        // 判断token是否超出了

        // 添加记忆
        chats.insert(0, userChats.at(i));
    }

    if (us->chatgpt_analysis && !us->chatgpt_analysis_format.isEmpty())
        chats.last().message = us->chatgpt_analysis_format + "\n" + chats.last().message;

    QJsonObject extraKey;
    extraKey.insert("uid", uid);

    chatgpt->getResponse(chats, extraKey);

    // 清理该UID超过上限的记录
    const int maxCount = qMax(us->chatgpt_max_context_count * 2, us->chatgpt_history_max_count); // 至少保留100条
    auto& uidChats = usersChats[uid];
    while (uidChats.size() > maxCount)
        uidChats.removeFirst();
}

void ChatGPTManager::clear()
{
    usersChats.clear();
}

void ChatGPTManager::localNotify(const QString &text)
{

}
