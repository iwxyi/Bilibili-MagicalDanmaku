#ifndef CRONTIMER_H
#define CRONTIMER_H

#include <QObject>
#include <QTimer>
#include "cronparser.h"

class CronTimer : public QObject
{
    Q_OBJECT
public:
    explicit CronTimer(QObject *parent = nullptr);
    explicit CronTimer(const QString &cronExpression, QObject *parent = nullptr);
    ~CronTimer();

    void setCronExpression(const QString &cronExpression);
    bool isValid() const;
    void start();
    void stop();

signals:
    void timeout();

private slots:
    void scheduleTriggered();

private:
    QTimer *m_timer;
    CronParser m_parser;
    qint64 calculateNextInterval() const;
};

#endif // CRONTIMER_H
