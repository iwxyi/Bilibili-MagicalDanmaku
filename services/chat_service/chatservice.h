#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <QObject>
#include "tx_nlp.h"
#include "chatgptmanager.h"

enum ChatPlatform
{
    ChatGPT,
    TxNLP
};

class ChatService : public QObject
{
    Q_OBJECT
public:
    explicit ChatService(QObject *parent = nullptr);

    void setLiveService(LiveServiceBase* service);

signals:

public slots:
    void chat(UIDT uid, QString text, NetStringFunc func);
    void clear();

public:
    ChatPlatform chatPlatform;
    ChatGPTManager* chatgpt = nullptr;
    TxNlp* txNlp = nullptr;
};

#endif // CHATSERVICE_H
