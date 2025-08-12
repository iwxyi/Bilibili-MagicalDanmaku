#include "chatservice.h"
#include "accountinfo.h"

ChatService::ChatService(QObject *parent) : QObject(parent)
{
    chatgpt = new ChatGPTManager(this);
    txNlp = new TxNlp(this);
}

void ChatService::setLiveService(LiveServiceBase *service)
{
    chatgpt->setLiveService(service);
}

void ChatService::chat(UIDT uid, QString text, NetStringFunc func)
{
    if (chatPlatform == ChatPlatform::ChatGPT)
    {
        chatgpt->chat(uid, text, func);
    }
    else if (chatPlatform == ChatPlatform::TxNLP)
    {
        txNlp->chat(text, func, ac->danmuLongest);
    }
}

void ChatService::analyze(QStringList texts, NetStringFunc func)
{
    if (chatPlatform == ChatPlatform::ChatGPT)
    {
        chatgpt->analyze(texts, func);
    }
}

bool ChatService::isAnalyzing() const
{
    if (chatPlatform == ChatPlatform::ChatGPT)
        return chatgpt->isAnalyzing();
    return false;
}

void ChatService::clear()
{
    chatgpt->clear();
}
