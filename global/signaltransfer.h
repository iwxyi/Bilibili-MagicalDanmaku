#ifndef SIGNALTRANSFER_H
#define SIGNALTRANSFER_H

#include <QObject>
class LiveDanmaku;

class SignalTransfer : public QObject
{
    Q_OBJECT
public:

signals:
    void replaceDanmakuVariables(QString* text, const LiveDanmaku& danmaku);
};

extern SignalTransfer* st;

#endif // SIGNALTRANSFER_H
