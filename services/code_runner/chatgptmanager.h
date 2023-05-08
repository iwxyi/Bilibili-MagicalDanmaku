#ifndef CHATGPTMANAGER_H
#define CHATGPTMANAGER_H

#include <QObject>
#include "livedanmaku.h"
#include "chatgptutil.h"

class ChatGPTManager : public QObject
{
public:
    ChatGPTManager(QObject* parent = nullptr);
};

#endif // CHATGPTMANAGER_H
