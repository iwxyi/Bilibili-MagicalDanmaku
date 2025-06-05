#ifndef CRONPARSER_H
#define CRONPARSER_H

#include <QDateTime>
#include <QString>
#include <QVector>

class CronParser
{
public:
    explicit CronParser();
    explicit CronParser(const QString &expression);
    void setCronExpression(const QString &expression);
    bool isValid() const;
    QDateTime nextAfter(const QDateTime &after = QDateTime::currentDateTime()) const;

private:
    struct CronField
    {
        QVector<int> values;
        bool parse(const QString &field, int min, int max);
    };

    QString m_cronExpression;
    bool m_isValid;
    CronField seconds;
    CronField minutes;
    CronField hours;
    CronField daysOfMonth;
    CronField months;
    CronField daysOfWeek;

    static QDateTime findNext(const QDateTime &after, const CronField &sec, const CronField &min,
                              const CronField &hour, const CronField &day, const CronField &month,
                              const CronField &dayOfWeek);
};

#endif // CRONPARSER_H
