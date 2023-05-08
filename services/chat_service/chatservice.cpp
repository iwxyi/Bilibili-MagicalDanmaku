#include "chatservice.h"

ChatService::ChatService(QObject *parent) : QObject(parent)
{
    txNlp = new TxNlp(this);
}
