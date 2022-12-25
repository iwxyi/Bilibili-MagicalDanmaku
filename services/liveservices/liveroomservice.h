#ifndef LIVEROOMSERVICE_H
#define LIVEROOMSERVICE_H

#include <QObject>

class LiveRoomService : public QObject
{
    Q_OBJECT
public:
    explicit LiveRoomService(QObject *parent = nullptr);

signals:

public slots:
};

#endif // LIVEROOMSERVICE_H
