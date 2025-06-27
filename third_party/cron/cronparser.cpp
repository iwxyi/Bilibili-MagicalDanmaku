#include "cronparser.h"
#include <QRegularExpression>
#include <QDebug>

CronParser::CronParser() : m_isValid(false)
{

}

CronParser::CronParser(const QString &expression) : m_isValid(false)
{
    setCronExpression(expression);
}

void CronParser::setCronExpression(const QString &expression)
{
    m_cronExpression = expression;
    qDebug() << "cronExpression:" << m_cronExpression;
    QStringList parts = expression.split(' ', QString::SkipEmptyParts);
    if (parts.size() != 6)
    {
        qWarning() << "Invalid cron expression format. Expected 6 fields.";
        return;
    }

    m_isValid = seconds.parse(parts[0], 0, 59) &&
                minutes.parse(parts[1], 0, 59) &&
                hours.parse(parts[2], 0, 23) &&
                daysOfMonth.parse(parts[3], 1, 31) &&
                months.parse(parts[4], 1, 12) &&
                daysOfWeek.parse(parts[5], 0, 6); // 0=Sunday
}

bool CronParser::isValid() const
{
    return m_isValid;
}

bool CronParser::CronField::parse(const QString &field, int min, int max)
{
    if (field == "*")
    {
        for (int i = min; i <= max; ++i)
        {
            values.append(i);
        }
        return true;
    }

    QStringList parts = field.split(',');
    for (const QString &part : parts)
    {
        if (part.contains('/'))
        {
            // Handle step values (e.g. */5)
            QStringList stepParts = part.split('/');
            if (stepParts.size() != 2)
                return false;

            QString range = stepParts[0];
            int step = stepParts[1].toInt();
            if (step <= 0)
                return false;

            int start = min;
            int end = max;

            if (range != "*")
            {
                if (range.contains('-'))
                {
                    QStringList rangeParts = range.split('-');
                    if (rangeParts.size() != 2)
                        return false;
                    start = rangeParts[0].toInt();
                    end = rangeParts[1].toInt();
                }
                else
                {
                    start = end = range.toInt();
                }
            }

            for (int i = start; i <= end; i += step)
            {
                if (i >= min && i <= max)
                {
                    values.append(i);
                }
            }
        }
        else if (part.contains('-'))
        {
            // Handle ranges (e.g. 1-5)
            QStringList rangeParts = part.split('-');
            if (rangeParts.size() != 2)
                return false;

            int start = rangeParts[0].toInt();
            int end = rangeParts[1].toInt();

            if (start < min || end > max || start > end)
                return false;

            for (int i = start; i <= end; ++i)
            {
                values.append(i);
            }
        }
        else
        {
            // Single value
            int value = part.toInt();
            if (value < min || value > max)
                return false;
            values.append(value);
        }
    }

    if (values.isEmpty())
        return false;

    // Sort and remove duplicates
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());

    return true;
}

QDateTime CronParser::nextAfter(const QDateTime &after) const
{
    if (!m_isValid)
        return QDateTime();
    return findNext(after, seconds, minutes, hours, daysOfMonth, months, daysOfWeek);
}

QDateTime CronParser::findNext(const QDateTime &after, const CronField &sec, const CronField &min,
                               const CronField &hour, const CronField &day, const CronField &month,
                               const CronField &dayOfWeek)
{
    QDateTime next = after.addSecs(1); // Start checking from next second

    while (true)
    {
        // Check month
        if (!month.values.contains(next.date().month()))
        {
            next = next.addMonths(1);
            next.setDate(QDate(next.date().year(), next.date().month(), 1));
            next.setTime(QTime(0, 0, 0));
            continue;
        }

        // Check day of month and day of week
        bool dayMatch = day.values.contains(next.date().day());
        bool dowMatch = dayOfWeek.values.contains(next.date().dayOfWeek());

        if (!dayMatch && !dowMatch)
        {
            next = next.addDays(1);
            next.setTime(QTime(0, 0, 0));
            continue;
        }

        // Check hour
        if (!hour.values.contains(next.time().hour()))
        {
            next = next.addSecs(3600 - next.time().minute() * 60 - next.time().second());
            continue;
        }

        // Check minute
        if (!min.values.contains(next.time().minute()))
        {
            next = next.addSecs(60 - next.time().second());
            continue;
        }

        // Check second
        if (!sec.values.contains(next.time().second()))
        {
            next = next.addSecs(1);
            continue;
        }

        break;
    }

    return next;
}
