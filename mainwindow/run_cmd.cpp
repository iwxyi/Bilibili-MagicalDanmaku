#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "web_server/webserver.h"
#include "fileutil.h"
#include "conditionutil.h"
#include "tx_nlp.h"
#include "variantviewer.h"
#include "csvviewer.h"
#include "chatgptutil.h"

void MainWindow::processRemoteCmd(QString msg, bool response)
{
    if (!us->remoteControl)
        return ;

    if (msg == "关闭功能")
    {
        processRemoteCmd("关闭欢迎", false);
        processRemoteCmd("关闭关注答谢", false);
        processRemoteCmd("关闭送礼答谢", false);
        processRemoteCmd("关闭禁言", false);
        if (response)
            cr->sendNotifyMsg(">已暂停自动弹幕");
    }
    else if (msg == "开启功能")
    {
        processRemoteCmd("开启欢迎", false);
        processRemoteCmd("开启关注答谢", false);
        processRemoteCmd("开启送礼答谢", false);
        processRemoteCmd("开启禁言", false);
        if (response)
            cr->sendNotifyMsg(">已开启自动弹幕");
    }
    else if (msg == "关闭机器人")
    {
        liveService->liveSocket->abort();
        liveService->connectServerTimer->stop();
    }
    else if (msg == "关闭欢迎")
    {
        ui->autoSendWelcomeCheck->setChecked(false);
        if (response)
            cr->sendNotifyMsg(">已暂停自动欢迎");
    }
    else if (msg == "开启欢迎")
    {
        ui->autoSendWelcomeCheck->setChecked(true);
        if (response)
            cr->sendNotifyMsg(">已开启自动欢迎");
    }
    else if (msg == "关闭关注答谢")
    {
        ui->autoSendAttentionCheck->setChecked(false);
        if (response)
            cr->sendNotifyMsg(">已暂停自动答谢关注");
    }
    else if (msg == "开启关注答谢")
    {
        ui->autoSendAttentionCheck->setChecked(true);
        if (response)
            cr->sendNotifyMsg(">已开启自动答谢关注");
    }
    else if (msg == "关闭送礼答谢")
    {
        ui->autoSendGiftCheck->setChecked(false);
        if (response)
            cr->sendNotifyMsg(">已暂停自动答谢送礼");
    }
    else if (msg == "开启送礼答谢")
    {
        ui->autoSendGiftCheck->setChecked(true);
        if (response)
            cr->sendNotifyMsg(">已开启自动答谢送礼");
    }
    else if (msg == "关闭禁言")
    {
        ui->autoBlockNewbieCheck->setChecked(false);
        on_autoBlockNewbieCheck_clicked();
        if (response)
            cr->sendNotifyMsg(">已暂停新人关键词自动禁言");
    }
    else if (msg == "开启禁言")
    {
        ui->autoBlockNewbieCheck->setChecked(true);
        on_autoBlockNewbieCheck_clicked();
        if (response)
            cr->sendNotifyMsg(">已开启新人关键词自动禁言");
    }
    else if (msg == "关闭偷塔")
    {
        ui->pkAutoMelonCheck->setChecked(false);
        on_pkAutoMelonCheck_clicked();
        if (response)
            cr->sendNotifyMsg(">已暂停自动偷塔");
    }
    else if (msg == "开启偷塔")
    {
        ui->pkAutoMelonCheck->setChecked(true);
        on_pkAutoMelonCheck_clicked();
        if (response)
            cr->sendNotifyMsg(">已开启自动偷塔");
    }
    else if (msg == "关闭点歌")
    {
        ui->DiangeAutoCopyCheck->setChecked(false);
        if (response)
            cr->sendNotifyMsg(">已暂停自动点歌");
    }
    else if (msg == "开启点歌")
    {
        ui->DiangeAutoCopyCheck->setChecked(true);
        if (response)
            cr->sendNotifyMsg(">已开启自动点歌");
    }
    else if (msg == "关闭点歌回复")
    {
        ui->diangeReplyCheck->setChecked(false);
        on_diangeReplyCheck_clicked();
        if (response)
            cr->sendNotifyMsg(">已暂停自动点歌回复");
    }
    else if (msg == "开启点歌回复")
    {
        ui->diangeReplyCheck->setChecked(true);
        on_diangeReplyCheck_clicked();
        if (response)
            cr->sendNotifyMsg(">已开启自动点歌回复");
    }
    else if (msg == "关闭自动连接")
    {
        ui->timerConnectServerCheck->setChecked(false);
        on_timerConnectServerCheck_clicked();
        if (response)
            cr->sendNotifyMsg(">已暂停自动连接");
    }
    else if (msg == "开启自动连接")
    {
        ui->timerConnectServerCheck->setChecked(true);
        on_timerConnectServerCheck_clicked();
        if (response)
            cr->sendNotifyMsg(">已开启自动连接");
    }
    else if (msg == "关闭定时任务")
    {
        for (int i = 0; i < ui->taskListWidget->count(); i++)
        {
            QListWidgetItem* item = ui->taskListWidget->item(i);
            auto widget = ui->taskListWidget->itemWidget(item);
            auto tw = static_cast<TaskWidget*>(widget);
            tw->check->setChecked(false);
        }
        saveTaskList();
        if (response)
            cr->sendNotifyMsg(">已关闭定时任务");
    }
    else if (msg == "开启定时任务")
    {
        for (int i = 0; i < ui->taskListWidget->count(); i++)
        {
            QListWidgetItem* item = ui->taskListWidget->item(i);
            auto widget = ui->taskListWidget->itemWidget(item);
            auto tw = static_cast<TaskWidget*>(widget);
            tw->check->setChecked(true);
        }
        saveTaskList();
        if (response)
            cr->sendNotifyMsg(">已开启定时任务");
    }
    else if (msg == "开启录播")
    {
        startLiveRecord();
        if (response)
            cr->sendNotifyMsg(">已开启录播");
    }
    else if (msg == "关闭录播")
    {
        finishLiveRecord();
        if (response)
            cr->sendNotifyMsg(">已关闭录播");
    }
    else if (msg == "撤销禁言")
    {
        if (!rt->blockedQueue.size())
        {
            cr->sendNotifyMsg(">没有可撤销的禁言用户");
            return ;
        }

        LiveDanmaku danmaku = rt->blockedQueue.takeLast();
        liveService->delBlockUser(danmaku.getUid());
        if (us->eternalBlockUsers.contains(EternalBlockUser(danmaku.getUid(), ac->roomId.toLongLong(), "")))
        {
            us->eternalBlockUsers.removeOne(EternalBlockUser(danmaku.getUid(), ac->roomId.toLongLong(), ""));
            saveEternalBlockUsers();
        }
        if (response)
            cr->sendNotifyMsg(">已解除禁言：" + danmaku.getNickname());
    }
    else if (msg.startsWith("禁言 "))
    {
        QRegularExpression re("^禁言\\s*(\\S+)\\s*(\\d+)?$");
        QRegularExpressionMatch match;
        if (msg.indexOf(re, 0, &match) == -1)
            return ;
        QString nickname = match.captured(1);
        QString hours = match.captured(2);
        int hour = ui->autoBlockTimeSpin->value();
        if (!hours.isEmpty())
            hour = hours.toInt();

    }
    else if (msg.startsWith("解禁 ") || msg.startsWith("解除禁言 ") || msg.startsWith("取消禁言 "))
    {
        QRegularExpression re("^(?:解禁|解除禁言|取消禁言)\\s*(.+)\\s*$");
        QRegularExpressionMatch match;
        if (msg.indexOf(re, 0, &match) == -1)
            return ;
        QString nickname = match.captured(1);

        // 优先遍历禁言的
        for (int i = rt->blockedQueue.size()-1; i >= 0; i--)
        {
            const LiveDanmaku danmaku = rt->blockedQueue.at(i);
            if (!danmaku.is(MSG_DANMAKU))
                continue;

            QString nick = danmaku.getNickname();
            if (nick.contains(nickname))
            {
                if (us->eternalBlockUsers.contains(EternalBlockUser(danmaku.getUid(), ac->roomId.toLongLong(), "")))
                {
                    us->eternalBlockUsers.removeOne(EternalBlockUser(danmaku.getUid(), ac->roomId.toLongLong(), ""));
                    saveEternalBlockUsers();
                }

                liveService->delBlockUser(danmaku.getUid());
                cr->sendNotifyMsg(">已解禁：" + nick, true);
                rt->blockedQueue.removeAt(i);
                return ;
            }
        }

        // 其次遍历弹幕的
        for (int i = liveService->roomDanmakus.size()-1; i >= 0; i--)
        {
            const LiveDanmaku danmaku = liveService->roomDanmakus.at(i);
            if (danmaku.getUid() == 0)
                continue;

            QString nick = danmaku.getNickname();
            if (nick.contains(nickname))
            {
                if (us->eternalBlockUsers.contains(EternalBlockUser(danmaku.getUid(), ac->roomId.toLongLong(), "")))
                {
                    us->eternalBlockUsers.removeOne(EternalBlockUser(danmaku.getUid(), ac->roomId.toLongLong(), ""));
                    saveEternalBlockUsers();
                }

                liveService->delBlockUser(danmaku.getUid());
                cr->sendNotifyMsg(">已解禁：" + nick, true);
                rt->blockedQueue.removeAt(i);
                return ;
            }
        }
    }
    else if (msg.startsWith("永久禁言 "))
    {
        QRegularExpression re("^永久禁言\\s*(\\S+)\\s*$");
        QRegularExpressionMatch match;
        if (msg.indexOf(re, 0, &match) == -1)
            return ;
        QString nickname = match.captured(1);
        for (int i = liveService->roomDanmakus.size()-1; i >= 0; i--)
        {
            const LiveDanmaku danmaku = liveService->roomDanmakus.at(i);
            if (!danmaku.is(MSG_DANMAKU))
                continue;

            QString nick = danmaku.getNickname();
            if (nick.contains(nickname))
            {
                eternalBlockUser(danmaku.getUid(), danmaku.getNickname(), danmaku.getText());
                cr->sendNotifyMsg(">已永久禁言：" + nick, true);
                return ;
            }
        }
    }
    else if (msg == "开启弹幕回复")
    {
        ui->AIReplyMsgCheck->setCheckState(Qt::CheckState::Checked);
        ui->AIReplyCheck->setChecked(true);
        on_AIReplyMsgCheck_clicked();
        if (!danmakuWindow)
            on_actionShow_Live_Danmaku_triggered();
        if (response)
            cr->sendNotifyMsg(">已开启弹幕回复");
    }
    else if (msg == "关闭弹幕回复")
    {
        ui->AIReplyMsgCheck->setChecked(Qt::CheckState::Unchecked);
        on_AIReplyMsgCheck_clicked();
        if (us->value("danmaku/aiReply", false).toBool())
            ui->AIReplyCheck->setChecked(false);
        if (response)
            cr->sendNotifyMsg(">已关闭弹幕回复");
    }
    else
        return ;
    qInfo() << "执行远程命令：" << msg;
}

