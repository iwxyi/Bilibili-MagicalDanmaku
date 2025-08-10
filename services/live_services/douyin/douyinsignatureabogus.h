#ifndef DOUYINSIGNATUREABOGUS_H
#define DOUYINSIGNATUREABOGUS_H

#include <QObject>
#include <QJSEngine>
#include <QJSValue>
#include <QString>
#include <QJSValueList>

class DouyinSignatureAbogus : public QObject
{
    Q_OBJECT

public:
    explicit DouyinSignatureAbogus(QObject *parent = nullptr);

    QString signDetail(const QString &params, const QString &userAgent);
    QString signReply(const QString &params, const QString &userAgent);

private:
    QJSEngine *m_engine;
    QJSValue m_signDetailFunc;
    QJSValue m_signReplyFunc;

    bool initializeEngine();
};

#endif // DOUYINSIGNATUREABOGUS_H
