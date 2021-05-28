#ifndef USETTINGS_H
#define USETTINGS_H

#include <QObject>

class USettings : public QObject
{
    Q_OBJECT
public:
    explicit USettings(QObject *parent = nullptr);

signals:

public slots:
};

#endif // USETTINGS_H
