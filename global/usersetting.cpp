#include "usersettings.h"
#include <QRegularExpression>
#include <QDebug>

QString UserSettings::getSubAccountCookie(const QString &arg) const
{
    if (arg.isEmpty())
        return "";
    
    // 主账号索引为0，返回空即可
    // 子账号索引从1开始
    auto findIndex = [&](int index) -> QString {
        if (index == 0)
            return "";
        if (index > 0 && index <= subAccounts.size())
        {
            qDebug() << "通过Index找到子账号" << subAccounts[index - 1].nickname << subAccounts[index - 1].uid;
            return subAccounts[index - 1].cookie;
        }

        qWarning() << "子账号Index超出范围" << index;
        return "";
    };
    
    // 最先判断？，即全部子账号随机
    if (arg == "?" || arg == "？")
    {
        int r = qrand() % subAccounts.size();
        return findIndex(r + 1);
    }
    // 判断??，随机主账号+全部子账号
    if (arg == "??" || arg == "？？")
    {
        int r = qrand() % (subAccounts.size() + 1);
        if (r == 0)
            return "";
        return findIndex(r);
    }
    
    // 先判断Index
    bool ok = false;
    int index = arg.toInt(&ok);
    if (ok)
    {
        return findIndex(index);
    }

    // 再看看index范围，例如 3~5
    static const QRegularExpression re("^(\\d+)\\s*-\\s*(\\d+)$");
    QRegularExpressionMatch match;
    if (arg.indexOf(re, 0, &match) >= 0)
    {
        if (subAccounts.size() == 0)
        {
            qWarning() << "没有子账号";
            return "";
        }
        int start = match.captured(1).toInt();
        int end = match.captured(2).toInt();
        if (start >= 0 && end >= 0 && start <= end && end <= subAccounts.size())
        {
            int r = qrand() % (end - start + 1) + start;
            return findIndex(r);
        }
        qWarning() << "子账号Index范围超出范围" << start << "~" << end;
        return "";
    }

    // 判断 UID 或者 nickname 全匹配
    foreach (SubAccount sub, subAccounts)
    {
        if (sub.uid == arg || sub.nickname == arg)
        {
            qDebug() << "通过UID或Nickname找到子账号" << sub.nickname << sub.uid;
            return sub.cookie;
        }
    }

    // 没有找到的格式
    qWarning() << "没有找到子账号" << arg;
    return "";
}
