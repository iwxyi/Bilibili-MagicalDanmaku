#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <QObject>
#include "tx_nlp.h"

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

signals:

public slots:

public:
    ChatPlatform chatPlatform;
    TxNlp* txNlp = nullptr;
};

#endif // CHATSERVICE_H
