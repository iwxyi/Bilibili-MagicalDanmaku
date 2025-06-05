#include "crontimer.h"
#include <QDebug>

CronTimer::CronTimer(QObject *parent) : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &CronTimer::scheduleTriggered);
}

CronTimer::CronTimer(const QString &cronExpression, QObject *parent)
    : CronTimer(parent)
{
    m_parser = CronParser(cronExpression);
}

CronTimer::~CronTimer()
{
    stop();
}

void CronTimer::setCronExpression(const QString &cronExpression)
{
    m_parser = CronParser(cronExpression);
}

bool CronTimer::isValid() const
{
    return m_parser.isValid();
}

void CronTimer::start()
{
    if (!m_parser.isValid())
    {
        qWarning() << "Cannot start with invalid cron expression";
        return;
    }
    qint64 interval = calculateNextInterval();
    if (interval > 0)
    {
        m_timer->start(interval);
    }
    else
    {
        qWarning() << "Invalid cron expression";
        m_timer->stop();
    }
}

void CronTimer::stop()
{
    m_timer->stop();
}

void CronTimer::scheduleTriggered()
{
    emit timeout();
    start();
}

qint64 CronTimer::calculateNextInterval() const
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime next = m_parser.nextAfter(now);

    if (!next.isValid())
    {
        qWarning() << "Invalid next execution time";
        return -1;
    }

    qint64 ms = now.msecsTo(next);
    qDebug() << "下次时长：" << ms / 1000 << "s";
    return ms;
}