bool MainWindow::execFunc(QString msg, LiveDanmaku &danmaku, CmdResponse &res, int &resVal)
{
    // 语法糖
    QRegularExpressionMatch match;
    if (ui->syntacticSugarCheck->isChecked())
    {
        // {key} = val
        // {key} += val
        QRegularExpression re("^\\s*\\{(.+?)\\}\\s*(.?)=\\s*(.*)\\s*$");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QString key = match.captured(1);
            QString ope = match.captured(2); // 操作符
            QString val = match.captured(3);
            if (!key.contains("/"))
                key = "heaps/" + key;
            if (ope.isEmpty())
            {
                cr->heaps->setValue(key, val);
                qInfo() << "set value" << key << "=" << val;
            }
            else // 数值运算
            {
                qint64 v = cr->heaps->value(key).toLongLong();
                qint64 x = val.toLongLong();
                if (ope == "+")
                    v += x;
                else if (ope == "-")
                    v -= x;
                else if (ope == "*")
                    v *= x;
                else if (ope == "/")
                {
                    if (x == 0)
                    {
                        showError("错误的/运算", msg);
                        x = 1;
                    }
                    v /= x;
                }
                else if (ope == "%")
                {
                    if (x == 0)
                    {
                        showError("错误的%运算", msg);
                        x = 1;
                    }
                    v /= x;
                }
                cr->heaps->setValue(key, v);
                qInfo() << "set value" << key << "=" << v;
            }
            return true;
        }

        // {key}++  {key}--
        re = QRegularExpression("^\\s*\\{(.+)\\}\\s*(\\+\\+|\\-\\-)\\s*$");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QString key = match.captured(1);
            QString ope = match.captured(2);
            if (!key.contains("/"))
                key = "heaps/" + key;
            qint64 v = cr->heaps->value(key).toLongLong();
            if (ope == "++")
                v++;
            else if (ope == "--")
                v--;
            cr->heaps->setValue(key, v);
            qInfo() << "set value" << key << "=" << v;
            return true;
        }
    }

    // 总体判断判断是不是 >func() 格式的命令
    QRegularExpression re("^\\s*>");
    if (msg.indexOf(re) == -1)
        return false;

    // qDebug() << "尝试执行命令：" << msg;
    auto RE = [=](QString exp) -> QRegularExpression {
        return QRegularExpression("^\\s*>\\s*" + exp + "\\s*$");
    };

    // 过滤器
    if (msg.contains("reject"))
    {
        re = RE("reject\\s*\\(\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            return true;
        }
    }

    // 禁言
    if (msg.contains("block"))
    {
        re = RE("block\\s*\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            qint64 uid = caps.at(1).toLongLong();
            int hour = caps.at(2).toInt();
            QString msg = caps.at(3);
            liveService->addBlockUser(uid, hour, msg);
            return true;
        }
        re = RE("block\\s*\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            qint64 uid = caps.at(1).toLongLong();
            int hour = caps.at(2).toInt();
            liveService->addBlockUser(uid, hour, "");
            return true;
        }
        re = RE("block\\s*\\(\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            qint64 uid = caps.at(1).toLongLong();
            int hour = ui->autoBlockTimeSpin->value();
            liveService->addBlockUser(uid, hour, "");
            return true;
        }
    }

    // 解禁言
    if (msg.contains("unblock"))
    {
        re = RE("unblock\\s*\\(\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            qint64 uid = caps.at(1).toLongLong();
            liveService->delBlockUser(uid);
            return true;
        }
    }

    // 永久禁言
    if (msg.contains("eternalBlock"))
    {
        re = RE("eternalBlock\\s*\\(\\s*(\\d+)\\s*,\\s*(\\S+)\\s*,\\s*(.+*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            qint64 uid = caps.at(1).toLongLong();
            QString uname = caps.at(2);
            QString msg = caps.at(3);
            eternalBlockUser(uid, uname, msg);
            return true;
        }
        re = RE("eternalBlock\\s*\\(\\s*(\\d+)\\s*,\\s*(\\S+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            qint64 uid = caps.at(1).toLongLong();
            QString uname = caps.at(2);
            eternalBlockUser(uid, uname, "");
            return true;
        }
    }

    // 任命房管
    if (msg.contains("appointAdmin"))
    {
        re = RE("appointAdmin\\s*\\(\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            qint64 uid = caps.at(1).toLongLong();
            liveService->appointAdmin(uid);
            return true;
        }
    }

    // 取消房管
    if (msg.contains("dismissAdmin"))
    {
        re = RE("dismissAdmin\\s*\\(\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            qint64 uid = caps.at(1).toLongLong();
            liveService->dismissAdmin(uid);
            return true;
        }
    }

    // 赠送礼物
    if (msg.contains("sendGift"))
    {
        re = RE("sendGift\\s*\\(\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            int giftId = caps.at(1).toInt();
            int num = caps.at(2).toInt();
            liveService->sendGift(giftId, num);
            return true;
        }
    }

    // 终止
    if (msg.contains("abort"))
    {
        re = RE("abort\\s*(\\(\\s*\\))?");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            res = AbortRes;
            return true;
        }
    }

    // 延迟
    if (msg.contains("delay"))
    {
        re = RE("delay\\s*\\(\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            int delay = caps.at(1).toInt(); // 单位：毫秒
            res = DelayRes;
            resVal = delay;
            return true;
        }
    }

    // 添加到游戏用户
    if (msg.contains("addGameUser"))
    {
        re = RE("addGameUser\\s*\\(\\s*(\\d{1,2})\\s*,\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps << cr->gameUsers[0].size();
            int chan = caps.at(1).toInt();
            qint64 uid = caps.at(2).toLongLong();
            if (chan < 0 || chan >= CHANNEL_COUNT)
                chan = 0;
            cr->gameUsers[chan].append(uid);
            return true;
        }

        re = RE("addGameUser\\s*\\(\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps << cr->gameUsers[0].size();
            qint64 uid = caps.at(1).toLongLong();
            cr->gameUsers[0].append(uid);
            return true;
        }
    }

    // 从游戏用户中移除
    if (msg.contains("removeGameUser"))
    {
        re = RE("removeGameUser\\s*\\(\\s*(\\d{1,2})\\s*,\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps << cr->gameUsers[0].size();
            int chan = caps.at(1).toInt();
            qint64 uid = caps.at(2).toLongLong();
            if (chan < 0 || chan >= CHANNEL_COUNT)
                chan = 0;
            cr->gameUsers[chan].removeOne(uid);
            return true;
        }

        re = RE("removeGameUser\\s*\\(\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps << cr->gameUsers[0].size();
            qint64 uid = caps.at(1).toLongLong();
            cr->gameUsers[0].removeOne(uid);
            return true;
        }
    }

    // 添加到游戏数值
    if (msg.contains("addGameNumber"))
    {
        re = RE("addGameNumber\\s*\\(\\s*(\\d{1,2})\\s*,\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps << cr->gameNumberLists[0].size();
            int chan = caps.at(1).toInt();
            qint64 uid = caps.at(2).toLongLong();
            if (chan < 0 || chan >= CHANNEL_COUNT)
                chan = 0;
            cr->gameNumberLists[chan].append(uid);
            saveGameNumbers(chan);
            return true;
        }

        re = RE("addGameNumber\\s*\\(\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps << cr->gameNumberLists[0].size();
            qint64 uid = caps.at(1).toLongLong();
            cr->gameNumberLists[0].append(uid);
            saveGameNumbers(0);
            return true;
        }
    }

    // 从游戏数值中移除
    if (msg.contains("removeGameNumber"))
    {
        re = RE("removeGameNumber\\s*\\(\\s*(\\d{1,2})\\s*,\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps << cr->gameNumberLists[0].size();
            int chan = caps.at(1).toInt();
            qint64 uid = caps.at(2).toLongLong();
            if (chan < 0 || chan >= CHANNEL_COUNT)
                chan = 0;
            cr->gameNumberLists[chan].removeOne(uid);
            saveGameNumbers(chan);
            return true;
        }

        re = RE("removeGameNumber\\s*\\(\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps << cr->gameNumberLists[0].size();
            qint64 uid = caps.at(1).toLongLong();
            cr->gameNumberLists[0].removeOne(uid);
            saveGameNumbers(0);
            return true;
        }
    }

    // 添加到文本数值
    if (msg.contains("addGameText"))
    {
        re = RE("addGameText\\s*\\(\\s*(\\d{1,2})\\s*,\\s*(.+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps << cr->gameTextLists[0].size();
            int chan = caps.at(1).toInt();
            QString text = caps.at(2);
            if (chan < 0 || chan >= CHANNEL_COUNT)
                chan = 0;
            cr->gameTextLists[chan].append(text);
            saveGameTexts(chan);
            return true;
        }

        re = RE("addGameText\\s*\\(\\s*(.+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps << cr->gameTextLists[0].size();
            QString text = caps.at(1);
            cr->gameTextLists[0].append(text);
            saveGameTexts(0);
            return true;
        }
    }

    // 从游戏文本中移除
    if (msg.contains("removeGameText"))
    {
        re = RE("removeGameText\\s*\\(\\s*(\\d{1,2})\\s*,\\s*(.+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps << cr->gameTextLists[0].size();
            int chan = caps.at(1).toInt();
            QString text = caps.at(2);
            if (chan < 0 || chan >= CHANNEL_COUNT)
                chan = 0;
            cr->gameTextLists[chan].removeOne(text);
            saveGameTexts(chan);
            return true;
        }

        re = RE("removeGameText\\s*\\(\\s*(.+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps << cr->gameTextLists[0].size();
            QString text = caps.at(1);
            cr->gameTextLists[0].removeOne(text);
            saveGameTexts(0);
            return true;
        }
    }

    // 执行远程命令
    if (msg.contains("execRemoteCommand"))
    {
        re = RE("execRemoteCommand\\s*\\(\\s*(.+?)\\s*,\\s*(\\d)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString cmd = caps.at(1);
            int response = caps.at(2).toInt();
            qInfo() << "执行命令：" << caps;
            processRemoteCmd(cmd, response);
            return true;
        }

        re = RE("execRemoteCommand\\s*\\(\\s*(.+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString cmd = caps.at(1);
            qInfo() << "执行命令：" << caps;
            processRemoteCmd(cmd);
            return true;
        }
    }

    // 发送私信
    if (msg.contains("sendPrivateMsg"))
    {
        re = RE("sendPrivateMsg\\s*\\(\\s*(\\d+)\\s*,\\s*(.+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qint64 uid = caps.at(1).toLongLong();
            QString msg = caps.at(2);
            qInfo() << "执行命令：" << caps;
            msg = cr->toMultiLine(msg);
            liveService->sendPrivateMsg(snum(uid), msg);
            return true;
        }
    }

    // 发送指定直播间弹幕
    if (msg.contains("sendRoomMsg"))
    {
        re = RE("sendRoomMsg\\s*\\(\\s*(\\d+)\\s*,\\s*(.+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString roomId = caps.at(1);
            QString msg = caps.at(2);
            qInfo() << "执行命令：" << caps;
            if (roomId == ac->roomId)
                cr->addNoReplyDanmakuText(msg);
            liveService->sendRoomMsg(roomId, msg);
            return true;
        }
    }

    // 发送全屏弹幕
    if (msg.contains("showScreenDanmu"))
    {
        re = RE("showScreenDanmu\\s*\\(\\s*(.*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString msg = caps.at(1);
            if (msg.isEmpty())
                return true;
            qInfo() << "执行命令：" << caps;
            LiveDanmaku dmk;
            dmk.setText(msg);
            showScreenDanmaku(dmk);
            return true;
        }
    }

    // 定时操作
    if (msg.contains("timerShot"))
    {
        re = RE("timerShot\\s*\\(\\s*(\\d+)\\s*,\\s*?(.*)\\s*?\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            int time = caps.at(1).toInt();
            QString msg = caps.at(2);
            msg = cr->toRunableCode(cr->toMultiLine(msg));
            QTimer::singleShot(time, this, [=]{
                LiveDanmaku ld = danmaku;
                cr->sendAutoMsg(msg, danmaku);
            });
            return true;
        }
    }

    // 发送本地通知
    if (msg.contains("localNotify"))
    {
        re = RE("localNotify\\s*\\(\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString msg = caps.at(1);
            msg = cr->toMultiLine(msg);
            qInfo() << "执行命令：" << caps;
            localNotify(msg);
            return true;
        }

        re = RE("localNotify\\s*\\((\\d+)\\s*,\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString uid = caps.at(1);
            QString msg = caps.at(2);
            msg = cr->toMultiLine(msg);
            qInfo() << "执行命令：" << caps;
            localNotify(msg, uid.toLongLong());
            return true;
        }
    }

    // 朗读文本
    if (msg.contains("speakText"))
    {
        re = RE("speakText\\s*\\(\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString text = cr->toMultiLine(caps.at(1));
            qInfo() << "执行命令：" << caps;
            voiceService->speakText(text);
            return true;
        }

        /* re = RE("speakTextXfy\\s*\\(\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString text = caps.at(1);
            qInfo() << "执行命令：" << caps;
            if (!xfyTTS)
            {
                VoicePlatform temp = voicePlatform;
                voicePlatform = VoiceXfy;
                initTTS();
                voicePlatform = temp;
            }
            xfyTTS->speakText(text);
            return true;
        } */

        re = RE("speakTextSSML\\s*\\(\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString text = cr->toMultiLine(caps.at(1));
            qInfo() << "执行命令：" << caps;
            if (!voiceService->msTTS)
            {
                VoicePlatform temp = voiceService->voicePlatform;
                voiceService->voicePlatform = VoiceMS;
                voiceService->initTTS();
                voiceService->voicePlatform = temp;
            }
            voiceService->msTTS->speakSSML(text);
            return true;
        }

        re = RE("speakTextUrl\\s*\\(\\s*(.+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString link = caps.at(1).trimmed();
            qInfo() << "执行命令：" << caps;
            voiceService->playNetAudio(link);
            return true;
        }
    }

    if (msg.contains("setVoice"))
    {
        // 发音人
        re = RE("setVoiceSpeaker\\s*\\(\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString text = caps.at(1);
            qInfo() << "执行命令：" << caps;
            ui->voiceNameEdit->setText(text);
            on_voiceNameEdit_editingFinished();
            return true;
        }

        // 音速
        re = RE("setVoiceSpeed\\s*\\(\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            bool ok;
            int val = caps.at(1).toInt(&ok);
            if (!ok)
            {
                showError("setVoiceSpeed", "无法识别的数字：" + caps.at(1));
                return true;
            }
            qInfo() << "执行命令：" << caps;
            ui->voiceSpeedSlider->setValue(val);
            on_voiceSpeedSlider_valueChanged(val);
            return true;
        }

        // 音调
        re = RE("setVoicePitch\\s*\\(\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            bool ok;
            int val = caps.at(1).toInt(&ok);
            if (!ok)
            {
                showError("setVoicePitch", "无法识别的数字：" + caps.at(1));
                return true;
            }
            qInfo() << "执行命令：" << caps;
            ui->voicePitchSlider->setValue(val);
            on_voicePitchSlider_valueChanged(val);
            return true;
        }

        // 音量
        re = RE("setVoiceVolume\\s*\\(\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            bool ok;
            int val = caps.at(1).toInt(&ok);
            if (!ok)
            {
                showError("setVoiceSpeed", "无法识别的数字：" + caps.at(1));
                return true;
            }
            qInfo() << "执行命令：" << caps;
            ui->voiceVolumeSlider->setValue(val);
            on_voiceVolumeSlider_valueChanged(val);
            return true;
        }
    }

    // 连接直播间
    if (msg.contains("connectRoom"))
    {
        re = RE("connectRoom\\s*\\(\\s*(\\w+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString rm = caps.at(1);
            qInfo() << "执行命令：" << caps;
            ui->roomIdEdit->setText(rm);
            on_roomIdEdit_editingFinished();
            return true;
        }
    }

    // 网络操作
    if (msg.contains("openUrl"))
    {
        re = RE("openUrl\\s*\\(\\s*(.+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString url = caps.at(1);
            qInfo() << "执行命令：" << caps;
            openLink(url);
            return true;
        }
    }

    // 后台网络操作
    if (msg.contains("connectNet"))
    {
        re = RE("connectNet\\s*\\(\\s*(.+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString url = caps.at(1);
            get(url, [=](QNetworkReply* reply){
                QByteArray ba(reply->readAll());
                qInfo() << QString(ba);
            });
            return true;
        }
    }
    if (msg.contains("getData"))
    {
        re = RE("getData\\s*\\(\\s*(.+)\\s*,\\s*(\\S*?)\\s*\\)"); // 带参数二
        if (msg.indexOf(re, 0, &match) == -1)
        {
            re = RE("getData\\s*\\(\\s*(.+)\\s*\\)"); // 不带参数二
            if (msg.indexOf(re, 0, &match) == -1)
                return false;
        }
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString url = caps.at(1);
            QString callback = caps.size() > 2 ? caps.at(2) : "";
            get(url, [=](QNetworkReply* reply){
                QByteArray ba(reply->readAll());
                qInfo() << QString(ba);
                if (!callback.isEmpty())
                {
                    LiveDanmaku dmk;
                    dmk.with(MyJson(ba));
                    dmk.setText(ba);
                    triggerCmdEvent(callback, dmk);
                }
            });
            return true;
        }
    }
    if (msg.contains("postData"))
    {
        re = RE("postData\\s*\\(\\s*(.+?)\\s*,\\s*(.*)\\s*,\\s*(\\S+?)\\s*\\)"); // 带参数三
        if (msg.indexOf(re, 0, &match) == -1)
        {
            re = RE("postData\\s*\\(\\s*(.+?)\\s*,\\s*(.*)\\s*\\)"); // 不带参数三
            if (msg.indexOf(re, 0, &match) == -1)
                return false;
        }
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString url = caps.at(1);
            QString data = caps.at(2);
            QString callback = caps.size() > 3 ? caps.at(3) : "";
            data = cr->toMultiLine(data);
            post(url, data.toStdString().data(), [=](QNetworkReply* reply){
                QByteArray ba(reply->readAll());
                qInfo() << QString(ba);
                if (!callback.isEmpty())
                {
                    LiveDanmaku dmk;
                    dmk.with(MyJson(ba));
                    dmk.setText(ba);
                    triggerCmdEvent(callback, dmk);
                }
            });
            return true;
        }
    }
    if (msg.contains("postHeaderData"))
    {
        re = RE("postHeaderData\\s*\\(\\s*(.+?)\\s*,\\s*(.*?)\\s*,\\s*(.*)\\s*,\\s*(\\S+?)\\s*\\)"); // 带参数三
        if (msg.indexOf(re, 0, &match) == -1)
        {
            re = RE("postHeaderData\\s*\\(\\s*(.+?)\\s*,\\s*(.*?)\\s*,\\s*(.*)\\s*\\)"); // 不带参数三
            if (msg.indexOf(re, 0, &match) == -1)
                return false;
        }
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString url = caps.at(1);
            QString headerS = caps.at(2);
            QString data = caps.at(3);
            QString callback = caps.size() > 4 ? caps.at(4) : "";
            data = cr->toMultiLine(data);
            QStringList headers = headerS.split("&", QString::SkipEmptyParts);

            // 开始联网
            QNetworkAccessManager* manager = new QNetworkAccessManager;
            QNetworkRequest* request = new QNetworkRequest(url);
            setUrlCookie(url, request);
            request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
            request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
            for (auto header: headers)
            {
                int find = header.indexOf("=");
                if (find == -1)
                {
                    qWarning() << "错误的 Post Header：" << header;
                    continue;
                }
                QString key = header.left(find);
                QString val = header.right(header.length() - find - 1);
                request->setRawHeader(key.toStdString().data(), val.toStdString().data());
            }
            // 连接槽
            QObject::connect(manager, &QNetworkAccessManager::finished, me, [=](QNetworkReply* reply){
                QByteArray ba(reply->readAll());
                qInfo() << QString(ba);
                if (!callback.isEmpty())
                {
                    LiveDanmaku dmk;
                    dmk.with(MyJson(ba));
                    dmk.setText(ba);
                    triggerCmdEvent(callback, dmk);
                }

                manager->deleteLater();
                delete request;
                reply->deleteLater();
            });

            manager->post(*request, data.toStdString().data());
            return true;
        }
    }
    if (msg.contains("postJson"))
    {
        re = RE("postJson\\s*\\(\\s*(.+?)\\s*,\\s*(.*)\\s*,\\s*(\\S+?)\\s*\\)"); // 带参数三
        if (msg.indexOf(re, 0, &match) == -1)
        {
            re = RE("postJson\\s*\\(\\s*(.+?)\\s*,\\s*(.*)\\s*\\)"); // 不带参数三
            if (msg.indexOf(re, 0, &match) == -1)
                return false;
        }
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString url = caps.at(1);
            QString data = caps.at(2);
            data = cr->toMultiLine(data);
            QString callback = caps.size() > 3 ? caps.at(3) : "";
            postJson(url, data.toStdString().data(), [=](QNetworkReply* reply){
                QByteArray ba(reply->readAll());
                qInfo() << QString(ba);
                if (!callback.isEmpty())
                {
                    LiveDanmaku dmk;
                    dmk.with(MyJson(ba));
                    dmk.setText(ba);
                    triggerCmdEvent(callback, dmk);
                }
            });
            return true;
        }
    }
    if (msg.contains("downloadFile"))
    {
        re = RE("downloadFile\\s*\\(\\s*(.+?)\\s*,\\s*(.+)\\s*,\\s*(\\S+?)\\s*\\)"); // 带参数三
        if (msg.indexOf(re, 0, &match) == -1)
        {
            re = RE("downloadFile\\s*\\(\\s*(.+?)\\s*,\\s*(.+)\\s*\\)"); // 不带参数三
            if (msg.indexOf(re, 0, &match) == -1)
                return false;
        }
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString url = caps.at(1);
            QString path = caps.at(2);
            QString callback = caps.size() > 3 ? caps.at(3) : "";
            get(url, [=](QNetworkReply* reply) {
                QByteArray ba = reply->readAll();
                if (!ba.size())
                {
                    showError("下载文件", "空文件：" + url);
                    return ;
                }

                QFile file(path);
                if (!file.open(QIODevice::WriteOnly))
                {
                    showError("下载文件", "保存失败：" + path);
                    return ;
                }
                file.write(ba);
                file.close();
                if (!callback.isEmpty())
                {
                    LiveDanmaku ld(danmaku);
                    ld.setText(path);
                    triggerCmdEvent(callback, ld);
                }
            });
            return true;
        }
    }

    // 发送socket
    if (msg.contains("sendToSockets"))
    {
        re = RE("sendToSockets\\s*\\(\\s*(\\S+),\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString cmd = caps.at(1);
            QString data = caps.at(2);
            data = cr->toMultiLine(data);

            qInfo() << "执行命令：" << caps;
            sendTextToSockets(cmd, data.toUtf8());
            return true;
        }
    }
    if (msg.contains("sendToLastSocket"))
    {
        re = RE("sendToLastSocket\\s*\\(\\s*(\\S+),\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString cmd = caps.at(1);
            QString data = caps.at(2);
            data = cr->toMultiLine(data);

            qInfo() << "执行命令：" << caps;
            if (webServer->extensionSockets.size())
                sendTextToSockets(cmd, data.toUtf8(), webServer->extensionSockets.last());
            return true;
        }
    }

    // 命令行
    if (msg.contains("runCommandLine"))
    {
        re = RE("runCommandLine\\s*\\(\\s*(.+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString cmd = caps.at(1);
            cmd = cr->toMultiLine(cmd);
            qInfo() << "执行命令：" << caps;
            QProcess p(nullptr);
            p.start(cmd);
            p.waitForStarted();
            p.waitForFinished();
            qInfo() << QString::fromLocal8Bit(p.readAllStandardError());
            return true;
        }
    }

    // 命令行
    if (msg.contains("startProgram"))
    {
        re = RE("startProgram\\s*\\(\\s*(.+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString cmd = caps.at(1);
            cmd = cr->toMultiLine(cmd);
            qInfo() << "执行命令：" << caps;
            QProcess p(nullptr);
            p.startDetached(cmd);
            return true;
        }
    }

    // 打开文件
    if (msg.contains("openFile"))
    {
        re = RE("openFile\\s*\\(\\s*(.+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString path = caps.at(1);
            qInfo() << "执行命令：" << caps;
#ifdef Q_OS_WIN
            path = QString("file:///") + path;
            bool is_open = QDesktopServices::openUrl(QUrl(path, QUrl::TolerantMode));
            if(!is_open)
                qWarning() << "打开文件失败";
#else
            QString  cmd = QString("xdg-open ")+ path; //在linux下，可以通过system来xdg-open命令调用默认程序打开文件；
            system(cmd.toStdString().c_str());
#endif
            return true;
        }
    }

    // 写入文件
    if (msg.contains("writeTextFile"))
    {
        re = RE("writeTextFile\\s*\\(\\s*(.+?)\\s*\\,\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString fileName = cr->toFilePath(caps.at(1));
            QString text = caps.at(2);
            text = cr->toMultiLine(text);
            QFileInfo info(fileName);
            QDir dir = info.absoluteDir();
            dir.mkpath(dir.absolutePath());
            writeTextFile(info.absoluteFilePath(), text);
            qInfo() << "写入文件：" << fileName;
            return true;
        }
    }

    // 写入文件行
    if (msg.contains("appendFileLine"))
    {
        re = RE("appendFileLine\\s*\\(\\s*(.+?)\\s*\\,\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString fileName = cr->toFilePath(caps.at(1));
            QString format = caps.at(2);
            format = cr->toMultiLine(format);
            QFileInfo info(fileName);
            QDir dir = info.absoluteDir();
            dir.mkpath(dir.absolutePath());
            appendFileLine(fileName, format, liveService->lastDanmaku);
            qInfo() << "修改文件：" << fileName;
            return true;
        }
    }

    // 插入文件锚点
    if (msg.contains("insertFileAnchor"))
    {
        re = RE("insertFileAnchor\\s*\\(\\s*(.+?)\\s*,\\s*(.+?)\\s*\\,\\s*(.*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString fileName = cr->toFilePath(caps.at(1));
            QString anchor = caps.at(2);
            QString content = caps.at(3);
            QString text = readTextFile(fileName);
            text.replace(anchor, content + anchor);
            if (!liveService->codeFileCodec.isEmpty())
                writeTextFile(fileName, text, liveService->codeFileCodec);
            else
                writeTextFile(fileName, text);
            qInfo() << "修改文件：" << fileName;
            return true;
        }
    }

    // 移除文件某一行，行数从1开始
    if (msg.contains("removeTextFileLine"))
    {
        re = RE("removeTextFileLine\\s*\\(\\s*(.+?)\\s*,\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString fileName = cr->toFilePath(caps.at(1));
            int line = caps.at(2).toInt();

            QString text = readTextFileAutoCodec(fileName);
            QStringList sl = text.split("\n");
            line--;
            if (line < 0 || line >= sl.size())
            {
                showError("removeTextFileLine", "错误的行数：" + snum(line+1) + "不在1~" + snum(sl.size()) + "之间");
                return true;
            }

            sl.removeAt(line);
            writeTextFile(fileName, sl.join("\n"));
            qInfo() << "修改文件：" << fileName;

            return true;
        }
    }

    // 修改文件行
    if (msg.contains("modifyTextFileLine"))
    {
        re = RE("modifyTextFileLine\\s*\\(\\s*(.+?)\\s*,\\s*(\\d+)\\s*,\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString fileName = cr->toFilePath(caps.at(1));
            int line = caps.at(2).toInt();
            QString newText = caps.at(3);
            if (newText.length() >= 2 && newText.startsWith("\"") && newText.startsWith("\""))
                newText = newText.mid(1, newText.length() - 2);

            QString text = readTextFileAutoCodec(fileName);
            QStringList sl = text.split("\n");
            line--;
            if (line < 0 || line >= sl.size())
            {
                showError("modifyTextFileLine", "错误的行数：" + snum(line+1) + "不在1~" + snum(sl.size()) + "之间");
                return true;
            }

            sl[line] = newText;
            writeTextFile(fileName, sl.join("\n"));
            qInfo() << "修改文件：" << fileName;

            return true;
        }
    }

    // 删除文件
    if (msg.contains("removeFile"))
    {
        re = RE("removeFile\\s*\\(\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString fileName = cr->toFilePath(caps.at(1));
            QFile file(fileName);
            file.remove();
            qInfo() << "删除文件：" << fileName;
            return true;
        }
    }

    // 文件每一行
    if (msg.contains("fileEachLine"))
    {
        re = RE("fileEachLine\\s*\\(\\s*(.+?)\\s*,(?:\\s*(\\d+),)?\\s*(.*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString fileName = cr->toFilePath(caps.at(1));
            QString startLineS = caps.at(2);
            int startLine = 0;
            if (!startLineS.isEmpty())
                startLine = startLineS.toInt();
            QString code = caps.at(3);
            code = cr->toRunableCode(cr->toMultiLine(code));
            QString content = readTextFileAutoCodec(fileName);
            QStringList lines = content.split("\n", QString::SkipEmptyParts);
            for (int i = startLine; i < lines.size(); i++)
            {
                LiveDanmaku dmk = danmaku;
                dmk.setNumber(i+1);
                dmk.setText(lines.at(i));
                QStringList sl = cr->getEditConditionStringList(code, dmk);
                if (!sl.empty())
                    cr->sendAutoMsg(sl.first(), dmk);
            }
            qInfo() << "遍历文件：" << fileName;
            return true;
        }
    }


    // 文件每一行
    if (msg.contains("CSVEachLine") || msg.contains("csvEachLine"))
    {
        re = RE("(?:CSV|csv)EachLine\\s*\\(\\s*(.+?)\\s*,(?:\\s*(\\d+),)?\\s*(.*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString fileName = cr->toFilePath(caps.at(1));
            QString startLineS = caps.at(2);
            int startLine = 0;
            if (!startLineS.isEmpty())
                startLine = startLineS.toInt();
            QString code = caps.at(3);
            code = cr->toRunableCode(cr->toMultiLine(code));
            QString content = readTextFileAutoCodec(fileName);
            QStringList lines = content.split("\n", QString::SkipEmptyParts);
            for (int i = startLine; i < lines.size(); i++)
            {
                LiveDanmaku dmk = danmaku;
                dmk.setNumber(i+1);
                dmk.setText(lines.at(i));

                QStringList li = lines.at(i).split(QRegExp("\\s*,\\s*"));
                li.insert(0, lines.at(i));
                dmk.setArgs(li);

                QStringList sl = cr->getEditConditionStringList(code, dmk);
                if (!sl.empty())
                    cr->sendAutoMsg(sl.first(), dmk);
            }
            qInfo() << "遍历文件：" << fileName;
            return true;
        }
    }

    // 播放音频文件
    if (msg.contains("playSound"))
    {
        re = RE("playSound\\s*\\(\\s*(.+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString path = cr->toFilePath(caps.at(1));
            qInfo() << "执行命令：" << caps;
            QMediaPlayer* player = new QMediaPlayer(this);
            player->setMedia(QUrl::fromLocalFile(path));
            connect(player, &QMediaPlayer::stateChanged, this, [=](QMediaPlayer::State state) {
                if (state == QMediaPlayer::StoppedState)
                {
                    player->deleteLater();
                    triggerCmdEvent("PLAY_SOUND_FINISHED", LiveDanmaku(path));
                }
            });
            player->play();
            qInfo() << "播放音频：" << path;
            return true;
        }
    }

    // 保存到配置
    if (msg.contains("setSetting"))
    {
        re = RE("setSetting\\s*\\(\\s*(\\S+?)\\s*,\\s*(.*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString key = caps.at(1);
            if (!key.contains("/"))
                key = "heaps/" + key;
            QString value = caps.at(2);
            qInfo() << "执行命令：" << caps;
            us->setValue(key, value);
            return true;
        }
    }

    // 删除配置
    if (msg.contains("removeSetting"))
    {
        re = RE("removeSetting\\s*\\(\\s*(\\S+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString key = caps.at(1);
            if (!key.contains("/"))
                key = "heaps/" + key;
            qInfo() << "执行命令：" << caps;

            us->remove(key);
            return true;
        }
    }

    // 保存到heaps
    if (msg.contains("setValue"))
    {
        re = RE("setValue\\s*\\(\\s*(\\S+?)\\s*,\\s*(.*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString key = caps.at(1);
            if (!key.contains("/"))
                key = "heaps/" + key;
            QString value = caps.at(2);
            qInfo() << "执行命令：" << caps;
            cr->heaps->setValue(key, value);
            return true;
        }
    }

    // 添加值
    if (msg.contains("addValue"))
    {
        re = RE("addValue\\s*\\(\\s*(\\S+?)\\s*,\\s*(-?\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString key = caps.at(1);
            if (!key.contains("/"))
                key = "heaps/" + key;
            qint64 value = cr->heaps->value(key).toLongLong();
            qint64 modify = caps.at(2).toLongLong();
            value += modify;
            cr->heaps->setValue(key, value);
            qInfo() << "执行命令：" << caps << key << value;
            return true;
        }
    }

    // 批量修改heaps
    if (msg.contains("setValues"))
    {
        re = RE("setValues\\s*\\(\\s*(\\S+?)\\s*,\\s*(.*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString key = caps.at(1);
            QString value = caps.at(2);
            qInfo() << "执行命令：" << caps;

            cr->heaps->beginGroup("heaps");
            auto keys = cr->heaps->allKeys();
            QRegularExpression re(key);
            for (int i = 0; i < keys.size(); i++)
            {
                if (keys.at(i).indexOf(re) > -1)
                {
                    cr->heaps->setValue(keys.at(i), value);
                }
            }
            cr->heaps->endGroup();
            return true;
        }
    }

    // 批量添加heaps
    if (msg.contains("addValues"))
    {
        re = RE("addValues\\s*\\(\\s*(\\S+?)\\s*,\\s*(-?\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString key = caps.at(1);
            qint64 modify = caps.at(2).toLongLong();
            qInfo() << "执行命令：" << caps;

            cr->heaps->beginGroup("heaps");
            auto keys = cr->heaps->allKeys();
            QRegularExpression re(key);
            for (int i = 0; i < keys.size(); i++)
            {
                if (keys.at(i).indexOf(re) > -1)
                {
                    cr->heaps->setValue(keys.at(i), cr->heaps->value(keys.at(i)).toLongLong() + modify);
                }
            }
            cr->heaps->endGroup();
            return true;
        }
    }

    // 按条件批量修改heaps
    if (msg.contains("setValuesIf"))
    {
        re = RE("setValuesIf\\s*\\(\\s*(\\S+?)\\s*,\\s*\\[(.*?)\\]\\s*,\\s*(.*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString key = caps.at(1);
            QString VAL_EXP = caps.at(2);
            QString newValue = caps.at(3);
            qInfo() << "执行命令：" << caps;

            // 开始修改
            cr->heaps->beginGroup("heaps");
            auto keys = cr->heaps->allKeys();
            QRegularExpression re(key);
            QRegularExpressionMatch match2;
            for (int i = 0; i < keys.size(); i++)
            {
                if (keys.at(i).indexOf(re, 0, &match2) > -1)
                {
                    QString exp = VAL_EXP;
                    // _VALUE_ 替换为 当前key的值
                    exp.replace("_VALUE_", cr->heaps->value(keys.at(i)).toString());
                    // _$1_ 替换为 match的值
                    if (exp.contains("_$"))
                    {
                        auto caps = match2.capturedTexts();
                        for (int i = 0; i < caps.size(); i++)
                            exp.replace("_$" + snum(i) + "_", caps.at(i));
                    }
                    // 替换获取配置的值 _{}_
                    if (exp.contains("_{"))
                    {
                        QRegularExpression re2("_\\{(.*?)\\}_");
                        while (exp.indexOf(re2, 0, &match2) > -1)
                        {
                            QString _var = match2.captured(0);
                            QString key = match2.captured(1);
                            QVariant var = cr->heaps->value(key);
                            exp.replace(_var, var.toString());
                        }
                    }
                    if (ConditionUtil::judgeCondition(exp))
                    {
                        // 处理 newValue
                        if (newValue.contains("_VALUE_"))
                        {
                            // _VALUE_ 替换为 当前key的值
                            newValue.replace("_VALUE_", cr->heaps->value(keys.at(i)).toString());

                            // 替换计算属性 _[]_
                            if (newValue.contains("_["))
                            {
                                QRegularExpression re2("_\\[(.*?)\\]_");
                                while (newValue.indexOf(re2, 0, &match2) > -1)
                                {
                                    QString _var = match2.captured(0);
                                    QString text = match2.captured(1);
                                    text = snum(ConditionUtil::calcIntExpression(text));
                                    newValue.replace(_var, text); // 默认使用变量类型吧
                                }
                            }
                        }

                        // 真正设置
                        cr->heaps->setValue(keys.at(i), newValue);
                    }
                }
            }
            cr->heaps->endGroup();
            return true;
        }
    }

    // 按条件批量添加heaps
    if (msg.contains("addValuesIf"))
    {
        re = RE("addValuesIf\\s*\\(\\s*(\\S+?)\\s*,\\s*\\[(.*?)\\]\\s*,\\s*(.*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString key = caps.at(1);
            QString VAL_EXP = caps.at(2);
            qint64 modify = caps.at(3).toLongLong();
            qInfo() << "执行命令：" << caps;

            // 开始修改
            cr->heaps->beginGroup("heaps");
            auto keys = cr->heaps->allKeys();
            QRegularExpression re(key);
            QRegularExpressionMatch match2;
            for (int i = 0; i < keys.size(); i++)
            {
                if (keys.at(i).indexOf(re, 0, &match2) > -1)
                {
                    QString exp = VAL_EXP;
                    // _VALUE_ 替换为 当前key的值
                    exp.replace("_VALUE_", cr->heaps->value(keys.at(i)).toString());
                    // _$1_ 替换为 match的值
                    if (exp.contains("_$"))
                    {
                        auto caps = match2.capturedTexts();
                        for (int i = 0; i < caps.size(); i++)
                            exp.replace("_$" + snum(i) + "_", caps.at(i));
                    }
                    // 替换获取配置的值 _{}_
                    if (exp.contains("_{"))
                    {
                        QRegularExpression re2("_\\{(.*?)\\}_");
                        while (exp.indexOf(re2, 0, &match2) > -1)
                        {
                            QString _var = match2.captured(0);
                            QString key = match2.captured(1);
                            QVariant var = cr->heaps->value(key);
                            exp.replace(_var, var.toString());
                        }
                    }

                    if (ConditionUtil::judgeCondition(exp))
                    {
                        cr->heaps->setValue(keys.at(i), cr->heaps->value(keys.at(i)).toLongLong() + modify);
                    }
                }
            }
            cr->heaps->endGroup();
            return true;
        }
    }

    // 删除heaps
    if (msg.contains("removeValue"))
    {
        re = RE("removeValue\\s*\\(\\s*(\\S+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString key = caps.at(1);
            if (!key.contains("/"))
                key = "heaps/" + key;
            qInfo() << "执行命令：" << caps;

            cr->heaps->remove(key);
            return true;
        }
    }

    // 批量删除heaps
    if (msg.contains("removeValues"))
    {
        re = RE("removeValues\\s*\\(\\s*(\\S+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString key = caps.at(1);
            qInfo() << "执行命令：" << caps;

            cr->heaps->beginGroup("heaps");
            auto keys = cr->heaps->allKeys();
            QRegularExpression re(key);
            for (int i = 0; i < keys.size(); i++)
            {
                if (keys.at(i).indexOf(re) > -1)
                {
                    cr->heaps->remove(keys.takeAt(i--));
                }
            }
            cr->heaps->endGroup();
            return true;
        }
    }

    // 按条件批量删除heaps
    if (msg.contains("removeValuesIf"))
    {
        re = RE("removeValuesIf\\s*\\(\\s*(\\S+?)\\s*,\\s*\\[(.*)\\]\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString key = caps.at(1);
            QString VAL_EXP = caps.at(2);
            qInfo() << "执行命令：" << caps;

            cr->heaps->beginGroup("heaps");
            auto keys = cr->heaps->allKeys();
            QRegularExpression re(key);
            QRegularExpressionMatch match2;
            for (int i = 0; i < keys.size(); i++)
            {
                if (keys.at(i).indexOf(re, 0, &match2) > -1)
                {
                    QString exp = VAL_EXP;
                    // _VALUE_ 替换为 当前key的值
                    exp.replace("_VALUE_", cr->heaps->value(keys.at(i)).toString());
                    // _$1_ 替换为 match的值
                    if (exp.contains("_$"))
                    {
                        auto caps = match2.capturedTexts();
                        for (int i = 0; i < caps.size(); i++)
                            exp.replace("_$" + snum(i) + "_", caps.at(i));
                    }
                    // 替换获取配置的值 _{}_
                    if (exp.contains("_{"))
                    {
                        QRegularExpression re2("_\\{(.*?)\\}_");
                        while (exp.indexOf(re2, 0, &match2) > -1)
                        {
                            QString _var = match2.captured(0);
                            QString key = match2.captured(1);
                            QVariant var = cr->heaps->value(key);
                            exp.replace(_var, var.toString());
                        }
                    }
                    if (ConditionUtil::judgeCondition(exp))
                    {
                        cr->heaps->remove(keys.takeAt(i--));
                    }
                }
            }
            cr->heaps->endGroup();
            return true;
        }
    }

    // 提升点歌
    if (msg.contains("improveSongOrder") || msg.contains("improveMusic"))
    {
        re = RE("improve(?:SongOrder|Music)\\s*\\(\\s*(.+?)\\s*,\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString uname = caps.at(1);
            int promote = caps.at(2).toInt();
            qInfo() << "执行命令：" << caps;
            if (musicWindow)
            {
                musicWindow->improveUserSongByOrder(uname, promote);
            }
            else
            {
                localNotify("未开启点歌姬");
                qWarning() << "未开启点歌姬";
            }
            return true;
        }
    }

    // 切歌
    if (msg.contains("cutOrderSong") || msg.contains("cutMusic"))
    {
        // 指定用户昵称的切歌
        re = RE("cut(?:OrderSong|Music)\\s*\\(\\s*(.+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString uname = caps.at(1);
            qInfo() << "执行命令：" << caps;
            if (musicWindow)
            {
                if (!musicWindow->cutSongIfUser(uname))
                {
                    const Song &song = musicWindow->getPlayingSong();
                    if (!song.isValid())
                        showError("切歌失败", "未在播放歌曲");
                    else if (song.addBy.isEmpty())
                        showError("切歌失败", "用户不能切手动播放的歌");
                    else
                        showError("切歌失败", "“" + uname + "”无法切“" + song.addBy + "”的歌");
                }
            }
            else
            {
                showError("切歌失败", "未开启点歌姬");
            }
            return true;
        }

        // 强制切歌
        re = RE("cut(?:OrderSong|Music)\\s*\\(\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            if (musicWindow)
            {
                if (!musicWindow->cutSong())
                    showError("切歌失败，未在播放歌曲");
            }
            else
            {
                showError("未开启点歌姬");
            }
            return true;
        }
    }

    // 播放暂停歌曲
    if (msg.contains(QRegExp("(play|pause|toggle)(OrderSong|Music)")))
    {
        re = RE("(play|pause|toggle)(OrderSong|Music)(State)?\\s*\\(\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString action = caps[1];
            qInfo() << "执行命令：" << caps;
            if (musicWindow)
            {
                if (action == "play")
                    musicWindow->play();
                else if (action == "pause")
                    musicWindow->pause();
                else if (action == "toggle")
                    musicWindow->togglePlayState();
            }
            else
            {
                localNotify("未开启点歌姬");
                qWarning() << "未开启点歌姬";
            }
            return true;
        }
    }

    // 提醒框
    if (msg.contains("addMusic") || msg.contains("addOrderSong"))
    {
        re = RE("add(?:OrderSong|Music)\\s*\\(\\s*(.+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString song = caps.at(1);
            qInfo() << "执行命令：" << caps;
            if (musicWindow)
            {
                musicWindow->addMusic(song);
            }
            else
            {
                localNotify("未开启点歌姬");
                qWarning() << "未开启点歌姬";
            }
            return true;
        }
    }

    // 提醒框
    if (msg.contains("msgBox") || msg.contains("messageBox"))
    {
        re = RE("(?:msgBox|messageBox)\\s*\\(\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString text = cr->toMultiLine(caps.at(1));
            qInfo() << "执行命令：" << caps;
            QMessageBox::information(this, "神奇弹幕", text);
            return true;
        }
    }

    // 发送长文本
    if (msg.contains("sendLongText"))
    {
        re = RE("sendLongText\\s*\\(\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString text = caps.at(1);
            qInfo() << "执行命令：" << caps;
            liveService->sendLongText(text);
            return true;
        }
    }

    // 执行自动回复任务
    if (msg.contains("triggerReply"))
    {
        re = RE("triggerReply\\s*\\(\\s*(.+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString text = caps.at(1);
            qInfo() << "执行命令：" << caps;
            bool replyed = false;
            for (int i = 0; i < ui->replyListWidget->count(); i++)
            {
                auto rw = static_cast<ReplyWidget*>(ui->replyListWidget->itemWidget(ui->replyListWidget->item(i)));
                if (rw->triggerIfMatch(text, danmaku))
                    replyed = true;
            }
            if (!replyed)
            {
                qWarning() << "未找到合适的回复动作：" << text;
                showError("triggerReply", "未找到合适的回复动作");
            }
            return true;
        }
    }

    // 开关定时任务: enableTimerTask(id, time)
    // time=1开，0关，>1修改时间
    if (msg.contains("enableTimerTask") || msg.contains("setTimerTask"))
    {
        re = RE("(?:enable|set)TimerTask\\s*\\(\\s*(.+)\\s*,\\s*(-?\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString id = caps.at(1);
            int time = caps.at(2).toInt();
            qInfo() << "执行命令：" << caps;
            bool find = false;
            for (int i = 0; i < ui->taskListWidget->count(); i++)
            {
                auto tw = static_cast<TaskWidget*>(ui->taskListWidget->itemWidget(ui->taskListWidget->item(i)));
                if (!tw->matchId(id))
                    continue;
                if (time == 0) // 切换
                    tw->check->setChecked(!tw->check->isChecked());
                else if (time == 1) // 开
                    tw->check->setChecked(true);
                else if (time == -1) // 关
                    tw->check->setChecked(false);
                else if (time > 1) // 修改时间
                    tw->spin->setValue(time);
                else if (time < -1) // 刷新
                    tw->timer->start();
                find = true;
            }
            if (!find)
                showError("enableTimerTask", "未找到对应ID：" + id);
            return true;
        }
    }

    // 强制AI回复
    if (msg.contains("aiReply") || msg.contains("AIReply"))
    {
        re = RE("(?:ai|AI)Reply\\s*\\(\\s*(.+?)\\s*(?:,\\s*(\\d+))?\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString text = caps.at(1).trimmed();
            if (text.isEmpty())
                return true;
            int maxLen = ui->danmuLongestSpin->value(); // 默认只有一条弹幕的长度
            if (caps.size() > 2 && !caps.at(2).isEmpty()
                    && caps.at(2).toInt() > 0)
                maxLen = caps.at(2).toInt();
            if (caps.size() > 3 && !caps.at(3).isEmpty())
            {
                // 触发事件
                chatService->chat(danmaku.getUid(), text, [=](QString s){
                    QJsonObject js;
                    js.insert("reply", s);
                    LiveDanmaku dm = danmaku;
                    dm.with(js);
                    triggerCmdEvent(caps.at(3), danmaku);
                });
            }
            else
            {
                // 直接发送弹幕
                chatService->chat(danmaku.getUid(), text, [=](QString s){
                    liveService->sendLongText(s);
                });
            }
            return true;
        }
    }

    if (msg.contains("aiChat") || msg.contains("AIChat"))
    {
        re = RE("(?:ai|AI)Chat\\s*\\(\\s*\"(.+?)\"\\s*,\\s*(.+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) == -1)
        {
            re = RE("(?:ai|AI)Chat\\s*\\(\\s*(.+?)\\s*,\\s*(.+)\\s*\\)");
            msg.indexOf(re, 0, &match);
        }

        if (match.isValid())
        {
            if (ui->chatGPTRadio->isChecked())
            {
                msg.replace(QRegularExpression("(?:ai|AI)Chat\\("), "ChatGPT(" + snum(danmaku.getUid()) + ",");
            }
            else
            {
                msg.replace(QRegularExpression("(?:ai|AI)Chat"), "TXChat");
            }
            return execFunc(msg, danmaku, res, resVal);
        }
    }

    if (msg.contains("txChat") || msg.contains("TXChat"))
    {
        re = RE("(?:ai|AI|tx|TX)Chat\\s*\\(\\s*\"(.+?)\"\\s*,\\s*(.+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) == -1)
        {
            re = RE("(?:ai|AI|tx|TX)Chat\\s*\\(\\s*(.+?)\\s*,\\s*(.+)\\s*\\)");
            msg.indexOf(re, 0, &match);
        }

        if (match.isValid())
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString text = caps.at(1).trimmed();
            if (text.isEmpty())
                return true;
            QString code = caps.at(2);
            code = cr->toRunableCode(cr->toMultiLine(code));

            chatService->txNlp->chat(text, [=](QString result) {
                // 过滤回复后的内容
                LiveDanmaku dmk = danmaku;
                dmk.setReply(result);
                if (cr->isFilterRejected("FILTER_AI_REPLY_MSG", dmk))
                {
                    qInfo() << "过滤器已阻止指定文本的AI回复：" << dmk.getText() << dmk.getReply();
                    return;
                }

                dmk.setText(result); // 这个是用来适配旧版，新的已经添加 reply 字段了
                QStringList sl = cr->getEditConditionStringList(code, dmk);
                if (!sl.empty())
                    cr->sendAutoMsg(sl.first(), dmk);
            });
            return true;
        }
    }


    if (msg.contains("ChatGPT") || msg.contains("ChatGPT.chat"))
    {
        re = RE("ChatGPT(?:\\.chat)?\\s*\\(\\s*(\\d*?)\\s*,\\s*(.+?)\\s*,\\s*(.+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            qint64 uid = caps.at(1).toLongLong();
            QString text = caps.at(2).trimmed();
            if (text.isEmpty())
                return true;
            QString code = caps.at(3);
            code = cr->toRunableCode(cr->toMultiLine(code));

            chatService->chatgpt->chat(uid, text, [=](QString result){
                // 处理代码
                if (us->chatgpt_analysis)
                {
                    // 如果开启了功能型GPT，那么回复会是JSON格式
                    QString reply = result;
                    if (!reply.startsWith("{"))
                    {
                        int index = reply.indexOf("{");
                        if (index > -1)
                        {
                            reply = reply.right(reply.length() - index);
                        }
                    }
                    MyJson json(reply.toUtf8());
                    if (json.isEmpty())
                    {
                        qWarning() << "无法解析的GPT回复格式：" << reply.toUtf8();
                        return;
                    }
                    result = json.value("msg").toString();
                }
                else
                {
                    // 过滤回复后的内容
                    LiveDanmaku dmk = danmaku;
                    dmk.setReply(result);
                    if (cr->isFilterRejected("FILTER_AI_REPLY_MSG", dmk))
                    {
                        qInfo() << "过滤器已阻止指定文本的AI回复：" << dmk.getText() << dmk.getReply();
                        return;
                    }
                }

                // 发送弹幕
                LiveDanmaku dmk = danmaku;
                dmk.setText(result);
                qDebug() << code << dmk.toJson();
                QStringList sl = cr->getEditConditionStringList(code, dmk);
                qDebug() << sl;
                if (!sl.empty())
                    cr->sendAutoMsg(sl.first(), dmk);
            });

            return true;
        }
    }

    // 忽略自动欢迎
    if (msg.contains("ignoreWelcome"))
    {
        re = RE("ignoreWelcome\\s*\\(\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qint64 uid = caps.at(1).toLongLong();
            qInfo() << "执行命令：" << caps;

            if (!us->notWelcomeUsers.contains(uid))
            {
                us->notWelcomeUsers.append(uid);

                QStringList ress;
                foreach (qint64 uid, us->notWelcomeUsers)
                    ress << QString::number(uid);
                us->setValue("danmaku/notWelcomeUsers", ress.join(";"));
            }

            return true;
        }
    }

    // 启用欢迎
    if (msg.contains("enableWelcome"))
    {
        re = RE("enableWelcome\\s*\\(\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qint64 uid = caps.at(1).toLongLong();
            qInfo() << "执行命令：" << caps;

            if (us->notWelcomeUsers.contains(uid))
            {
                us->notWelcomeUsers.removeOne(uid);

                QStringList ress;
                foreach (qint64 uid, us->notWelcomeUsers)
                    ress << QString::number(uid);
                us->setValue("danmaku/notWelcomeUsers", ress.join(";"));
            }

            return true;
        }
    }

    // 设置专属昵称
    if (msg.contains("setNickname") || msg.contains("setLocalName"))
    {
        re = RE("set(?:Nickname|LocalName)\\s*\\(\\s*(\\d+)\\s*,\\s*(.*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qint64 uid = caps.at(1).toLongLong();
            QString name = caps.at(2);
            qInfo() << "执行命令：" << caps;
            if (uid == 0)
            {
                showError("setLocalName失败", "未找到UID:" + caps.at(1));
            }
            else if (name.isEmpty()) // 移除
            {
                if (us->localNicknames.contains(uid))
                    us->localNicknames.remove(uid);
            }
            else // 添加
            {
                us->localNicknames[uid] = name;

                QStringList ress;
                auto it = us->localNicknames.begin();
                while (it != us->localNicknames.end())
                {
                    ress << QString("%1=>%2").arg(it.key()).arg(it.value());
                    it++;
                }
                us->setValue("danmaku/localNicknames", ress.join(";"));
            }
            return true;
        }
    }

    // 开启大乱斗
    if (msg.contains("joinBattle"))
    {
        re = RE("joinBattle\\s*\\(\\s*([12])\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            int type = caps.at(1).toInt();
            qInfo() << "执行命令：" << caps;
            liveService->joinBattle(type);
            return true;
        }
    }

    // 自定义事件
    if (msg.contains("triggerEvent") || msg.contains("emitEvent"))
    {
        re = RE("(?:trigger|emit)Event\\s*\\(\\s*(.+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString text = caps.at(1);
            qInfo() << "执行命令：" << caps;
            triggerCmdEvent(text, danmaku);
            return true;
        }
    }

    if (msg.contains("call"))
    {
        re = RE("call\\s*\\(\\s*([^,]+)\\s*,?(.*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString event = caps.at(1);
            QString args = caps.at(2).trimmed();
            QStringList argList = args.split(",", QString::SkipEmptyParts);
            argList.insert(0, caps.at(0));
            qInfo() << "执行命令：" << event << " 参数：" << argList.join(",");
            danmaku.setArgs(argList);
            triggerCmdEvent(event, danmaku);
            return true;
        }
    }

    // 点歌
    if (msg.contains("orderSong"))
    {
        re = RE("orderSong\\s*\\(\\s*(.+)\\s*,\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString text = caps.at(1);
            QString uname = caps.at(2);
            qInfo() << "执行命令：" << caps;
            if (!musicWindow)
                on_actionShow_Order_Player_Window_triggered();
            musicWindow->slotSearchAndAutoAppend(text, uname);
            return true;
        }
    }

    // 模拟快捷键
    if (msg.contains("simulateKeys"))
    {
        re = RE("simulateKeys\\s*\\((.+)\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString text = caps.at(1);
            qInfo() << "执行命令：" << caps;
            cr->simulateKeys(text);
            return true;
        }
    }

    if (msg.contains("simulatePressKeys"))
    {
        re = RE("simulatePressKeys\\s*\\((.+)\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString text = caps.at(1);
            qInfo() << "执行命令：" << caps;
            cr->simulateKeys(text, true, false);
            return true;
        }
    }

    if (msg.contains("simulateReleaseKeys"))
    {
        re = RE("simulateReleaseKeys\\s*\\((.+)\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString text = caps.at(1);
            qInfo() << "执行命令：" << caps;
            cr->simulateKeys(text, false, true);
            return true;
        }
    }

    // 模拟鼠标点击
    if (msg.contains("simulateClick"))
    {
        re = RE("simulateClick\\s*\\(\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            cr->simulateClick();
            return true;
        }

        // 点击绝对位置
        re = RE("simulateClick\\s*\\(\\s*([-\\d]+)\\s*,\\s*([-\\d]+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            unsigned long x = caps.at(1).toLong();
            unsigned long y = caps.at(2).toLong();
            qInfo() << "执行命令：" << caps;
            cr->moveMouseTo(x, y);
            cr->simulateClick();
            return true;
        }
    }

    // 指定按键的模拟
    if (msg.contains("simulateClickButton"))
    {
#ifdef Q_OS_WIN
        auto getFlags = [=](QString param) -> DWORD {
            if (param == "left")
                return MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
            else if (param == "right")
                return MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP;
            else if (param == "middle")
                return MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_MIDDLEUP;
            else if (param == "x")
                return MOUSEEVENTF_XDOWN | MOUSEEVENTF_XUP;
            else if (param.indexOf(QRegExp("^\\d+$")) > -1)
                return param.toUShort();
            else
            {
                qWarning() << "无法识别的鼠标按键：" << param;
                return 0;
            }
        };

        // 点击当前位置
        re = RE("simulateClickButton\\s*\\(\\s*(\\S*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString param = caps[1].toLower();
            qInfo() << "执行命令：" << caps;
            DWORD flag = getFlags(param);
            if (flag == 0)
                return true;

            cr->simulateClickButton(flag);
            return true;
        }

        // 点击绝对位置
        re = RE("simulateClickButton\\s*\\(\\s*(\\S+)\\s*,\\s*([-\\d]+)\\s*,\\s*([-\\d]+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString param = caps[1].toLower();
            unsigned long x = caps.at(2).toLong();
            unsigned long y = caps.at(3).toLong();
            qInfo() << "执行命令：" << caps;
            DWORD flag = getFlags(param);
            if (flag == 0)
                return true;
            cr->moveMouseTo(x, y);
            cr->simulateClickButton(flag);
            return true;
        }
#endif
    }

    // 移动鼠标（相对现在位置）
    if (msg.contains("adjustMouse") || msg.contains("adjustCursor"))
    {
        re = RE("adjust(?:Mouse|Cursor)\\s*\\(\\s*([-\\d]+)\\s*,\\s*([-\\d]+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            unsigned long dx = caps.at(1).toLong();
            unsigned long dy = caps.at(2).toLong();
            qInfo() << "执行命令：" << caps;
            cr->moveMouse(dx, dy);
            return true;
        }
    }

    // 移动鼠标到绝对位置
    if (msg.contains("moveMouse") || msg.contains("moveCursor"))
    {
        re = RE("move(?:Mouse|Cursor)\\s*\\(\\s*([-\\d]+)\\s*,\\s*([-\\d]+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            unsigned long dx = caps.at(1).toLong();
            unsigned long dy = caps.at(2).toLong();
            qInfo() << "执行命令：" << caps;
            cr->moveMouseTo(dx, dy);
            return true;
        }
    }

    // 执行脚本
    if (msg.contains("execScript"))
    {
        re = RE("execScript\\s*\\(\\s*(.+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString text = caps.at(1);
            QString path;
            if (isFileExist(path = rt->dataPath + "control/" + text + ".bat"))
            {
                qInfo() << "执行bat脚本：" << path;
                QProcess p(nullptr);
                p.start(path);
                if (!p.waitForFinished())
                    qWarning() << "执行bat脚本失败：" << path << p.errorString();
            }
            else if (isFileExist(path = rt->dataPath + "control/" + text + ".vbs"))
            {
                qInfo() << "执行vbs脚本：" << path;
                QDesktopServices::openUrl("file:///" + path);
            }
            else if (isFileExist(path = text + ".bat"))
            {
                qInfo() << "执行bat脚本：" << path;
                QProcess p(nullptr);
                p.start(path);
                if (!p.waitForFinished())
                    qWarning() << "执行bat脚本失败：" << path << p.errorString();
            }
            else if (isFileExist(path = text + ".vbs"))
            {
                qInfo() << "执行vbs脚本：" << path;
                QDesktopServices::openUrl("file:///" + path);
            }
            else if (isFileExist(path = text))
            {
                qInfo() << "执行脚本：" << path;
                QDesktopServices::openUrl("file:///" + path);
            }
            else
            {
                qWarning() << "脚本不存在：" << text;
            }
            return true;
        }
    }

    // 添加违禁词（到指定锚点）
    if (msg.contains("addBannedWord"))
    {
        re = RE("addBannedWord\\s*\\(\\s*(.+)\\s*,\\s*(\\S+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString word = caps.at(1);
            QString anchor = caps.at(2);
            cr->addBannedWord(word, anchor);
            return true;
        }
    }

    // 列表值
    // showValueTable(title, loop-key, title1:key1, title2:key2...)
    // 示例：showValueTable(title, integral_(\d+), ID:"_ID_", 昵称:name__ID_, 积分:integral__ID_)
    if (msg.contains("showValueTable"))
    {
        re = RE("showValueTable\\s*\\((.+)\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList tableFileds = match.captured(1).trimmed().split(QRegularExpression("\\s*,\\s*"));
            QString caption, loopKeyStr;
            if (tableFileds.size() >= 2)
                caption = tableFileds.takeFirst();
            if (tableFileds.size() >= 1)
                loopKeyStr = tableFileds.takeFirst();
            if (loopKeyStr.trimmed().isEmpty()) // 关键词是空的，不知道要干嘛
                return true;
            if (!loopKeyStr.contains("/"))
                loopKeyStr = "heaps/" + loopKeyStr;
            QSettings* sts = cr->heaps;
            if (loopKeyStr.startsWith(COUNTS_PREFIX))
            {
                loopKeyStr.remove(0, COUNTS_PREFIX.length());
                sts = us->danmakuCounts;
            }
            else if (loopKeyStr.startsWith(SETTINGS_PREFIX))
            {
                loopKeyStr.remove(0, SETTINGS_PREFIX.length());
                sts = us;
            }

            auto viewer = new VariantViewer(caption, sts, loopKeyStr, tableFileds, us->danmakuCounts, cr->heaps, this);
            viewer->setGeometry(this->geometry());
            viewer->show();
            return true;
        }
    }

    if (msg.contains("showCSV"))
    {
        re = RE("showCSV\\s*\\((.*)\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QString path = match.captured(1);
            auto showCSV = [=](QString path){
                CSVViewer * view = new CSVViewer(path, this);
                view->show();
            };

            if (isFileExist(path))
            {
                showCSV(path);
            }
            else if (isFileExist(rt->dataPath + path))
            {
                showCSV(rt->dataPath + path);
            }
            return true;
        }
    }

    if (msg.contains("execTouta"))
    {
        re = RE("execTouta\\s*\\(\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            if (liveService->pking)
                liveService->execTouta();
            else
                qWarning() << "不在PK中，无法偷塔";
            return true;
        }
    }

    if (msg.contains("setToutaMaxGold"))
    {
        re = RE("setToutaMaxGold\\s*\\(\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            int val = caps.at(1).toInt();
            us->setValue("pk/maxGold", liveService->pkMaxGold = val);
            return true;
        }
    }

    if (msg.contains("copyText"))
    {
        re = RE("copyText\\s*\\((.+?)\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QApplication::clipboard()->setText(cr->toMultiLine(caps.at(1)));
            return true;
        }
    }

    if (msg.contains("setRoomTitle"))
    {
        re = RE("setRoomTitle\\s*\\((.+?)\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            liveService->myLiveSetTitle(caps.at(1));
            return true;
        }
    }

    if (msg.contains("setRoomCover"))
    {
        re = RE("setRoomCover\\s*\\((.+?)\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            liveService->myLiveSetCover(caps.at(1));
            return true;
        }
    }

    if (msg.contains("setLocalMode"))
    {
        re = RE("setLocalMode\\s*\\((.+?)\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString val = caps[1];
            qInfo() << "执行命令：" << caps;
            if (val == "" || val == "0" || val.toLower() == "false")
            {
                // true -> false
                ui->actionLocal_Mode->setChecked(false);
            }
            else
            {
                // false -> true
                ui->actionLocal_Mode->setChecked(true);
            }
            on_actionLocal_Mode_triggered();
            return true;
        }
    }

    if (msg.contains("reconnectRoom"))
    {
        re = RE("reconnectRoom\\s*\\(\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            startConnectRoom();
            return true;
        }
    }

    // 执行数据库
    if (msg.contains("sqlExec") || msg.contains("SQLExec"))
    {
        re = RE("(?:sql|SQL)Exec\\s*\\(\\s*(.+?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString sql = caps[1];
            sql = cr->toMultiLine(sql);
            qInfo() << "执行命令：" << caps;
            if (sqlService.exec(sql))
                localNotify("数据库执行成功：" + sql);
            return true;
        }
    }

    // 显示数据库查询结果
    if (msg.contains("sqlQuery") || msg.contains("SQLQuery"))
    {
        re = RE("(?:sql|SQL)Query\\s*\\((.+?)\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString sql = caps[1];
            sql = cr->toMultiLine(sql);
            qInfo() << "执行命令：" << caps;
            showSqlQueryResult(sql);
            return true;
        }
    }

    // 图片相关的
    if (msg.contains("saveScreenShot"))
    {
        re = RE("saveScreenShot\\s*\\(\\s*(\\d+)\\s*,\\s*(\\-?\\d+)\\s*,\\s*(\\-?\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(.+)\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            int id = caps.at(1).toInt();
            int x = caps.at(2).toInt();
            int y = caps.at(3).toInt();
            int w = caps.at(4).toInt();
            int h = caps.at(5).toInt();
            QString path = caps.at(6);
            auto screens = QGuiApplication::screens();
            if (id < 0 || id >= screens.size())
            {
                showError("保存屏幕截图", "错误的屏幕ID：" + snum(id));
                return true;
            }
            QScreen *screen = screens.at(id);
            QImage image = screen->grabWindow(QApplication::desktop()->winId(), x, y, w, h).toImage();

            QFileInfo info(path);
            QDir dir = info.absoluteDir();
            dir.mkpath(dir.absolutePath());
            image.save(path);
            if (cr->cacheImages.contains(path))
            {
                cr->cacheImages[path] = image;
            }
            qInfo() << "保存截图：" << QRect(x, y, w, h) << screen->geometry() << path;
            return true;
        }
    }

    // windows API
    if (msg.contains("showWindow"))
    {
        re = RE("showWindow\\s*\\(\\s*(\\d*)\\s*,\\s*(\\S*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            QString pointer = caps[1];
            QString typer = caps[2].toLower();
            qInfo() << "执行命令：" << caps;
#ifdef Q_OS_WIN
            int type = SW_MAXIMIZE;
            bool ok = false;
            if (typer.contains("hide"))
                type = SW_HIDE;
            else if (typer.contains("normal"))
                type = SW_NORMAL;
            else if (typer.contains("max"))
                type = SW_MAXIMIZE;
            else if (typer.contains("show"))
                type = SW_SHOW;
            else if (typer.contains("min"))
                type = SW_MINIMIZE;
            else if (typer.contains("restore"))
                type = SW_RESTORE;
            else if (typer.contains("default"))
                type = SW_SHOWDEFAULT;
            else if (typer.toInt(&ok) || ok)
                type = typer.toInt();
            HWND hWnd = (HWND)pointer.toLongLong();
            if (hWnd == nullptr)
            {
                showError("showWindow", "找不到该窗口：" + pointer);
                return true;
            }
            ShowWindow(hWnd, type);
#endif
            return true;
        }
    }

    if (msg.contains("sendWindowMessage")) // 需要窗口的输入框获取焦点才能进行输入
    {
        re = RE("sendWindowMessage\\s*\\(\\s*(\\d*)\\s*,\\s*(.*?)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString pointer = caps[1];
            QString text = cr->toMultiLine(caps[2]);
#ifdef Q_OS_WIN
            HWND hWnd = (HWND)pointer.toLongLong();
            if (hWnd == nullptr)
            {
                showError("sendWindowMessage", "找不到该窗口：" + pointer);
                return true;
            }
            for(QChar c: text)
            {
                int v_latin = c.unicode(); // 对应的code码
                SendMessageW(hWnd,WM_IME_CHAR,(WPARAM)v_latin,(LPARAM)v_latin);
            }
#endif
            return true;
        }
    }

    if (msg.contains("moveWindow"))
    {
        re = RE("moveWindow\\s*\\(\\s*(\\d*)\\s*,\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString pointer = caps[1];
#ifdef Q_OS_WIN
            HWND hWnd = (HWND)pointer.toLongLong();
            if (hWnd == nullptr)
            {
                showError("moveWindow", "找不到该窗口：" + pointer);
                return true;
            }
            int x = caps[2].toInt();
            int y = caps[3].toInt();
            int w = caps[4].toInt();
            int h = caps[5].toInt();
            MoveWindow(hWnd, x, y, w, h, true);
#endif
            return true;
        }
    }

    // 邮件服务
    if (msg.contains("sendEmail"))
    {
        re = RE("sendEmail\\s*\\(\\s*(\\S+)\\s*,\\s*(.*?)\\s*,\\s*(.*)\\s*\\)");
        if (msg.indexOf(re, 0, &match) > -1)
        {
            QStringList caps = match.capturedTexts();
            qInfo() << "执行命令：" << caps;
            QString to = caps.at(1);
            QString subject = caps.at(2);
            QString body = caps.at(3);
            sendEmail(to, subject, body);
            return true;
        }
    }


    return false;
}
