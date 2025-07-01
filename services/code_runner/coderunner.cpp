#include "coderunner.h"
#include "conditionutil.h"
#include "calculator/calculator_util.h"
#include "ui_mainwindow.h"
#ifdef Q_OS_WIN
#include "widgets/windowshwnd.h"
#endif
#include "string_distance_util.h"
#include "eventwidget.h"
#include "replywidget.h"
#include "fileutil.h"
#include "ImageSimilarityUtil.h"
#include "netutil.h"
#include "signaltransfer.h"

CodeRunner::CodeRunner(QObject *parent) : QObject(parent)
{
    // 全局替换
    connect(st, &SignalTransfer::replaceDanmakuVariables, this, [=](QString* text, const LiveDanmaku& danmaku) {
        *text = processDanmakuVariants(*text, danmaku);
    });
    
    // 等待通道
    for (int i = 0; i < CHANNEL_COUNT; i++)
        msgWaits[i] = WAIT_CHANNEL_MAX;

    // 发送队列
    autoMsgTimer = new QTimer(this) ;
    autoMsgTimer->setInterval(1500); // 1.5秒发一次弹幕
    connect(autoMsgTimer, &QTimer::timeout, this, [=]{
        slotSendAutoMsg(true);
    });

    // 编程引擎
    jsEngine = new JSEngine(this);
    jsEngine->setHeaps(heaps);
    connect(jsEngine, &JSEngine::signalError, this, [=](const QString& err){
        emit signalShowError("JS引擎", err);
    });
    connect(jsEngine, &JSEngine::signalLog, this, [=](const QString& log){
        localNotify(log);
    });

    luaEngine = new LuaEngine(this);
    luaEngine->setHeaps(heaps);
    connect(luaEngine, &LuaEngine::signalError, this, [=](const QString& err){
        emit signalShowError("Lua引擎", err);
    });
    connect(luaEngine, &LuaEngine::signalLog, this, [=](const QString& log){
        localNotify(log);
    });

    pythonEngine = new PythonEngine(this);
    pythonEngine->setHeaps(heaps);
    connect(pythonEngine, &PythonEngine::signalError, this, [=](const QString& err){
        emit signalShowError("Python引擎", err);
    });
    connect(pythonEngine, &PythonEngine::signalLog, this, [=](const QString& log){
        localNotify(log);
    });
}

void CodeRunner::setLiveService(LiveRoomService *service)
{
    this->liveService = service;
}

void CodeRunner::setHeaps(MySettings *heaps)
{
    this->heaps = heaps;
    jsEngine->setHeaps(heaps);
    luaEngine->setHeaps(heaps);
    pythonEngine->setHeaps(heaps);
}

void CodeRunner::setMainUI(Ui::MainWindow *ui)
{
    this->ui = ui;
}

void CodeRunner::setMusicWindow(OrderPlayerWindow *musicWindow)
{
    this->musicWindow = musicWindow;
}

void CodeRunner::setWebServer(WebServer *ws)
{
    this->webServer = ws;
}

void CodeRunner::setVoiceService(VoiceService *vs)
{
    this->voiceService = vs;
}

void CodeRunner::releaseData()
{
    autoMsgQueues.clear();
    for (int i = 0; i < CHANNEL_COUNT; i++)
        msgCds[i] = 0;
    for (int i = 0; i < CHANNEL_COUNT; i++)
        msgWaits[i] = WAIT_CHANNEL_MAX;
}

/**
 * 从文件名获取到对于的文件
 * @param name URL、绝对路径、相对路径、文件名简写等等
 * @return 文件内容
 */
QString CodeRunner::getCodeContentFromFile(QString name) const
{
    /// 如果是 URL，网络文件
    if (name.startsWith("http://") || name.startsWith("https://"))
    {
        static QMap<QString, QString> cache; // 网络文件的代码的缓存
        if (cache.contains(name))
            return cache[name];
        QByteArray data = NetUtil::getWebFile(name);
        QString content = QString::fromUtf8(data);
        cache[name] = content;
        return content;
    }
    
    /// 如果是文件，那么不进行缓存，因为随时有可能进行修改
    // 如果是绝对路径或相对路径
    if (QFile::exists(name))
        return readTextFileAutoCodec(name);

    /// 如果是文件简写，可以是带后缀，或者不带后缀的
    // 获取 dataPath/codes/ 下的所有文件名（先是带后缀的）
    QDir dir(rt->dataPath + "codes/");
    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    foreach (QString file, files)
    {
        if (file == name)
        {
            qDebug() << "读取默认目录下代码文件：" << name;
            return readTextFileAutoCodec(dir.absoluteFilePath(file));
        }
    }
    // 再是不带后缀的
    foreach (QString file, files)
    {
        if (file.left(file.lastIndexOf(".")) == name)
        {
            qDebug() << "读取默认目录下简写代码文件：" << name;
            return readTextFileAutoCodec(dir.absoluteFilePath(file));
        }
    }
    return readTextFileAutoCodec(name);
}

/**
 * 发送带有变量的弹幕或者执行代码
 * @param msg       多行，带变量
 * @param danmaku   参数
 * @param channel   通道
 * @param manual    是否手动
 * @param delayMine 自己通过弹幕触发的，执行需要晚一点
 * @return          是否有发送弹幕或执行代码
 */
bool CodeRunner::sendVariantMsg(QString msg, const LiveDanmaku &danmaku, int channel, bool manual, bool delayMine)
{
    // 多段代码判断，在所有情况之前
    if (msg.contains(QRegularExpression("^\\s*-{3,}\\s*$", QRegularExpression::MultilineOption)))
    {
        QStringList codes = msg.split(QRegularExpression("^\\s*-{3,}\\s*$", QRegularExpression::MultilineOption), QString::SkipEmptyParts);
        bool hasSuccess = false;
        foreach (QString code, codes)
        {
            if (sendVariantMsg(code, danmaku, channel, manual, delayMine))
                hasSuccess = true;
        }
        return hasSuccess;
    }
    
    // 编程语言或代码文件
    bool ok;
    msg = replaceCodeLanguage(msg, danmaku, &ok);
    if (ok)
    {
        return sendVariantMsg(msg, danmaku, channel, manual, delayMine);
    }
    
    // 解析变量、条件
    QStringList msgs = getEditConditionStringList(msg, danmaku);
    if (!msgs.size())
        return false;

    // 随机选择一条消息
    int r = qrand() % msgs.size();
    QString s = msgs.at(r);
    if (!s.trimmed().isEmpty())
    {
        if (delayMine && danmaku.getUid() == ac->cookieUid) // 自己发的，自己回复，必须要延迟一会儿
        {
            if (s.contains(QRegExp("cd\\d+\\s*:\\s*\\d+"))) // 带冷却通道，不能放前面
                autoMsgTimer->start(); // 先启动，避免立即发送
            else
                s = "\\n" + s; // 延迟一次发送的时间
        }
        sendCdMsg(s, danmaku, 0, channel, true, false, manual);
    }
    return true;
}

/**
 * 发送多条消息
 * 不允许包含变量
 * 使用“\n”进行多行换行
 */
void CodeRunner::sendAutoMsg(QString msgs, const LiveDanmaku &danmaku)
{
    if (msgs.trimmed().isEmpty())
    {
        if (us->debugPrint)
            localNotify("[空弹幕，已忽略]");
        return ;
    }

    // 发送前替换
    for (auto it = us->replaceVariant.begin(); it != us->replaceVariant.end(); ++it)
    {
        msgs.replace(QRegularExpression(it->first), it->second);
    }

    // 分割与发送
    QStringList sl = msgs.split("\\n", QString::SkipEmptyParts);
    autoMsgQueues.append(qMakePair(sl, danmaku));
    if (/*!autoMsgTimer->isActive() &&*/ !inDanmakuDelay && !inDanmakuCd)
    {
        slotSendAutoMsg(false); // 先运行一次
    }
    if (!autoMsgTimer->isActive())
        autoMsgTimer->start();
}

void CodeRunner::sendAutoMsgInFirst(QString msgs, const LiveDanmaku &danmaku, int interval)
{
    if (msgs.trimmed().isEmpty())
    {
        if (us->debugPrint)
            localNotify("[空弹幕，已忽略]");
        return ;
    }
    QStringList sl = msgs.split("\\n", QString::SkipEmptyParts);
    autoMsgQueues.insert(0, qMakePair(sl, danmaku));
    if (interval > 0)
    {
        autoMsgTimer->setInterval(interval);
    }
    autoMsgTimer->stop();
    autoMsgTimer->start();
}

/**
 * 执行发送队列中的发送弹幕，或者命令操作
 * 不允许包括变量
 * // @return 是否是执行命令。为空或发送弹幕为false
 */
void CodeRunner::slotSendAutoMsg(bool timeout)
{
    if (timeout) // 全部发完之后 timer 一定还开着的，最后一次 timeout 清除弹幕发送冷却
        inDanmakuCd = false;

    if (autoMsgQueues.isEmpty())
    {
        autoMsgTimer->stop();
        return ;
    }

    if (autoMsgTimer->interval() != AUTO_MSG_CD) // 之前命令修改过延时
        autoMsgTimer->setInterval(AUTO_MSG_CD);

    if (!autoMsgQueues.first().first.size()) // 有一条空消息，应该是哪里添加错了的
    {
        qWarning() << "错误的发送消息：空消息，已跳过";
        autoMsgQueues.removeFirst();
        slotSendAutoMsg(false);
        return ;
    }

    QStringList* sl = &autoMsgQueues[0].first;
    LiveDanmaku* danmaku = &autoMsgQueues[0].second;
    QString msg = sl->takeFirst();

    if (sl->isEmpty()) // 消息发送完了
    {
        danmaku = new LiveDanmaku(autoMsgQueues[0].second);
        sl = new QStringList;
        autoMsgQueues.removeFirst(); // 此时 danmaku 对象也有已经删掉了
        QTimer::singleShot(0, [=]{
            delete sl;
            delete danmaku;
        });
    }
    if (!autoMsgQueues.size())
        autoMsgTimer->stop();

    // 判断是否有子账号信息，并提取相关Cookie
    QString subAccountArg = danmaku->getSubAccount();
    QString accountCookie = us->getSubAccountCookie(subAccountArg);

    // 弹幕纯文本，或者命令
    CmdResponse res = NullRes;
    int resVal = 0;
    bool exec = execFuncCallback(msg, *danmaku, res, resVal);
    if (!exec) // 如果是发送弹幕
    {
        msg = msgToShort(msg);
        addNoReplyDanmakuText(msg);
        if (danmaku->isRetry())
            liveService->sendRoomMsg(danmaku->getRoomId(), msg, accountCookie);
        else
            liveService->sendMsg(msg, accountCookie);
        inDanmakuCd = true;
        us->setValue("danmaku/robotTotalSend", ++robotTotalSendMsg);
        ui->robotSendCountLabel->setText(snum(robotTotalSendMsg));
        ui->robotSendCountLabel->setToolTip("累计发送弹幕 " + snum(robotTotalSendMsg) + " 条");
    }
    else // 是执行命令，发送下一条弹幕就不需要延迟了
    {
        if (res == AbortRes) // 终止这一轮后面的弹幕
        {
            if (!sl->isEmpty()) // 如果为空，则自动为终止
                autoMsgQueues.removeFirst();
            return ;
        }
        else if (res == DelayRes) // 修改延迟
        {
            if (resVal < 0)
                qCritical() << "设置延时时间出错";
            autoMsgTimer->setInterval(resVal);
            if (!autoMsgTimer->isActive())
                autoMsgTimer->start();
            inDanmakuDelay++;
            QTimer::singleShot(resVal, [=]{
                inDanmakuDelay--;
                if (inDanmakuDelay < 0)
                    inDanmakuDelay = 0;
            });
            return ;
        }
    }

    // 如果后面是命令的话，尝试立刻执行
    // 可以取消间隔，大大加快速度
    // 因为大部分复杂的代码，也就是一条弹幕+一堆命令
    if (!inDanmakuDelay && autoMsgQueues.size())
    {
        const QString& nextMsg = autoMsgQueues.first().first.first();
        QRegularExpression re("^\\s*(>.+\\)|\\{.+\\}.*[=\\+\\-].*)\\s*$");
        if (nextMsg.indexOf(re) > -1) // 下一条是命令，直接执行
        {
            slotSendAutoMsg(false); // 递归
        }
    }
}

/**
 * 发送前确保没有需要调整的变量了
 * 若有变量，请改用：sendVariantMsg
 * 而且已经不需要在意条件了
 * 一般解析后的弹幕列表都随机执行一行，以cd=0来发送cd弹幕
 * 带有cd channel，但实际上不一定有cd
 * @param msg 单行要发送的弹幕序列
 */
void CodeRunner::sendCdMsg(QString msg, LiveDanmaku danmaku, int cd, int channel, bool enableText, bool enableVoice, bool manual)
{
    if (!manual && !shallAutoMsg()) // 不在直播中
    {
        qInfo() << "未开播，不做操作(cd)" << msg;
        if (us->debugPrint)
            localNotify("[未开播，不做操作]");
        return ;
    }
    if (msg.trimmed().isEmpty())
    {
        if (us->debugPrint)
            localNotify("[空弹幕，已跳过]");
        return ;
    }

    bool forceAdmin = false;
    int waitCount = 0;
    int waitChannel = 0;

    // 弹幕选项
    QRegularExpression re("^\\s*\\(([\\w\\d:, ]+)\\)");
    QRegularExpressionMatch match;
    if (msg.indexOf(re, 0, &match) > -1)
    {
        QStringList caps = match.capturedTexts();
        QString full = caps.at(0);
        QString optionStr = caps.at(1);
        QStringList options = optionStr.split(",", QString::SkipEmptyParts);

        // 遍历每一个选项
        foreach (auto option, options)
        {
            option = option.trimmed();

            // 冷却通道
            re.setPattern("^cd(\\d{1,2})\\s*:\\s*(\\d+)$");
            if (option.indexOf(re, 0, &match) > -1)
            {
                caps = match.capturedTexts();
                QString chann = caps.at(1);
                QString val = caps.at(2);
                channel = chann.toInt();
                cd = val.toInt() * 1000; // 秒转毫秒
                continue;
            }

            // 等待通道
            re.setPattern("^wait(\\d{1,2})\\s*:\\s*(\\d+)$");
            if (option.indexOf(re, 0, &match) > -1)
            {
                caps = match.capturedTexts();
                QString chann = caps.at(1);
                QString val = caps.at(2);
                waitChannel = chann.toInt();
                waitCount = val.toInt();
                continue;
            }

            // 强制房管权限
            re.setPattern("^admin(\\s*[=:]\\s*(true|1))$");
            if (option.contains(re))
            {
                forceAdmin = true;
                continue;
            }

            // 发送Cookie
            re.setPattern("^(?:ac|account)\\s*[=:]\\s*(\\S+)$");
            if (option.indexOf(re, 0, &match) > -1)
            {
                caps = match.capturedTexts();
                QString sub = caps.at(1);
                danmaku.setSubAccount(sub);
            }
        }

        // 去掉选项，发送剩余弹幕
        msg = msg.right(msg.length() - full.length());
    }

    // 冷却通道：避免太频繁发消息
    /* analyzeMsgAndCd(msg, cd, channel); */
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch(); // 值是毫秒，用户配置为秒
    if (cd > 0 && timestamp - msgCds[channel] < cd)
    {
        if (us->debugPrint)
            localNotify("[未完成冷却：" + snum(timestamp - msgCds[channel]) + " 不足 " + snum(cd) + "毫秒]");
        return ;
    }
    msgCds[channel] = timestamp; // 重新冷却

    // 等待通道
    if (waitCount > 0 && msgWaits[waitChannel] < waitCount)
    {
        if (us->debugPrint)
            localNotify("[未完成等待：" + snum(msgWaits[waitChannel]) + " 不足 " + snum(waitCount) + "条]");
        return ;
    }
    msgWaits[waitChannel] = 0; // 重新等待

    // 强制房管权限
    if (forceAdmin)
        danmaku.setAdmin(true);

    if (enableText)
    {
        sendAutoMsg(msg, danmaku);
    }
    if (enableVoice)
        speekVariantText(msg);
}

void CodeRunner::sendCdMsg(QString msg, const LiveDanmaku &danmaku)
{
    sendCdMsg(msg, danmaku, 0, NOTIFY_CD_CN, true, false, false);
}

void CodeRunner::sendGiftMsg(QString msg, const LiveDanmaku &danmaku)
{
    cr->sendCdMsg(msg, danmaku, ui->sendGiftCDSpin->value() * 1000, GIFT_CD_CN,
              ui->sendGiftTextCheck->isChecked(), ui->sendGiftVoiceCheck->isChecked(), false);
}

void CodeRunner::sendAttentionMsg(QString msg, const LiveDanmaku &danmaku)
{
    cr->sendCdMsg(msg, danmaku, ui->sendAttentionCDSpin->value() * 1000, GIFT_CD_CN,
              ui->sendAttentionTextCheck->isChecked(), ui->sendAttentionVoiceCheck->isChecked(), false);
}

void CodeRunner::sendNotifyMsg(QString msg, bool manual)
{
    cr->sendCdMsg(msg, LiveDanmaku(), NOTIFY_CD, NOTIFY_CD_CN,
              true, false, manual);
}

void CodeRunner::sendNotifyMsg(QString msg, const LiveDanmaku &danmaku, bool manual)
{
    cr->sendCdMsg(msg, danmaku, NOTIFY_CD, NOTIFY_CD_CN,
              true, false, manual);
}

/// 添加或者删除过滤器
/// 也不一定是过滤器，任何内容都有可能
void CodeRunner::setFilter(QString filterName, QString content)
{
    QString filterKey;
    if (filterName == FILTER_MUSIC_ORDER)
    {
        filterKey = "filter/musicOrder";
        filter_musicOrder = content;
        filter_musicOrderRe = QRegularExpression(filter_musicOrder);
    }
    else if (filterName == FILTER_DANMAKU_MSG)
    {
        filterKey = "filter/danmakuMsg";
        filter_danmakuMsg = content;
    }
    else if (filterName == FILTER_DANMAKU_COME)
    {
        filterKey = "filter/danmakuCome";
        filter_danmakuCome = content;
    }
    else if (filterName == FILTER_DANMAKU_GIFT)
    {
        filterKey = "filter/danmakuGift";
        filter_danmakuGift = content;
    }
    else // 不是系统过滤器
        return ;
    us->setValue(filterKey, content);
    qInfo() << "设置过滤器：" << filterKey << content;
}

void CodeRunner::addNoReplyDanmakuText(const QString &text)
{
    noReplyMsgs.append(text);
}

void CodeRunner::analyzeMsgAndCd(QString& msg, int &cd, int &channel) const
{
    QRegularExpression re("^\\s*\\(cd(\\d{1,2}):(\\d+)\\)");
    QRegularExpressionMatch match;
    if (msg.indexOf(re, 0, &match) == -1)
        return ;
    QStringList caps = match.capturedTexts();
    QString full = caps.at(0);
    QString chann = caps.at(1);
    QString val = caps.at(2);
    msg = msg.right(msg.length() - full.length());
    cd = val.toInt() * 1000;
    channel = chann.toInt();
}

/**
 * 支持变量名
 */
QString CodeRunner::processTimeVariants(QString msg) const
{
    // 早上/下午/晚上 - 好呀
    int hour = QTime::currentTime().hour();
    if (msg.contains("%hour%"))
    {
        QString rst;
        if (hour <= 3)
            rst = "晚上";
        else if (hour <= 5)
            rst = "凌晨";
        else if (hour < 11)
            rst = "早上";
        else if (hour <= 13)
            rst = "中午";
        else if (hour <= 17)
            rst = "下午";
        else if (hour <= 24)
            rst = "晚上";
        else
            rst = "今天";
        msg = msg.replace("%hour%", rst);
    }

    // 早上好、早、晚上好-
    if (msg.contains("%greet%"))
    {
        QStringList rsts;
        rsts << "你好";
        if (hour <= 3)
            rsts << "晚上好";
        else if (hour <= 5)
            rsts << "凌晨好";
        else if (hour < 11)
            rsts << "早上好" << "早" << "早安";
        else if (hour <= 13)
            rsts << "中午好";
        else if (hour <= 16)
            rsts << "下午好";
        else if (hour <= 24)
            rsts << "晚上好";
        int r = qrand() % rsts.size();
        msg = msg.replace("%greet%", rsts.at(r));
    }

    // 全部打招呼
    if (msg.contains("%all_greet%"))
    {
        QStringList sl{"啊", "呀"};
        int r = qrand() % sl.size();
        QString tone = sl.at(r);
        QStringList rsts;
        rsts << "您好"+tone
             << "你好"+tone;
        if (hour <= 3)
            rsts << "晚上好"+tone;
        else if (hour <= 5)
            rsts << "凌晨好"+tone;
        else if (hour < 11)
            rsts << "早上好"+tone << "早"+tone;
        else if (hour <= 13)
            rsts << "中午好"+tone;
        else if (hour <= 16)
            rsts << "下午好"+tone;
        else if (hour <= 24)
            rsts << "晚上好"+tone;

        if (hour >= 6 && hour <= 8)
            rsts << "早饭吃了吗";
        else if (hour >= 11 && hour <= 12)
            rsts << "午饭吃了吗";
        else if (hour >= 17 && hour <= 18)
            rsts << "晚饭吃了吗";

        if ((hour >= 23 && hour <= 24)
                || (hour >= 0 && hour <= 3))
            rsts << "还没睡"+tone << "怎么还没睡";
        else if (hour >= 3 && hour <= 5)
            rsts << "通宵了吗";

        r = qrand() % rsts.size();
        msg = msg.replace("%all_greet%", rsts.at(r));
    }

    // 语气词
    if (msg.contains("%tone%"))
    {
        QStringList sl{"啊", "呀"};
        int r = qrand() % sl.size();
        msg = msg.replace("%tone%", sl.at(r));
    }
    if (msg.contains("%lela%"))
    {
        QStringList sl{"了", "啦"};
        int r = qrand() % sl.size();
        msg = msg.replace("%lela%", sl.at(r));
    }

    // 标点
    if (msg.contains("%punc%"))
    {
        QStringList sl{"~", "！"};
        int r = qrand() % sl.size();
        msg = msg.replace("%punc%", sl.at(r));
    }

    // 语气词或标点
    if (msg.contains("%tone/punc%"))
    {
        QStringList sl{"啊", "呀", "~", "！"};
        int r = qrand() % sl.size();
        msg = msg.replace("%tone/punc%", sl.at(r));
    }

    return msg;
}

QString CodeRunner::replaceCodeLanguage(QString code, const LiveDanmaku& danmaku, bool *ok)
{
    QString msg = code;
    *ok = true;

    // 文件判断
    if (msg.contains("<file:"))
    {
        QRegularExpression re("\\s*<\\s*file\\s*:\\s*(.*)\\s*>\\s*");
        QRegularExpressionMatch match;
        if (msg.contains(re, &match))
        {
            QString placeholder = match.captured(0);
            QString name = match.captured(1); // 文件名/文件路径
            qDebug() << "加载代码文件：" << name;
            QString content = getCodeContentFromFile(name);
            msg = msg.replace(placeholder, content);
            return msg;
        }
    }

    // 判断是否是自定义编程语言
    QRegularExpressionMatch match;
    if (msg.contains(QRegularExpression("^\\s*var:\\s*", QRegularExpression::CaseInsensitiveOption), &match)) // 替换变量
    {
        QString code = msg.mid(match.capturedLength());
        code = processDanmakuVariants(code, danmaku);
        return code;
    }
    else if (msg.contains(QRegularExpression("^\\s*(js|javascript):\\s*", QRegularExpression::CaseInsensitiveOption), &match))
    {
        QString code = msg.mid(match.capturedLength());
        QString result = jsEngine->runCode(danmaku, code);
        return result;
    }
    else if (msg.contains(QRegularExpression("^\\s*lua:\\s*", QRegularExpression::CaseInsensitiveOption), &match))
    {
        QString code = msg.mid(match.capturedLength());
        QString result = luaEngine->runCode(danmaku, code);
        return result;
    }
    else if (msg.contains(QRegularExpression("^\\s*(python|python3|py|py3):\\s*", QRegularExpression::CaseInsensitiveOption), &match))
    {
        QString exeName = match.captured(1);
        if (exeName == "py") exeName = "python";
        else if (exeName == "py3") exeName = "python3";
        QString code = msg.mid(match.capturedLength());
        QString result = pythonEngine->runCode(danmaku, exeName, code);
        return result;
    }
    else if (msg.contains(QRegularExpression("^\\s*exec:(.+?):\\s*", QRegularExpression::CaseInsensitiveOption), &match))
    {
        QString exePath = match.captured(1);
        exePath.replace("<codes>", rt->dataPath + "codes");
        QString code = msg.mid(match.capturedLength());
        QString result = pythonEngine->runCode(danmaku, exePath, code);
        return result;
    }

    *ok = false;
    return msg;
}

/**
 * 获取可以发送的代码的列表
 * @param plainText 为替换变量的纯文本，允许使用\\n分隔的单行
 * @param danmaku   弹幕变量
 * @return          一行一条执行序列，带发送选项
 */
QStringList CodeRunner::getEditConditionStringList(QString plainText, LiveDanmaku danmaku)
{
    // 替换变量，整理为一行一条执行序列
    plainText = processDanmakuVariants(plainText, danmaku);
    CALC_DEB << "处理变量之后：" << plainText;
    lastConditionDanmu.append(plainText);
    if (lastConditionDanmu.size() > debugLastCount)
        lastConditionDanmu.removeFirst();

    // 寻找条件为true的
    QStringList lines = plainText.split("\n", QString::SkipEmptyParts);
    QStringList result;
    for (int i = 0; i < lines.size(); i++)
    {
        QString line = lines.at(i);
        line = processMsgHeaderConditions(line);
        CALC_DEB << "取条件后：" << line << "    原始：" << lines.at(i);
        if (!line.isEmpty())
            result.append(line.trimmed());
    }

    // 判断超过长度的
    if (us->removeLongerRandomDanmaku)
    {
        for (int i = 0; i < result.size() && result.size() > 1; i++)
        {
            QString s = result.at(i);
            if (s.contains(">") || s.contains("\\n")) // 包含多行或者命令的，不需要或者懒得判断长度
                continue;
            s = s.replace(QRegExp("^\\s+\\(\\s*[\\w\\d: ,]*\\s*\\)"), "").replace("*", "").trimmed(); // 去掉发送选项
            // s = s.replace(QRegExp("^\\s+\\(\\s*cd\\d+\\s*:\\s*\\d+\\s*\\)"), "").replace("*", "").trimmed();
            if (s.length() > ac->danmuLongest && !s.contains("%"))
            {
                if (us->debugPrint)
                    localNotify("[去掉过长候选：" + s + "]");
                result.removeAt(i--);
            }
        }
    }

    // 查看是否优先级
    auto hasPriority = [=](const QStringList& sl){
        for (int i = 0; i < sl.size(); i++)
            if (sl.at(i).startsWith("*"))
                return true;
        return false;
    };

    // 去除优先级
    while (hasPriority(result))
    {
        for (int i = 0; i < result.size(); i++)
        {
            if (!result.at(i).startsWith("*"))
                result.removeAt(i--);
            else
                result[i] = result.at(i).right(result.at(i).length()-1);
        }
    }
    CALC_DEB << "condition result:" << result;
    lastCandidateDanmaku.append(result.join("\n"));
    if (lastCandidateDanmaku.size() > debugLastCount)
        lastCandidateDanmaku.removeFirst();

    return result;
}

/**
 * 处理用户信息中蕴含的表达式
 * 用户信息、弹幕、礼物等等
 */
QString CodeRunner::processDanmakuVariants(QString msg, const LiveDanmaku& danmaku)
{
    QRegularExpressionMatch match;
    QRegularExpression re;

    // 去掉注释
    re = QRegularExpression("(?<!:)//.*?(?=\\n|$|\\\\n)");
    msg.replace(re, "");

    // 软换行符
    re = QRegularExpression("\\s*\\\\\\s*\\n\\s*");
    msg.replace(re, "");

    CALC_DEB << "有效代码：" << msg;

    // 自动回复传入的变量
    re = QRegularExpression("%\\$(\\d+)%");
    while (msg.indexOf(re, 0, &match) > -1)
    {
        QString _var = match.captured(0);
        int i = match.captured(1).toInt();
        QString text = danmaku.getArgs(i);
        msg.replace(_var, text);
    }

    // 生成代码
    re = QRegularExpression("^\\s*#\\s*(\\w+)\\s*\\((.+)\\)\\s*$", QRegularExpression::MultilineOption);
    int matchPos = 0;
    bool ok;
    while ((matchPos = msg.indexOf(re, matchPos, &match)) > -1)
    {
        QString mat = match.captured(0);
        QString funcName = match.captured(1);
        QString args = match.captured(2);
        QString rpls = generateCodeFunctions(funcName, args, danmaku, &ok);
        if (ok)
        {
            msg.replace(mat, rpls);
            matchPos = matchPos + rpls.length();
        }
        else
        {
            if (mat.length() > 1 && mat.endsWith("%"))
                matchPos += mat.length() - 1;
            else
                matchPos += mat.length();
        }
    }

    // 自定义变量
    for (auto it = us->customVariant.begin(); it != us->customVariant.end(); ++it)
    {
        msg.replace(QRegularExpression(it->first), it->second);
    }

    // 翻译
    for (auto it = us->variantTranslation.begin(); it != us->variantTranslation.end(); ++it)
    {
        msg.replace(it->first, it->second);
    }

    // 招呼变量（固定文字，随机内容）
    msg = processTimeVariants(msg);

    // 弹幕变量、环境变量（固定文字）
    re = QRegularExpression("%[\\w_]+%");
    matchPos = 0;
    while ((matchPos = msg.indexOf(re, matchPos, &match)) > -1)
    {
        QString mat = match.captured(0);
        QString rpls = replaceDanmakuVariants(danmaku, mat, &ok);
        if (ok)
        {
            msg.replace(mat, rpls);
            matchPos = matchPos + rpls.length();
        }
        else
        {
            if (mat.length() > 1 && mat.endsWith("%"))
                matchPos += mat.length() - 1;
            else
                matchPos += mat.length();
        }
    }

    // 根据昵称替换为uid：倒找最近的弹幕、送礼
    re = QRegularExpression("%\\(([^(%)]+?)\\)%");
    while (msg.indexOf(re, 0, &match) > -1)
    {
        QString _var = match.captured(0);
        QString text = match.captured(1);
        msg.replace(_var, unameToUid(text));
    }

    bool find = true;
    while (find)
    {
        find = false;

        // 替换JSON数据
        // 尽量支持 %.data.%{key}%.list% 这样的格式吧
        re = QRegularExpression("%\\.([^%]*?[^%\\.])%");
        matchPos = 0;
        QJsonObject json = danmaku.extraJson;
        while ((matchPos = msg.indexOf(re, matchPos, &match)) > -1)
        {
            bool ok = false;
            QString rpls = replaceDanmakuJson(match.captured(1), json, &ok);
            if (!ok)
            {
                matchPos++;
                continue;
            }
            msg.replace(match.captured(0), rpls);
            matchPos += rpls.length();
            find = true;
        }

        // 读取配置文件的变量
        //re = QRegularExpression("%\\{([^%{[(]*?)\\}%");
        re = QRegularExpression("%\\{((.(?!%[\\[\\(\\{]))*?)\\}%");
        while (msg.indexOf(re, 0, &match) > -1)
        {
            QString _var = match.captured(0);
            QString key = match.captured(1);
            QString def = "";
            if (key.contains("|"))
            {
                int pos = key.indexOf("|");
                def = key.right(key.length() - pos - 1);
                key = key.left(pos);
            }
            if (!key.contains("/"))
                key = "heaps/" + key;
            QVariant var = heaps->value(key, def);
            msg.replace(_var, var.toString()); // 默认使用变量类型吧
            find = true;
        }

        // 进行数学计算的变量
        if (!us->complexCalc)
            re = QRegularExpression("%\\[([\\d\\+\\-\\*/% \\(\\)]*?)\\]%"); // 纯数字+运算符+括号
        else
            re = QRegularExpression("%\\[([^(%(\\{|\\[|>))]*?)\\]%"); // 允许里面带点字母，用来扩展函数
        while (msg.indexOf(re, 0, &match) > -1)
        {
            QString _var = match.captured(0);
            QString text = match.captured(1);
            if (!us->complexCalc)
                text = QString::number(ConditionUtil::calcIntExpression(text));
            else
                text = QString::number(CalculatorUtil::calculate(text.toStdString().c_str()));
            msg.replace(_var, text); // 默认使用变量类型吧
            find = true;
        }

        // 函数替换
        re = QRegularExpression("%>(\\w+)\\s*\\(([^(%(\\{|\\[|>))]*?)\\)%");
        matchPos = 0;
        while ((matchPos = msg.indexOf(re, matchPos, &match)) > -1)
        {
            QString rpls = replaceDynamicVariants(match.captured(1), match.captured(2), danmaku);
            msg.replace(match.captured(0), rpls);
            matchPos += rpls.length();
            find = true;
        }
    }

    // 无奈的替换
    // 一些代码特殊字符的也给替换掉
    find = true;
    while (find)
    {
        find = false;

        // 函数替换
        // 允许里面的参数出现%
        re = QRegularExpression("%>(\\w+)\\s*\\((.*?)\\)%");
        matchPos = 0;
        while ((matchPos = msg.indexOf(re, matchPos, &match)) > -1)
        {
            QString rpls = replaceDynamicVariants(match.captured(1), match.captured(2), danmaku);
            msg.replace(match.captured(0), rpls);
            matchPos += rpls.length();
            find = true;
        }
    }

    // 一些用于替换特殊字符的东西，例如想办法避免无法显示 100% 这种

    return msg;
}

QString CodeRunner::replaceDanmakuVariants(const LiveDanmaku& danmaku, const QString &key, bool *ok)
{
    *ok = true;
    QRegularExpressionMatch match;
    // 用户昵称
    if (key == "%uname%" || key == "%username%" || key =="%nickname%" || key == "%file_name%")
        return danmaku.getNickname();

    // 用户昵称
    else if (key == "%uid%")
        return danmaku.getUid();

    // 本地昵称+简化
    else if (key == "%ai_name%")
    {
        QString name = us->getLocalNickname(danmaku.getUid());
        if (name.isEmpty())
            name = nicknameSimplify(danmaku);
        if (name.isEmpty())
            name = danmaku.getNickname();
        return name;
    }

    // 专属昵称
    else if (key == "%local_name%")
    {
        QString local = us->getLocalNickname(danmaku.getUid());
        if (local.isEmpty())
            local = danmaku.getNickname();
        return local;
    }

    // 昵称简化
    else if (key == "%simple_name%")
    {
        return nicknameSimplify(danmaku);
    }

    // 用户等级
    else if (key == "%level%")
        return snum(danmaku.getLevel());

    else if (key == "%text%" || key == "%file_path%")
        return toSingleLine(danmaku.getText());

    else if (key == "%url_text%")
        return QString::fromUtf8(danmaku.getText().toUtf8().toPercentEncoding());

    // 进来次数
    else if (key == "%come_count%")
    {
        if (danmaku.is(MSG_WELCOME) || danmaku.is(MSG_WELCOME_GUARD))
            return snum(danmaku.getNumber());
        else
            return snum(us->danmakuCounts->value("come/" + danmaku.getUid()).toInt());
    }

    // 上次进来
    else if (key == "%come_time%")
    {
        return snum(danmaku.is(MSG_WELCOME) || danmaku.is(MSG_WELCOME_GUARD)
                                        ? danmaku.getPrevTimestamp()
                                        : us->danmakuCounts->value("comeTime/" + danmaku.getUid()).toLongLong());
    }

    // 和现在的时间差
    else if (key == "%come_time_delta%")
    {
        qint64 prevTime = danmaku.is(MSG_WELCOME) || danmaku.is(MSG_WELCOME_GUARD)
                ? danmaku.getPrevTimestamp()
                : us->danmakuCounts->value("comeTime/" + danmaku.getUid()).toLongLong();
        return snum(QDateTime::currentSecsSinceEpoch() - prevTime);
    }

    else if (key == "%prev_time%")
    {
        return snum(danmaku.getPrevTimestamp());
    }

    else if (key == "%wealth_level%")
    {
        return snum(danmaku.getWealthLevel());
    }

    // 本次送礼金瓜子
    else if (key == "%gift_gold%")
        return snum(danmaku.isGoldCoin() ? danmaku.getTotalCoin() : 0);

    // 本次送礼银瓜子
    else if (key == "%gift_silver%")
        return snum(danmaku.isGoldCoin() ? 0 : danmaku.getTotalCoin());

    // 本次送礼金瓜子+银瓜子（应该只有一个，但直接相加了）
    else if (key == "%gift_coin%")
        return snum(danmaku.getTotalCoin());

    // 是否是金瓜子礼物
    else if (key == "%coin_gold%")
        return danmaku.isGoldCoin() ? "1" : "0";

    // 盲盒实际价值
    else if (key == "%discount_price%" || key == "%real_price%" || key == "%real_coin%")
        return snum(danmaku.getDiscountPrice());

    // 本次送礼名字
    else if (key == "%gift_name%")
        return us->giftAlias.contains(danmaku.getGiftId()) ? us->giftAlias.value(danmaku.getGiftId()) : danmaku.getGiftName();

    // 原始礼物名字
    else if (key == "%origin_gift_name%")
        return danmaku.getGiftName();

    // 礼物ID
    else if (key == "%gift_id%")
        return snum(danmaku.getGiftId());

    // 本次送礼数量
    else if (key == "%gift_num%")
        return snum(danmaku.getNumber());

    else if (key == "%gift_multi_num%")
    {
        QString danwei = "个";
        QString giftName = danmaku.getGiftName();
        if (giftName.endsWith("船"))
            danwei = "艘";
        else if (giftName.endsWith("条"))
            danwei = "根";
        else if (giftName.endsWith("锦鲤"))
            danwei = "条";
        else if (giftName.endsWith("卡"))
            danwei = "张";
        else if (giftName.endsWith("灯"))
            danwei = "盏";
        else if (giftName.endsWith("阔落"))
            danwei = "瓶";
        else if (giftName.endsWith("花"))
            danwei = "朵";
        else if (giftName.endsWith("车"))
            danwei = "辆";
        else if (giftName.endsWith("情书"))
            danwei = "封";
        else if (giftName.endsWith("城"))
            danwei = "座";
        else if (giftName.endsWith("心心"))
            danwei = "颗";
        else if (giftName.endsWith("亿圆"))
            danwei = "枚";
        else if (giftName.endsWith("麦克风"))
            danwei = "支";
        return danmaku.getNumber() > 1 ? snum(danmaku.getNumber()) + danwei : "";
    }

    // 总共赠送金瓜子
    else if (key == "%total_gold%")
        return snum(us->danmakuCounts->value("gold/" + danmaku.getUid()).toLongLong());

    // 总共赠送银瓜子
    else if (key == "%total_silver%")
        return snum(us->danmakuCounts->value("silver/" + danmaku.getUid()).toLongLong());

    // 购买舰长
    else if (key == "%guard_buy%")
        return danmaku.is(MSG_GUARD_BUY) ? "1" : "0";

    else if (key == "%guard_buy_count%")
        return snum(us->danmakuCounts->value("guard/" + danmaku.getUid(), 0).toInt());

    // 0续费，1第一次上船，2重新上船
    else if (key == "%guard_first%" || key == "%first%")
        return snum(danmaku.getFirst());

    // 特别关注
    else if (key == "%special%")
        return snum(danmaku.getSpecial());

    else if (key == "%spread%")
        return danmaku.getSpreadDesc();

    // 粉丝牌房间
    else if (key == "%anchor_roomid%" || key == "%medal_roomid%" || key == "%anchor_room_id%" || key == "%medal_room_id%")
        return danmaku.getAnchorRoomid();

    // 粉丝牌名字
    else if (key == "%medal_name%")
        return danmaku.getMedalName();

    // 粉丝牌等级
    else if (key == "%medal_level%")
        return snum(danmaku.getMedalLevel());

    // 粉丝牌主播
    else if (key == "%medal_up%")
        return danmaku.getMedalUp();

    // 房管
    else if (key == "%admin%")
        return danmaku.isAdmin() ? "1" : (!ac->upUid.isEmpty() && danmaku.getUid()==ac->upUid ? "1" : "0");

    // 舰长
    else if (key == "%guard%" || key == "%guard_level%")
        return snum(danmaku.getGuard());

    // 舰长名称
    else if (key == "%guard_name%" || key == "%guard_type%")
    {
        int guard = danmaku.getGuard();
        QString name = "";
        if (guard == 1)
            name = "总督";
        else if (guard == 2)
            name = "提督";
        else if (guard == 3)
            name = "舰长";
        return name;
    }

    // 房管或舰长
    else if (key == "%admin_or_guard%")
        return (danmaku.isGuard() || danmaku.isAdmin() || (!ac->upUid.isEmpty() && danmaku.getUid() == ac->upUid)) ? "1" : "0";

    // 高能榜
    else if (key == "%online_rank%")
    {
        UIDT uid = danmaku.getUid();
        for (int i = 0; i < liveService->onlineGoldRank.size(); i++)
        {
            if (liveService->onlineGoldRank.at(i).getUid() == uid)
            {
                return snum(i + 1);
            }
        }
        return snum(0);
    }

    // 是否是姥爷
    else if (key == "%vip%")
        return danmaku.isVip() ? "1" : "0";

    // 是否是年费姥爷
    else if (key == "%svip%")
        return danmaku.isSvip() ? "1" : "0";

    // 是否是正式会员
    else if (key == "%uidentity%")
        return danmaku.isUidentity() ? "1" : "0";

    // 是否有手机验证
    else if (key == "%iphone%")
        return danmaku.isIphone() ? "1" : "0";

    // 数量
    else if (key == "%number%")
        return snum(danmaku.getNumber());

    // 昵称长度
    else if (key == "%nickname_len%")
        return snum(danmaku.getNickname().length());

    // 礼物名字长度
    else if (key == "%giftname_len%")
        return snum((us->giftAlias.contains(danmaku.getGiftId()) ? us->giftAlias.value(danmaku.getGiftId()) : danmaku.getGiftName()).length());

    // 昵称+礼物名字长度
    else if (key == "%name_sum_len%")
        return snum(danmaku.getNickname().length() + (us->giftAlias.contains(danmaku.getGiftId()) ? us->giftAlias.value(danmaku.getGiftId()) : danmaku.getGiftName()).length());

    else if (key == "%ainame_sum_len%")
    {
        QString local = us->getLocalNickname(danmaku.getUid());
        if (local.isEmpty())
            local = nicknameSimplify(danmaku);
        if (local.isEmpty())
            local = danmaku.getNickname();
        return snum(local.length() + (us->giftAlias.contains(danmaku.getGiftId()) ? us->giftAlias.value(danmaku.getGiftId()) : danmaku.getGiftName()).length());
    }

    // 弹幕相关
    else if (key == "%danmu_longest")
    {
        return snum(ac->danmuLongest);
    }
    else if (key == "%reply%")
    {
        return toSingleLine(danmaku.getAIReply());
    }
    else if (key == "%reply_len%")
    {
        return snum(danmaku.getAIReply().length());
    }
    else if (key == "%sub_account_index%")
    {
        return "";
    }

    // 是否新关注
    else if (key == "%new_attention%")
    {
        bool isInFans = false;
        UIDT uid = danmaku.getUid();
        foreach (FanBean fan, liveService->fansList)
            if (fan.mid == uid)
            {
                isInFans = true;
                break;
            }
        return isInFans ? "1" : "0";
    }

    // 是否是对面串门
    else if (key == "%pk_opposite%")
        return danmaku.isOpposite() ? "1" : "0";

    // 是否是己方串门回来
    else if (key == "%pk_view_return%")
        return danmaku.isViewReturn() ? "1" : "0";

    // 本次进来人次
    else if (key == "%today_come%")
        return snum(liveService->dailyCome);

    // 新人发言数量
    else if (key == "%today_newbie_msg%")
        return snum(liveService->dailyNewbieMsg);

    // 今天弹幕总数
    else if (key == "%today_danmaku%")
        return snum(liveService->dailyDanmaku);

    // 今天新增关注
    else if (key == "%today_fans%")
        return snum(liveService->dailyNewFans);

    // 大航海人数
    else if (key == "%guard_count%")
        return snum(liveService->guardInfos.size());

    // 当前粉丝数量
    else if (key == "%fans_count%")
        return snum(ac->currentFans);
    else if (key == "%fans_club%")
        return snum(ac->currentFansClub);

    // 今天金瓜子总数
    else if (key == "%today_gold%")
        return snum(liveService->dailyGiftGold);

    // 今天银瓜子总数
    else if (key == "%today_silver%")
        return snum(liveService->dailyGiftSilver);

    // 今天是否有新舰长
    else if (key == "%today_guard%")
        return snum(liveService->dailyGuard);

    // 今日最高人气
    else if (key == "%today_max_ppl%")
        return snum(liveService->dailyMaxPopul);

    // 当前人气
    else if (key == "%popularity%")
        return snum(ac->currentPopul);

    // 当前时间
    else if (key == "%time_hour%")
        return snum(QTime::currentTime().hour());
    else if (key == "%time_minute%")
        return snum(QTime::currentTime().minute());
    else if (key == "%time_second%")
        return snum(QTime::currentTime().second());
    else if (key == "%time_day%")
        return snum(QDate::currentDate().day());
    else if (key == "%time_month%")
        return snum(QDate::currentDate().month());
    else if (key == "%time_year%")
        return snum(QDate::currentDate().year());
    else if (key == "%time_day_week%")
        return snum(QDate::currentDate().dayOfWeek());
    else if (key == "%time_day_year%")
        return snum(QDate::currentDate().dayOfYear());
    else if (key == "%timestamp%")
        return snum(QDateTime::currentSecsSinceEpoch());
    else if (key == "%timestamp13%")
        return snum(QDateTime::currentMSecsSinceEpoch());

    // 大乱斗
    else if (key == "%pking%")
        return snum(liveService->pking ? 1 : 0);
    else if (key == "%pk_video%")
        return snum(liveService->pkVideo ? 1 : 0);
    else if (key == "%pk_room_id%")
        return liveService->pkRoomId;
    else if (key == "%pk_uid%")
        return liveService->pkUid;
    else if (key == "%pk_uname%")
        return liveService->pkUname;
    else if (key == "%pk_count%")
        return snum(!liveService->pkRoomId.isEmpty() ? us->danmakuCounts->value("pk/" + liveService->pkRoomId, 0).toInt() : 0);
    else if (key == "%pk_touta_prob%")
    {
        int prob = 0;
        if (liveService->pking && !liveService->pkRoomId.isEmpty())
        {
            int totalCount = us->danmakuCounts->value("pk/" + liveService->pkRoomId, 0).toInt() - 1;
            int toutaCount = us->danmakuCounts->value("touta/" + liveService->pkRoomId, 0).toInt();
            if (totalCount > 1)
                prob = toutaCount * 100 / totalCount;
        }
        return snum(prob);
    }

    else if (key == "%pk_my_votes%")
        return snum(liveService->myVotes);
    else if (key == "%pk_match_votes%")
        return snum(liveService->matchVotes);
    else if (key == "%pk_ending%")
        return snum(liveService->pkEnding ? 1 : 0);
    else if (key == "%pk_trans_gold%")
        return snum(liveService->goldTransPk);
    else if (key == "%pk_max_gold%")
        return snum(liveService->pkMaxGold);

    else if (key == "%pk_id%")
        return snum(liveService->pkId);

    // 房间属性
    else if (key == "%living%")
        return snum(ac->liveStatus);
    else if (key == "%room_id%")
        return ac->roomId;
    else if (key == "%room_name%" || key == "%room_title%")
        return toSingleLine(ac->roomTitle);
    else if (key == "%room_desc%")
        return toSingleLine(ac->roomDescription);
//        return ac->roomTitle;
    else if (key == "%up_name%" || key == "%up_uname%")
        return ac->upName;
    else if (key == "%up_uid%")
        return ac->upUid;
    else if (key == "%my_uid%")
        return ac->cookieUid;
    else if (key == "%my_uname%")
        return ac->cookieUname;
    else if (key == "%area_id%")
        return ac->areaId;
    else if (key == "%area_name%")
        return ac->areaName;
    else if (key == "%parent_area_id%")
        return ac->parentAreaId;
    else if (key == "%parent_area_name%")
        return ac->parentAreaName;
    else if (key == "%live_start_timestamp%")
        return snum(ac->liveStartTime);
    else if (key == "%live_duration_second%")
        return snum(QDateTime::currentSecsSinceEpoch() - ac->liveStartTime);
    else if (key == "%from_room_id%")
        return danmaku.getFromRoomId();

    // 是主播
    else if (key == "%is_up%")
        return danmaku.getUid() == ac->upUid ? "1" : "0";
    // 是机器人
    else if (key == "%is_me%")
        return danmaku.getUid() == ac->cookieUid ? "1" : "0";
    // 戴房间勋章
    else if (key == "%is_room_medal%")
        return danmaku.getAnchorRoomid() == ac->roomId ? "1" : "0";

    // 本地设置
    // 特别关心
    else if (key == "%care%")
        return us->careUsers.contains(danmaku.getUid()) ? "1" : "0";
    // 强提醒
    else if (key == "%strong_notify%")
        return us->strongNotifyUsers.contains(danmaku.getUid()) ? "1" : "0";
    // 是否被禁言
    else if (key == "%blocked%")
        return us->userBlockIds.contains(danmaku.getUid()) ? "1" : "0";
    // 不自动欢迎
    else if (key == "%not_welcome%")
        return us->notWelcomeUsers.contains(danmaku.getUid()) ? "1" : "0";
    // 不自动欢迎
    else if (key == "%not_reply%")
        return us->notReplyUsers.contains(danmaku.getUid()) ? "1" : "0";

    // 弹幕人气
    else if (key == "%danmu_popularity%")
        return snum(liveService->danmuPopulValue);

    // 游戏用户
    else if (key == "%in_game_users%")
        return gameUsers[0].contains(danmaku.getUid()) ? "1" : "0";
    else if (key == "%in_game_numbers%")
        return gameNumberLists[0].contains(danmaku.getNumber()) ? "1" : "0";
    else if (key == "%in_game_texts%")
        return gameTextLists[0].contains(danmaku.getText()) ? "1" : "0";

    // 程序文件、路径
    else if (key == "%app_name%")
        return rt->appFileName;

    else if (key == "%app_path%")
        return QDir(rt->dataPath).absolutePath();

    else if (key == "%www_path%")
        return webServer->wwwDir.absolutePath();

    else if (key == "%server_domain%")
        return webServer->serverDomain.contains("://") ? webServer->serverDomain : "http://" + webServer->serverDomain;

    else if (key == "%server_port%")
        return snum(webServer->serverPort);

    else if (key == "%server_url%")
        return webServer->serverDomain + ":" + snum(webServer->serverPort);

    // cookie
    else if (key == "%csrf%")
        return ac->csrf_token;

    // 工作状态
    else if (key == "%working%")
        return isWorking() ? "1" : "0";

    // 用户备注
    else if (key == "%umark%")
        return us->userMarks->value("base/" + danmaku.getUid()).toString();

    // 对面直播间也在用神奇弹幕
    else if (key == "%pk_magical_room%")
        return !liveService->pkRoomId.isEmpty() && liveService->magicalRooms.contains(liveService->pkRoomId) ? "1" : "0";

    // 正在播放的音乐
    else if (key == "%playing_song%")
    {
        QString name = "";
        if (musicWindow)
        {
            Song song = musicWindow->getPlayingSong();
            if (song.isValid())
                name = song.name;
        }
        return name;
    }
    // 点歌的用户
    else if (key == "%song_order_uname%")
    {
        QString name = "";
        if (musicWindow)
        {
            Song song = musicWindow->getPlayingSong();
            if (song.isValid())
                name = song.addBy;
        }
        return name;
    }
    // 点歌队列数量
    else if (key == "%order_song_count%")
    {
        QString text = "0";
        if (musicWindow)
        {
            text = snum(musicWindow->getOrderSongs().size());
        }
        return text;
    }
    else if (key == "%random100%")
    {
        return snum(qrand() % 100 + 1);
    }
    else if (key.indexOf(QRegularExpression("^%cd(\\d{1,2})%$"), 0, &match) > -1)
    {
        int ch = match.captured(1).toInt();
        qint64 time = msgCds[ch];
        qint64 curr = QDateTime::currentMSecsSinceEpoch();
        qint64 delta = curr - time;
        return snum(delta);
    }
    else if (key.indexOf(QRegularExpression("^%wait(\\d{1,2})%$"), 0, &match) > -1)
    {
        int ch = match.captured(1).toInt();
        return snum(msgWaits[ch]);
    }
    else if (key == "%local_mode%")
    {
        return us->localMode ? "1" : "0";
    }
    else if (key == "%repeat_10%")
    {
        return hasSimilarOldDanmaku(danmaku.getText()) ? "1" : "0";
    }
    else if (key == "%playing_tts%")
    {
        switch (voiceService->voicePlatform) {
        case VoiceLocal:
    #if defined(ENABLE_TEXTTOSPEECH)
            return (voiceService->tts && voiceService->tts->state() == QTextToSpeech::Speaking) ? "1" : "0";
    #else
            return "0";
    #endif
        case VoiceXfy:
            return (voiceService->xfyTTS && voiceService->xfyTTS->isPlaying()) ? "1" : "0";
        case VoiceMS:
            return (voiceService->msTTS && voiceService->msTTS->isPlaying()) ? "1" : "0";
        case VoiceCustom:
            return (voiceService->ttsDownloading || (voiceService->ttsPlayer && voiceService->ttsPlayer->state() == QMediaPlayer::State::PlayingState)) ? "1" : "0";
        }
    }
    else if (key == "%mouse_x%")
    {
        return snum(QCursor().pos().x());
    }
    else if (key == "%mouse_y%")
    {
        return snum(QCursor().pos().y());
    }
    else
    {
        *ok = false;
        return "";
    }
}

QString CodeRunner::generateCodeFunctions(const QString &funcName, const QString &args, const LiveDanmaku &danmaku, bool *ok) const
{
    if (funcName == "traverseJson")
    {
        QRegularExpression re("(.+?)\\s*,\\s*(.*)");
        QRegularExpressionMatch match;
        if (args.contains(re, &match))
        {
            QString keySeq = match.captured(1).trimmed();
            QString code = match.captured(2).trimmed();
            *ok = true;
            return traverseJsonCode(keySeq, code, danmaku.extraJson);
        }
    }

    *ok = false;
    qWarning() << "无法解析：" << funcName << args;
    return "";
}

QJsonValue getJsonValueByKeySeq(QString keySeq, const QJsonObject& json)
{
    while (keySeq.startsWith(".")) // 第一个点是自己
        keySeq = keySeq.mid(1);
    QStringList keyTree = keySeq.split(".");
    if (keyTree.size() == 0)
    {
        qWarning() << "获取JSON值：路径为空";
        return "";
    }

    QJsonValue obj = json;
    while (keyTree.size())
    {
        QString key = keyTree.takeFirst();
        bool canIgnore = false;
        if (key.endsWith("?"))
        {
            canIgnore = true;
            key = key.left(key.length() - 1);
        }

        if (key.isEmpty()) // 居然是空字符串，是不是有问题
        {
            if (canIgnore)
            {
                obj = QJsonValue();
                break;
            }
            else
            {
                qWarning() << "解析JSON的键是空的：" << keySeq;
                return "";
            }
        }
        else if (obj.isObject())
        {
            if (!obj.toObject().contains(key)) // 不包含这个键
            {
                // qWarning() << "不存在的键：" << key << "    keys:" << key_seq;
                if (canIgnore)
                {
                    obj = QJsonValue();
                    break;
                }
                else
                {
                    // 可能是等待其他的变量，所以先留下
                    return "";
                }
            }
            obj = obj.toObject().value(key);
        }
        else if (obj.isArray())
        {
            int index = key.toInt();
            QJsonArray array = obj.toArray();
            if (index >= 0 && index < array.size())
            {
                obj = array.at(index);
            }
            else
            {
                if (canIgnore)
                {
                    obj = QJsonValue();
                    break;
                }
                else
                {
                    qWarning() << "错误的数组索引：" << index << "当前总数量：" << array.size() << "  " << keySeq;
                    return "";
                }
            }
        }
        else
        {
            qWarning() << "未知的JSON类型：" << keySeq;
            obj = QJsonValue();
            break;
        }
    }
    return obj;
}

/**
 * 额外数据（JSON）替换
 */
QString CodeRunner::replaceDanmakuJson(const QString &keySeq, const QJsonObject &json, bool* ok) const
{
    QJsonValue obj = getJsonValueByKeySeq(keySeq, json);
    *ok = true;

    if (obj.isNull() || obj.isUndefined())
        return "";
    if (obj.isString())
        return toSingleLine(obj.toString());
    if (obj.isBool())
        return obj.toBool(false) ? "1" : "0";
    if (obj.isDouble())
    {
        double val = obj.toDouble();
        if (qAbs(val - qint64(val)) < 1e-6) // 是整数类型的
            return QString::number(qint64(val));
        return QString::number(val);
    }
    if (obj.isObject()) // 不支持转换的类型
        return QJsonDocument(obj.toObject()).toJson(QJsonDocument::Compact);
    if (obj.isArray())
        return QJsonDocument(obj.toArray()).toJson(QJsonDocument::Compact);
    return "";
}

/**
 * 转换 traverseJson 函数为代码
 */
QString CodeRunner::traverseJsonCode(const QString &keySeq, const QString &code, const QJsonObject &json) const
{
    // 解析JSON
    QJsonValue jv = getJsonValueByKeySeq(keySeq, json);
    if (!jv.isNull())
        qDebug() << "遍历JSON：" << keySeq << code;
    QStringList codes;
    if (jv.isArray())
    {
        QJsonArray array = jv.toArray();
        for (int i = 0; i < array.size(); i++)
        {
            QString c = cr->toRunableCode(cr->toMultiLine(code));
            c.replace("%i%", QString::number(i));
            codes.append(c);
        }
    }
    else if (jv.isObject())
    {
        QJsonObject obj = jv.toObject();
        foreach (QString key, obj.keys())
        {
            QString c = cr->toRunableCode(cr->toMultiLine(code));
            c.replace("%key%", key);
            c.replace("%value%", cr->toSingleLine(MyJson(obj.value(key).toObject()).toBa(QJsonDocument::Compact)));
            codes.append(c);
        }
    }
    return codes.join("\\n");
}

/**
 * 函数替换
 */
QString CodeRunner::replaceDynamicVariants(const QString &funcName, const QString &args, const LiveDanmaku& danmaku)
{
    QRegularExpressionMatch match;
    QStringList argList = args.split(QRegExp("\\s*,\\s*"));
    auto errorArg = [=](QString tip){
        localNotify("函数%>"+funcName+"()%参数错误: " + tip);
        return "";
    };

    // 替换时间
    if (funcName == "simpleName")
    {
        LiveDanmaku ld = danmaku;
        ld.setNickname(args);
        return nicknameSimplify(ld);
    }
    else if (funcName == "simpleNum")
    {
        return numberSimplify(args.toInt());
    }
    else if (funcName == "time")
    {
        return QDateTime::currentDateTime().toString(args);
    }
    else if (funcName == "unameToUid")
    {
        return unameToUid(args);
    }
    else if (funcName == "ignoreEmpty")
    {
        QString s = args.trimmed();
        if (s.isEmpty())
            return "";
        s = s.replace(" ", "").replace("\t", "");
        if (s == "()" || s == "（）"
            || s == "[]" || s == "【】"
            || s == "{}" || s == "「」"
            || s == "<>" || s == "《》"
            || s == "\"\"" || s == "“”")
            return "";
        return args;
    }
    else if (funcName == "inGameUsers")
    {
        int ch = 0;
        UIDT uid = 0;
        if (argList.size() == 1)
            uid = argList.first().toLongLong();
        else if (argList.size() >= 2)
        {
            ch = argList.at(0).toInt();
            uid = argList.at(1).toLongLong();
        }
        if (ch < 0 || ch >= CHANNEL_COUNT)
            ch = 0;
        return gameUsers[ch].contains(uid) ? "1" : "0";
    }
    else if (funcName == "inGameNumbers")
    {
        int ch = 0;
        qint64 id = 0;
        if (argList.size() == 1)
            id = argList.first().toLongLong();
        else if (argList.size() >= 2)
        {
            ch = argList.at(0).toInt();
            id = argList.at(1).toLongLong();
        }
        if (ch < 0 || ch >= CHANNEL_COUNT)
            ch = 0;
        return gameNumberLists[ch].contains(id) ? "1" : "0";
    }
    else if (funcName == "inGameTexts")
    {
        int ch = 0;
        QString text;
        if (argList.size() == 1)
            text = argList.first();
        else if (argList.size() >= 2)
        {
            ch = argList.at(0).toInt();
            text = argList.at(1);
        }
        if (ch < 0 || ch >= CHANNEL_COUNT)
            ch = 0;
        return gameTextLists[ch].contains(text) ? "1" : "0";
    }
    else if (funcName == "strlen")
    {
        return snum(args.size());
    }
    else if (funcName == "trim")
    {
        return args.trimmed();
    }
    else if (funcName == "substr")
    {
        if (argList.size() < 2)
            return errorArg("字符串, 起始位置, 长度");

        QString text = argList.at(0);
        int left = 0, len = text.length();
        if (argList.size() >= 2)
            left = argList.at(1).toInt();
        if (argList.size() >= 3)
            len = argList.at(2).toInt();
        if (left < 0)
            left = 0;
        else if (left > text.length())
            left = text.length();
        return text.mid(left, len);
    }
    else if (funcName == "replace")
    {
        if (argList.size() < 3)
            return errorArg("字符串, 原文本, 新文本");

        QString text = argList.at(0);
        return text.replace(argList.at(1), argList.at(2));
    }
    else if (funcName == "replaceReg" || funcName == "regReplace")
    {
        if (argList.size() < 3)
            return errorArg("字符串, 原正则, 新文本");

        QString text = argList.at(0);
        return text.replace(QRegularExpression(argList.at(1)), argList.at(2));
    }
    else if (funcName == "reg")
    {
        if (argList.size() < 2)
            return errorArg("字符串, 正则表达式");
        int index = args.lastIndexOf(",");
        if (index == -1) // 不应该没找到的
            return "";
        QString full = args.left(index);
        QString reg = args.right(args.length() - index - 1).trimmed();

        QRegularExpressionMatch match;
        if (full.indexOf(QRegularExpression(reg), 0, &match) == -1)
            return "";
        return match.captured(0);
    }
    else if (funcName == "inputText")
    {
        QString label = argList.size() ? argList.first() : "";
        QString def = argList.size() >= 2 ? argList.at(1) : "";
        QString rst = QInputDialog::getText(rt->mainwindow, QApplication::applicationName(), label, QLineEdit::Normal, def);
        return rst;
    }
    else if (funcName == "getValue")
    {
        if (argList.size() < 1 || argList.first().trimmed().isEmpty())
            return errorArg("键, 默认值");
        QString key = argList.at(0);
        QString def = argList.size() >= 2 ? argList.at(1) : "";
        if (!key.contains("/"))
            key = "heaps/" + key;
        return heaps->value(key, def).toString();
    }
    else if (funcName == "random")
    {
        if (argList.size() < 1 || argList.first().trimmed().isEmpty())
            return errorArg("最小值，最大值");
        int min = 0, max = 100;
        if (argList.size() == 1)
            max = argList.at(0).toInt();
        else
        {
            min = argList.at(0).toInt();
            max = argList.at(1).toInt();
        }
        if (max < min)
        {
            int t = min;
            min = max;
            max = t;
        }
        return snum(qrand() % (max-min+1) + min);
    }
    else if (funcName == "randomArray")
    {
        if (!argList.size())
            return "";
        int r = qrand() % argList.size();
        return argList.at(r);
    }
    else if (funcName == "filterReject")
    {
        if (argList.size() < 1 || argList.first().trimmed().isEmpty())
            return errorArg("最小值，最大值");
        QString filterName = args;
        return isFilterRejected(filterName, danmaku) ? "1" : "0";
    }
    else if (funcName == "inFilterList") // 匹配过滤器的内容，空白符分隔
    {
        if (argList.size() < 2)
            return errorArg("过滤器名, 全部内容");
        int index = args.indexOf(",");
        if (index == -1) // 不应该没找到的
            return "";

        QString filterName = args.left(index);
        QString content = args.right(args.length() - index - 1).trimmed();
        qInfo() << ">inFilterList:" << filterName << content;
        for (int row = 0; row < ui->eventListWidget->count(); row++)
        {
            auto widget = ui->eventListWidget->itemWidget(ui->eventListWidget->item(row));
            auto tw = static_cast<EventWidget*>(widget);
            if (tw->eventEdit->text() != filterName || !tw->check->isChecked())
                continue;

            QStringList sl = tw->body().split(QRegularExpression("\\s+"), QString::SkipEmptyParts);
            foreach (QString s, sl)
            {
                if (content.contains(s))
                {
                    qInfo() << "keys:" << sl;
                    return "1";
                }
            }
        }
        return "0";
    }
    else if (funcName == "inFilterMatch") // 匹配过滤器的正则
    {
        if (argList.size() < 2)
            return errorArg("过滤器名, 全部内容");
        int index = args.indexOf(",");
        if (index == -1) // 不应该没找到的
            return "";
        QString filterName = args.left(index);
        QString content = args.right(args.length() - index - 1).trimmed();
        qInfo() << ">inFilterMatch:" << filterName << content;
        for (int row = 0; row < ui->eventListWidget->count(); row++)
        {
            auto widget = ui->eventListWidget->itemWidget(ui->eventListWidget->item(row));
            auto tw = static_cast<EventWidget*>(widget);
            if (tw->eventEdit->text() != filterName || !tw->check->isChecked())
                continue;

            QStringList sl = tw->body().split("\n", QString::SkipEmptyParts);
            foreach (QString s, sl)
            {
                if (content.indexOf(QRegularExpression(s)) > -1)
                {
                    qInfo() << "regs:" << sl;
                    return "1";
                }
            }
        }
        return "0";
    }
    else if (funcName == "abs")
    {
        QString val = args.trimmed();
        if (val.startsWith("-"))
            val.remove(0, 1);
        return val;
    }
    else if (funcName == "log2")
    {
        if (argList.size() < 1)
        {
            return errorArg("数值");
        }

        double a = argList.at(0).toDouble();
        return QString::number(int(log2(a)));
    }
    else if (funcName == "log10")
    {
        if (argList.size() < 1)
        {
            return errorArg("数值");
        }

        double a = argList.at(0).toDouble();
        return QString::number(int(log10(a)));
    }
    else if (funcName == "pow")
    {
        if (argList.size() < 2)
        {
            return errorArg("底数, 指数");
        }

        double a = argList.at(0).toDouble();
        double b = argList.at(1).toDouble();
        return QString::number(int(pow(a, b)));
    }
    else if (funcName == "pow2")
    {
        if (argList.size() < 1)
        {
            return errorArg("底数");
        }

        double a = argList.at(0).toDouble();
        return QString::number(int(a * a));
    }
    else if (funcName == "cd")
    {
        int ch = 0;
        if (argList.size() > 0)
            ch = argList.at(0).toInt();
        qint64 time = msgCds[ch];
        qint64 curr = QDateTime::currentMSecsSinceEpoch();
        qint64 delta = curr - time;
        return snum(delta);
    }
    else if (funcName == "wait")
    {
        int ch = 0;
        if (argList.size() > 0)
            ch = argList.at(0).toInt();
        return snum(msgWaits[ch]);
    }
    else if (funcName == "pasteText")
    {
        return QApplication::clipboard()->text();
    }
    else if (funcName == "getScreenPositionColor") // 获取屏幕上某个点的颜色
    {
        if (argList.size() < 3)
        {
            return errorArg("屏幕ID, 横坐标, 纵坐标");
        }
        int wid = argList.at(0).toInt(); // 屏幕ID
        int x = argList.at(1).toInt(); // 横坐标
        int y = argList.at(2).toInt(); // 纵坐标

        auto screens = QGuiApplication::screens();
        if (wid < 0 || wid >= screens.size())
        {
            liveService->showError("getScreenPositionColor", "没有该屏幕：" + QString::number(wid)
                      + "/" + QString::number(screens.size()));
            return "";
        }
        QScreen *screen = screens.at(wid);
        QPixmap pixmap = screen->grabWindow(wid, x, y, 1, 1);
        QColor color = pixmap.toImage().pixelColor(0, 0);
        return QVariant(color).toString();
    }
    else if (funcName == "getWindowPositionColor") // 获取指定窗口某个点的颜色
    {
        QString name = argList.at(0); // 窗口名字
        bool isId = false;
        int useId = name.toInt(&isId);
        int x = argList.at(1).toInt(); // 横坐标
        int y = argList.at(2).toInt(); // 纵坐标

        qint64 wid = 0;
        // 获取窗口句柄
#ifdef Q_OS_WIN
        {
            HWND pWnd = first_window(EXCLUDE_MINIMIZED); // 得到第一个窗口句柄
            while (pWnd)
            {
                QString title = get_window_title(pWnd);
                QString clss = get_window_class(pWnd);

                if (!title.isEmpty())
                {
                    if (title.contains(name) || clss.contains(name)
                            || (isId && (useId == reinterpret_cast<qint64>(pWnd))) )
                    {
                        wid = reinterpret_cast<qint64>(pWnd);
                        break;
                    }
                }

                pWnd = next_window(pWnd, EXCLUDE_MINIMIZED); // 得到下一个窗口句柄
            }
        }
        if (wid == 0)
        {
            liveService->showError("getWindowPositionColor", "无法查询到窗口句柄");
            return "";
        }
#endif

        QScreen *screen = QGuiApplication::primaryScreen();
        QPixmap pixmap = screen->grabWindow(wid, x, y, 1, 1);
        QColor color = pixmap.toImage().pixelColor(0, 0);
        return QVariant(color).toString();
    }
    else if (funcName == "execReplyResult" || funcName == "getReplyExecutionResult")
    {
        if (argList.size() < 1 || args.isEmpty())
        {
            return errorArg("回复内容");
        }
        QString key = args;
        return getReplyExecutionResult(key, danmaku);
    }
    else if (funcName == "execEventResult" || funcName == "getEventExecutionResult")
    {
        if (argList.size() < 1 || args.isEmpty())
        {
            return errorArg("事件名字");
        }
        QString key = args;
        return getEventExecutionResult(key, danmaku);
    }
    else if (funcName == "readTextFile")
    {
        if (argList.size() < 1 || args.isEmpty())
            return errorArg("文件路径");
        QString fileName = toFilePath(argList.at(0));
        QString content = readTextFileAutoCodec(fileName);
        return toSingleLine(content);
    }
    else if (funcName == "fileExists" || funcName == "isFileExist")
    {
        if (argList.size() < 1 || args.isEmpty())
            return errorArg("文件路径");
        QString fileName = toFilePath(argList.at(0));
        return isFileExist(fileName) ? "1" : "0";
    }
    else if (funcName == "getTextFileLine")
    {
        if (argList.size() < 1 || args.isEmpty())
            return errorArg("文件路径");
        QString fileName = toFilePath(argList.at(0));
        QString content = readTextFileAutoCodec(fileName);
        QStringList sl = content.split("\n");
        int line = argList.at(1).toInt();
        line--;
        if (line < 0 || line >= sl.size())
        {
            liveService->showError("getTextFileLine", "错误的行数：" + snum(line+1) + "不在1~" + snum(sl.size()) + "之间");
            return "";
        }
        return sl.at(line);
    }
    else if (funcName == "getTextFileLineCount")
    {
        if (argList.size() < 1 || args.isEmpty())
            return errorArg("文件路径");
        QString fileName = toFilePath(argList.at(0));
        QString content = readTextFileAutoCodec(fileName);
        return snum(content.split("\n").size());
    }
    else if (funcName == "urlEncode")
    {
        if (argList.size() < 1 || args.isEmpty())
            return errorArg("字符串");
        return QByteArray(toMultiLine(args).toUtf8()).toPercentEncoding();
    }
    else if (funcName == "urlDecode")
    {
        if (argList.size() < 1 || args.isEmpty())
            return errorArg("字符串");
        return toSingleLine(QByteArray::fromPercentEncoding(args.toUtf8()));
    }
    else if (funcName == "compareScreenShot")
    {
        if (argList.size() < 6)
            return errorArg("");

        int id = argList.at(0).toInt();
        int x = argList.at(1).toInt();
        int y = argList.at(2).toInt();
        int w = argList.at(3).toInt();
        int h = argList.at(4).toInt();
        QString path = argList.at(5);
        QString type = "";
        if (argList.size() > 6)
            type = argList.at(6).toLower();

        auto screens = QGuiApplication::screens();
        if (id < 0 || id >= screens.size())
        {
            liveService->showError("保存屏幕截图", "错误的屏幕ID：" + snum(id));
            return "0";
        }

        QScreen *screen = screens.at(id);
        QImage image = screen->grabWindow(QApplication::desktop()->winId(), x, y, w, h).toImage();

        QImage comparedImage;
        if (cacheImages.contains(path))
        {
            comparedImage = cacheImages[path];
        }
        else
        {
            if (!comparedImage.load(path))
            {
                liveService->showError("加载图片", "加载图片失败：" + path);
                return "0";
            }
            qInfo() << "加载本地图片进缓存：" << path;
            cacheImages[path] = comparedImage;
        }

        if (type.isEmpty() || type == "pixel")
        {
            int threshold = 8;
            if (argList.size() > 7)
                threshold = argList.at(7).toInt();
            return snum(int(ImageSimilarityUtil::compareImageByPixel(image, comparedImage, us->imageSimilarPrecision, threshold) * 100));
        }
        else if (type == "ahash")
        {
            return snum(ImageSimilarityUtil::aHash(image, comparedImage));
        }
        else if (type == "dhash")
        {
            return snum(ImageSimilarityUtil::dHash(image, comparedImage));
        }
        else if (type == "phash")
        {
            if (image.width() * comparedImage.height() != image.height() * comparedImage.width()) // 相同的宽高比
            {
                liveService->showError("比较图片", "图片比例不同，无法比较");
                return "0";
            }
            // TODO:phash
        }
        else
        {
            liveService->showError("比较图片", "不支持的图片相似度算法：" + type);
            return "0";
        }
    }
    else if (funcName == "getScreenWidth")
    {
        int screenId = 0;
        if (argList.size() > 0)
            screenId = argList.at(0).toInt();
        auto screens = QGuiApplication::screens();
        if (screenId < 0 || screenId >= screens.size())
        {
            liveService->showError(funcName, "错误的屏幕ID：" + snum(screenId));
            return "0";
        }

        QScreen *screen = screens.at(screenId);
        return snum(screen->geometry().width());
    }
    else if (funcName == "getScreenHeight")
    {
        int screenId = 0;
        if (argList.size() > 0)
            screenId = argList.at(0).toInt();
        auto screens = QGuiApplication::screens();
        if (screenId < 0 || screenId >= screens.size())
        {
            liveService->showError(funcName, "错误的屏幕ID：" + snum(screenId));
            return "0";
        }

        QScreen *screen = screens.at(screenId);
        return snum(screen->geometry().height());
    }
    else if (funcName == "findWindow" || funcName == "findWindowByName") // 通过类名或窗口标题来获取句柄
    {
        if (argList.size() < 1)
            return errorArg("窗口标题名/类名");
        QString winName = argList.at(0);
#ifdef Q_OS_WIN
        HWND hwnd = FindWindow(nullptr, static_cast<LPCWSTR>(winName.toStdWString().c_str()));
        qInfo() << funcName << winName << hwnd;
        return snum((long long)hwnd); // 如果找不到窗口，那么可能会是空的
#else
        return "0";
#endif
    }
    else if (funcName == "getForegroundWindow")
    {
#ifdef Q_OS_WIN
        HWND hWnd = GetForegroundWindow();
        qInfo() << funcName << hWnd;
        return snum((long long)hWnd); // 如果找不到窗口，那么可能会是空的
#else
        return "0";
#endif
    }
    else if (funcName == "getWindowState") // 窗口是否是全屏（最大化不算）
    {
        if (argList.size() < 2)
            return errorArg("窗口句柄");
#ifdef Q_OS_WIN
        HWND hWnd = (HWND)argList.at(0).toLongLong();
        if (hWnd == nullptr)
        {
            liveService->showError(funcName, "无法获取窗口ID");
            return "0";
        }

        //参数2取值定义如下：
        //0 : 判断窗口是否存在
        //1 : 判断窗口是否处于激活
        //2 : 判断窗口是否可见
        //3 : 判断窗口是否最小化
        //4 : 判断窗口是否最大化
        //5 : 判断窗口是否置顶
        //6 : 判断窗口是否无响应
        QString typer = argList.at(1).toLower();
        int type = 0;
        if (typer.contains("exist"))
            type = 0;
        else if (typer.contains("activ"))
            type = 1;
        else if (typer.contains("vis"))
            type = 2;
        else if (typer.contains("min"))
            type = 3;
        else if (typer.contains("max"))
            type = 4;
        else if  (typer.contains("top"))
            type = 5;
        else if (typer.contains("hung"))
            type = 6;

        // int result = GetWindowState(hWnd, type); // 暂时不支持这个函数
        // return result ? "1" : "0";
        return "0";
#else
        return "0";
#endif
    }
    else if (funcName == "isWindowFullScreen") // 窗口是否是全屏（最大化不算）
    {
        if (argList.size() < 1)
            return errorArg("窗口句柄");
#ifdef Q_OS_WIN
        HWND hWnd = (HWND)argList.at(0).toLongLong();
        if (hWnd == nullptr)
        {
            liveService->showError(funcName, "无法获取窗口ID");
            return "0";
        }

        //获取顶层窗口的矩形范围
        RECT top_window_rect;
        if(!GetWindowRect(hWnd, &top_window_rect))
        {
            return FALSE;
        }

        //获取窗口所在显示器
        HMONITOR monitor_hwnd = MonitorFromRect(&top_window_rect, MONITOR_DEFAULTTONULL);
        if(!monitor_hwnd)return FALSE;

        //获取显示器的信息
        MONITORINFO monitor_info;
        monitor_info.cbSize = sizeof(monitor_info);
        GetMonitorInfo(monitor_hwnd, &monitor_info);

        //判断显示器的显示范围和顶层窗口是否相同,相同就是全屏显示了
        //不相同就不是全屏显示
        bool eq = EqualRect(&top_window_rect, &(monitor_info.rcMonitor));
        return eq ? "1" : "0";
#else
        return "0";
#endif
    }
    else if (funcName == "getCursorPos")
    {
        if (argList.size() < 1)
            return errorArg("x/y");
        QString type = argList.at(0).toLower();
        if (type.contains("x"))
            return snum(QCursor::pos().x());
        else if (type.contains("y"))
            return snum(QCursor::pos().y());
        else
            return "0";
    }
    else if (funcName == "getWindowRect")
    {
        if (argList.size() < 2)
            return errorArg("窗口句柄, 尺寸类型");
#ifdef Q_OS_WIN
        HWND hWnd = (HWND)argList.at(0).toLongLong();
        if (hWnd == nullptr)
        {
            liveService->showError(funcName, "无法获取窗口ID");
            return "0";
        }

        //获取顶层窗口的矩形范围
        RECT top_window_rect;
        if(!GetWindowRect(hWnd, &top_window_rect))
        {
            liveService->showError("无法获取窗口尺寸");
            return "0";
        }

        QString type = argList.at(1).toLower();
        if (type.startsWith("x") || type.startsWith("l")) // x / left
            return snum(top_window_rect.left);
        if (type.startsWith("y") || type.startsWith("t")) // y / top
            return snum(top_window_rect.top);
        if (type.startsWith("r")) // y / right
            return snum(top_window_rect.right);
        if (type.startsWith("b")) // y / bottom
            return snum(top_window_rect.bottom);
        if (type.startsWith("w")) // y / width
            return snum(top_window_rect.right - top_window_rect.left);
        if (type.startsWith("h")) // y / height
            return snum(top_window_rect.bottom - top_window_rect.top);

#else
        return "0";
#endif
    }
    else if (funcName == "getWindowFromPoint")
    {
        if (argList.size() < 2)
            return errorArg("x, y");
        int x = argList.at(0).toInt();
        int y = argList.at(1).toInt();
#ifdef Q_OS_WIN
        POINT curpos {x, y};
        HWND hWnd = WindowFromPoint(curpos);
        return snum((long long)hWnd);
#endif
    }
    else if (funcName == "getDanmuHistory")
    {
        // uid, count, type, format, options
        if (argList.size() < 2)
            return errorArg("uid, count");
        QString uid = argList.at(0); // 指定UID或者所有
        int count = argList.at(1).toInt(); // 最大弹幕数量（仅限本次监听有的）
        QString format = argList.size() > 2 ? argList.at(2).toLower() : ""; // 每行弹幕的格式
        QString type = argList.size() > 3 ? argList.at(3).toLower() : ""; // 输出类型
        QString options = argList.size() > 4 ? argList.at(4).toLower() : ""; // 其他选项
        
        if (count == 0)
            count = 100;

        // 获取历史弹幕
        QList<LiveDanmaku> danmuList;
        if (uid.isEmpty() || uid == "0" || uid == "all") // 所有
        {
            danmuList = liveService->getAllDanmus(count);
        }
        else // 指定 UID
        {
            danmuList = liveService->getDanmusByUID(uid, count);
        }
        if (danmuList.isEmpty())
        {
            qWarning() << "没有找到弹幕" << args;
            return "";
        }

        // 其他选项
        if (options.contains("reverse"))
        {
            std::reverse(danmuList.begin(), danmuList.end());
        }
        if (options.contains("sort_by_time"))
        {
            std::sort(danmuList.begin(), danmuList.end(), [](const LiveDanmaku &a, const LiveDanmaku &b) {
                return a.getTimeline() < b.getTimeline();
            });
        }
        if (options.contains("sort_by_uid"))
        {
            std::sort(danmuList.begin(), danmuList.end(), [](const LiveDanmaku &a, const LiveDanmaku &b) {
                return a.getUid() < b.getUid();
            });
        }
        // 总字数上限（按每条长度的和）
        QRegularExpression re("word_count\\s*<\\s*(\\d+)");
        QRegularExpressionMatch match;
        int wordCount = 0;
        if (options.contains(re, &match))
        {
            wordCount = match.captured(1).toInt();
        }

        // 每行弹幕的格式
        if (format.isEmpty())
            format = "{time(MM-dd hh:mm)} {uname}:{text}";
        auto toFormatLine = [=](const LiveDanmaku &danmu) -> QString {
            QString line = format;
            QRegularExpression re("\\{time\\((.*?)?\\)\\}\\s*");
            QRegularExpressionMatch match;
            if (line.contains(re, &match))
            {
                QString time = match.captured(0);
                QString timeFormat = match.captured(1);
                line.replace(time, danmu.getTimeline().toString(timeFormat));
            }
            line.replace("{uid}", danmu.getUid());
            line.replace("{uname}", danmaku.getNickname());
            line.replace("{text}", danmu.getText());
            return line;
        };

        // 输出的类型：line, text, json, list, gpt
        if (type.isEmpty())
            type = "text";
        // 生成结果并返回
        if (type == "line" || type == "text" || type == "list") // 输出为文本
        {
            QStringList lines;
            foreach (LiveDanmaku danmu, danmuList)
            {
                lines.append(toFormatLine(danmu));
            }

            if (wordCount > 0)
            {
                int total = 0;
                for (int i = lines.size() - 1; i >= 0; i--)
                {
                    QString line = lines.at(i);
                    total += line.length();
                    if (total > wordCount)
                    {
                        lines = lines.mid(i);
                        break;
                    }
                }
            }
            if (type == "line")
                return lines.join(";");
            else if (type == "list")
                return lines.join("\n");
            else
                return lines.join("%n%");
        }
        else if (type == "json")
        {
            QJsonArray jsonArray;
            foreach (LiveDanmaku danmu, danmuList)
            {
                QJsonObject jsonObj;
                jsonObj["time"] = danmu.getTimeline().toString();
                jsonObj["uid"] = danmu.getUid();
                jsonObj["text"] = danmu.getText();
                jsonArray.append(jsonObj);
            }
            if (wordCount > 0)
            {
                int total = 0;
                QJsonArray newArray;
                for (int i = jsonArray.size() - 1; i >= 0; i--)
                {
                    QJsonObject jsonObj = jsonArray.at(i).toObject();
                    total += jsonObj["text"].toString().length();
                    if (total > wordCount)
                    {
                        // 从当前位置开始,将剩余元素添加到新数组
                        for (int j = i; j < jsonArray.size(); j++)
                        {
                            newArray.append(jsonArray.at(j));
                        }
                        jsonArray = newArray;
                        break;
                    }
                }
            }
            return QJsonDocument(jsonArray).toJson(QJsonDocument::Compact);
        }
        else if (type == "gpt") // 输出为 GPT 的 JSON 格式
        {
            QJsonArray jsonArray;
            foreach (LiveDanmaku danmu, danmuList)
            {
                QJsonObject jsonObj;
                jsonObj["role"] = "user";
                jsonObj["content"] = toFormatLine(danmu);
                jsonArray.append(jsonObj);
            }
            if (wordCount > 0)
            {
                int total = 0;
                QJsonArray newArray;
                for (int i = jsonArray.size() - 1; i >= 0; i--)
                {
                    QJsonObject jsonObj = jsonArray.at(i).toObject();
                    total += jsonObj["content"].toString().length();
                    if (total > wordCount)
                    {
                        // 从当前位置开始,将剩余元素添加到新数组
                        for (int j = i; j < jsonArray.size(); j++)
                        {
                            newArray.append(jsonArray.at(j));
                        }
                        jsonArray = newArray;
                        break;
                    }
                }
            }
            QString full = QJsonDocument(jsonArray).toJson(QJsonDocument::Compact);
            full = full.mid(1, full.length() - 2); // 去掉前后的[]，因为要放在gpt的参数里面
            return full;
        }
        else
        {
            liveService->showError(funcName, "无法识别的输出类型：" + type);
        }
    }

    return "";
}

/**
 * 处理条件变量
 * [exp1, exp2]...
 * 要根据时间戳、字符串
 * @return 如果返回空字符串，则不符合；否则返回去掉表达式后的正文
 */
QString CodeRunner::processMsgHeaderConditions(QString msg) const
{
    QString condRe;
    if (msg.contains(QRegularExpression("^\\s*\\[\\[\\[.*\\]\\]\\]")))
        condRe = "^\\s*\\[\\[\\[(.*?)\\]\\]\\]\\s*";
    else if (msg.contains(QRegularExpression("^\\s*\\[\\[.*\\]\\]"))) // [[%text% ~ "[\\u4e00-\\u9fa5]+[\\w]{3}[\\u4e00-\\u9fa5]+"]]
        condRe = "^\\s*\\[\\[(.*?)\\]\\]\\s*";
    else
        condRe = "^\\s*\\[(.*?)\\]\\s*";
    QRegularExpression re(condRe);
    if (!re.isValid())
        liveService->showError("无效的条件表达式", condRe);
    QRegularExpressionMatch match;
    if (msg.indexOf(re, 0, &match) == -1) // 没有检测到条件表达式，直接返回
        return msg;

    CALC_DEB << "条件表达式：" << match.capturedTexts();
    QString totalExp = match.capturedTexts().first(); // 整个表达式，带括号
    QString exprs = match.capturedTexts().at(1);

    if (!ConditionUtil::judgeCondition(exprs))
        return "";
    return msg.right(msg.length() - totalExp.length());
}

template<typename T>
bool CodeRunner::isConditionTrue(T a, T b, QString op) const
{
    if (op == "==" || op == "=")
        return a == b;
    if (op == "!=" || op == "<>")
        return a != b;
    if (op == ">")
        return a > b;
    if (op == ">=")
        return a >= b;
    if (op == "<")
        return a < b;
    if (op == "<=")
        return a <= b;
    qWarning() << "无法识别的比较模板类型：" << a << op << b;
    return false;
}

bool CodeRunner::isFilterRejected(QString filterName, const LiveDanmaku &danmaku)
{
    if (!enableFilter || rt->justStart)
        return false;

    qDebug() << "触发过滤器：" << filterName;
    // 查找所有事件，查看有没有对应的过滤器
    bool reject = false;
    for (int row = 0; row < ui->eventListWidget->count(); row++)
    {
        auto rowItem = ui->eventListWidget->item(row);
        auto widget = ui->eventListWidget->itemWidget(rowItem);
        if (!widget)
            continue;
        auto eventWidget = static_cast<EventWidget*>(widget);
        if (eventWidget->isEnabled() && eventWidget->title() == filterName)
        {
            QString filterCode = eventWidget->body();
            // 判断事件
            if (!processFilter(filterCode, danmaku))
            {
                qDebug() << "已过滤，代码：" << filterCode.left(20) + "...";
                reject = true;
            }
        }
    }

    return reject;
}

/**
 * 判断是否通过（没有reject）
 */
bool CodeRunner::processFilter(QString filterCode, const LiveDanmaku &danmaku)
{
    if (filterCode.isEmpty())
        return true;

    bool ok = false;
    filterCode = replaceCodeLanguage(filterCode, danmaku, &ok);
    if (ok)
    {
        return processFilter(filterCode, danmaku);
    }

    // 获取符合的所有结果
    QStringList msgs = getEditConditionStringList(filterCode, danmaku);
    if (!msgs.size())
        return true;

    // 如果有多个，随机取一个
    int r = qrand() % msgs.size();
    QString s = msgs.at(r);
    bool reject = s.contains(QRegularExpression(">\\s*reject\\s*(\\s*)"));
    if (reject)
        qInfo() << "已过滤:" << filterCode;

    if (reject && !s.contains("\\n")) // 拒绝，且不需要其他操作，直接返回
    {
        return false;
    }

    if (!s.trimmed().isEmpty()) // 可能还有其他的操作
    {
        sendAutoMsg(s, danmaku);
    }

    qDebug() << "过滤器结果：阻止=" << reject;
    return !reject;
}

/// 替换 unicode
/// 即 \u4000 这种
void CodeRunner::translateUnicode(QString &s) const
{
    s.replace("\\中文", "\u4e00-\u9fa5");
}

/**
 * 获取第一个符合条件的回复的文本
 * 会返回所有文本，如果有换行符，则会以脚本的 \n（实际上是\\n）来保存
 * 如果有命令，会继续保持文本形式，而不会触发
 * 仅判断启用的
 */
QString CodeRunner::getReplyExecutionResult(QString key, const LiveDanmaku& danmaku)
{
    for (int row = 0; row < ui->replyListWidget->count(); row++)
    {
        auto rowItem = ui->replyListWidget->item(row);
        auto widget = ui->replyListWidget->itemWidget(rowItem);
        if (!widget)
            continue;
        auto replyWidget = static_cast<ReplyWidget*>(widget);
        if (replyWidget->isEnabled() && key.contains(QRegularExpression(replyWidget->title())))
        {
            QString filterText = replyWidget->body();
            QStringList msgs = getEditConditionStringList(filterText, danmaku);
            return getExecutionResult(msgs, danmaku);
        }
    }
    return "";
}

QString CodeRunner::getEventExecutionResult(QString key, const LiveDanmaku &danmaku)
{
    for (int row = 0; row < ui->eventListWidget->count(); row++)
    {
        auto rowItem = ui->eventListWidget->item(row);
        auto widget = ui->eventListWidget->itemWidget(rowItem);
        if (!widget)
            continue;
        auto eventWidget = static_cast<EventWidget*>(widget);
        if (eventWidget->isEnabled() && eventWidget->title() == key)
        {
            QString filterText = eventWidget->body();
            QStringList msgs = getEditConditionStringList(filterText, danmaku);
            return getExecutionResult(msgs, danmaku);
        }
    }
    return "";
}

/**
 * 多个可执行序列
 * 选其中一个序列来执行与回复
 */
QString CodeRunner::getExecutionResult(QStringList& msgs, const LiveDanmaku &_danmaku)
{
    if (!msgs.size())
        return "";

    // 随机获取其中的一条
    int r = qrand() % msgs.size();
    QStringList dms = msgs.at(r).split("\\n");

    // 尝试执行
    CmdResponse res = NullRes;
    int resVal = 0;
    LiveDanmaku& danmaku = const_cast<LiveDanmaku&>(_danmaku);
    for (int i = 0; i < dms.size(); i++)
    {
        const QString& dm = dms.at(i);
        if (execFuncCallback(dm, danmaku, res, resVal))
        {
            if (res == AbortRes)
                return "";
            else if (res == DelayRes)
                ;
            dms.removeAt(i--);
        }
    }

    // 返回的弹幕内容
    return toSingleLine(dms.join("\\n"));
}

void CodeRunner::simulateKeys(QString seq, bool press, bool release)
{
    if (seq.isEmpty())
        return ;

    // 模拟点击右键
    // mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
    // mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
    // keybd_event(VK_CONTROL, (BYTE) 0, 0, 0);
    // keybd_event('P', (BYTE)0, 0, 0);
    // keybd_event('P', (BYTE)0, KEYEVENTF_KEYUP, 0);
    // keybd_event(VK_CONTROL, (BYTE)0, KEYEVENTF_KEYUP, 0);
#if defined(Q_OS_WIN)
    // 字符串转KEY
    QList<int>keySeq;
    QStringList keyStrs = seq.toLower().split("+", QString::SkipEmptyParts);

    // 先判断修饰键
    /*if (keyStrs.contains("ctrl"))
        keySeq.append(VK_CONTROL);
    if (keyStrs.contains("shift"))
        keySeq.append(VK_SHIFT);
    if (keyStrs.contains("alt"))
        keySeq.append(VK_MENU);
    keyStrs.removeOne("ctrl");
    keyStrs.removeOne("shift");
    keyStrs.removeOne("alt");*/

    // 其他键
    for (int i = 0; i < keyStrs.size(); i++)
    {
        QString ch = keyStrs.at(i);
        if (ch == "ctrl" || ch == "control")
            keySeq.append(VK_CONTROL);
        else if (ch == "shift")
            keySeq.append(VK_SHIFT);
        else if (ch == "alt")
            keySeq.append(VK_MENU);
        else if (ch == "tab")
            keySeq.append(VK_TAB);
        else if (ch.startsWith("0x"))
        {
            ch = ch.right(ch.length() - 2);
            int val = ch.toInt(nullptr, 16);
            keySeq.append(val);
        }
        /* else if (ch >= "0" && ch <= "9")
            keySeq.append(0x30 + ch.toInt());
        else if (ch >= "a" && ch <= "z")
            keySeq.append(0x41 + ch.at(0).toLatin1() - 'a'); */
        else
        {
            char c;
            // 特判名字
            if (ch == "add")
                c = '+';
            else if (ch == "space")
                c = ' ';
            else
                c = ch.at(0).toLatin1();

            DWORD sc = OemKeyScan(c);
            // DWORD shift = sc >> 16; // 判断有没有按下shift键（这里当做没有）
            unsigned char vkey = MapVirtualKey(sc & 0xffff, 1);
            keySeq.append(vkey);
        }
    }

    // 模拟按下（全部）
    if (press)
    {
        for (int i = 0; i < keySeq.size(); i++)
            keybd_event(keySeq.at(i), (BYTE) 0, 0, 0);
    }

    // 模拟松开（全部）
    if (release)
    {
        for (int i = 0; i < keySeq.size(); i++)
            keybd_event(keySeq.at(i), (BYTE) 0, KEYEVENTF_KEYUP, 0);
    }
#endif
}

/// 模拟鼠标点击（当前位置）
/// 参考资料：https://blog.csdn.net/qq_34106574/article/details/89639503
void CodeRunner::simulateClick()
{
    // mouse_event(dwFlags, dx, dy, dwData, dwExtraInfo)
#ifdef Q_OS_WIN
    mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
#else
    qWarning() << "不支持模拟鼠标点击";
#endif
}

void CodeRunner::simulateClickButton(qint64 keys)
{
#ifdef Q_OS_WIN
    mouse_event((DWORD)keys, 0, 0, 0, 0);
#else
    qWarning() << "不支持模拟鼠标点击";
#endif
}

/// 移动鼠标位置
void CodeRunner::moveMouse(unsigned long dx, unsigned long dy)
{
#ifdef Q_OS_WIN
    mouse_event(MOUSEEVENTF_MOVE, dx, dy, 0, 0);
#else
    qWarning() << "不支持模拟鼠标点击";
#endif
}

/// 移动鼠标位置
void CodeRunner::moveMouseTo(unsigned long tx, unsigned long ty)
{
#ifdef Q_OS_WIN
    // QPoint pos = QCursor::pos();
    // qInfo() << "鼠标移动距离：" << QPoint(tx, ty) << pos << QPoint(tx - pos.x(), ty - pos.y());
    // mouse_event(MOUSEEVENTF_MOVE, tx - pos.x(), ty - pos.y(), 0, 0); // 这个计算出来的位置不对啊！
    SetCursorPos(tx, ty); // 这个必须要在本程序窗口的区域内才有效
#else
    qWarning() << "不支持模拟鼠标点击";
#endif
}

void CodeRunner::addBannedWord(QString word, QString anchor)
{
    if (word.isEmpty())
        return ;

    QString text = ui->autoBlockNewbieKeysEdit->toPlainText();
    if (anchor.isEmpty())
    {
        if (anchor.endsWith("|") || word.startsWith("|"))
            text += word;
        else
            text += "|" + word;
    }
    else
    {
        if (!anchor.startsWith("|"))
            anchor = "|" + anchor;
        text.replace(anchor, "|" + word + anchor);
    }
    ui->autoBlockNewbieKeysEdit->setPlainText(text);

    text = ui->promptBlockNewbieKeysEdit->toPlainText();

    if (anchor.isEmpty())
    {
        if (anchor.endsWith("|") || word.startsWith("|"))
            text += word;
        else
            text += "|" + word;
    }
    else
    {
        if (!anchor.startsWith("|"))
            anchor = "|" + anchor;
        text.replace(anchor, "|" + word + anchor);
    }
    ui->promptBlockNewbieKeysEdit->setPlainText(text);
}

void CodeRunner::speekVariantText(QString text)
{
    if (!shallSpeakText())
        return ;

    // 开始播放
    emit signalSpeakText(text);
}

bool CodeRunner::hasSimilarOldDanmaku(const QString &s) const
{
    int count = us->danmuSimilarJudgeCount;
    for (int i = rt->allDanmakus.size() - 2; i >= 0; i--)
    {
        const LiveDanmaku& danmaku = rt->allDanmakus.at(i);
        if (!danmaku.is(MessageType::MSG_DANMAKU))
            continue;
        if (!us->useStringSimilar)
        {
            if (danmaku.getText() == s)
                return true;
        }
        else
        {
            if (StringDistanceUtil::getSimilarity(danmaku.getText(), s) >= us->stringSimilarThreshold)
                return true;
        }
        if (--count <= 0)
            break;
    }
    return false;
}

void CodeRunner::triggerCmdEvent(const QString &cmd, const LiveDanmaku &danmaku, bool debug)
{
    emit signalTriggerCmdEvent(cmd, danmaku, debug);
}

void CodeRunner::localNotify(const QString &text, UIDT uid)
{
    emit signalLocalNotify(text, uid);
}

void CodeRunner::showError(const QString &title, const QString &desc)
{
    emit signalShowError(title, desc);
}

QString CodeRunner::msgToShort(QString msg) const
{
    if (msg.startsWith(">") || msg.length() <= ac->danmuLongest)
        return msg;
    if (msg.contains(" "))
    {
        msg = msg.replace(" ", "");
        if (msg.length() <= ac->danmuLongest)
            return msg;
    }
    if (msg.contains("“"))
    {
        msg = msg.replace("“", "");
        msg = msg.replace("”", "");
        if (msg.length() <= ac->danmuLongest)
            return msg;
    }
    return msg;
}

/**
 * 用户设定的文件名
 * 获取相应的文件绝对路径
 */
QString CodeRunner::toFilePath(const QString &fileName) const
{
    if (QFileInfo(fileName).isRelative())
        return rt->dataPath + fileName;
    return fileName;
}

/**
 * 从不安全的输入方式读取到的文本，如读取txt
 * 转换为代码中可以解析的安全的文本
 */
QString CodeRunner::toSingleLine(QString text) const
{
    return text.replace("\n", "%n%").replace("\\n", "%m%").replace("\"", "\\\"");
}

/**
 * 从代码中解析到的文本，变为可以保存的文本
 */
QString CodeRunner::toMultiLine(QString text) const
{
    return text.replace("%n%", "\n").replace("%m%", "\\n").replace("\\\"", "\"");
}
/**
 * postJson中  数据来源不管怎么回事 但是json中的转义 不能直接把%n%转义为\n 得转义为\\n
 */
QString CodeRunner::toMultiLineForJson(QString text) const
{
    return text.replace("%n%", "\\n").replace("%m%", "\\n").replace("\\\"", "\"");
}

QString CodeRunner::toRunableCode(QString text) const
{
    return text.replace("\\%", "%");
}

UIDT CodeRunner::unameToUid(QString text)
{
    // 查找弹幕和送礼
    for (int i = liveService->roomDanmakus.size()-1; i >= 0; i--)
    {
        const LiveDanmaku danmaku = liveService->roomDanmakus.at(i);
        if (!danmaku.is(MSG_DANMAKU) && !danmaku.is(MSG_GIFT))
            continue;

        QString nick = danmaku.getNickname();
        if (nick.contains(text))
        {
            // 就是这个人
            triggerCmdEvent("FIND_USER_BY_UNAME", danmaku, true);
            return danmaku.getUid();
        }
    }

    // 查找专属昵称
    QSet<UIDT> hadMatches;
    for (int i = liveService->roomDanmakus.size()-1; i >= 0; i--)
    {
        const LiveDanmaku danmaku = liveService->roomDanmakus.at(i);
        if (!danmaku.is(MSG_DANMAKU) && !danmaku.is(MSG_GIFT))
            continue;
        UIDT uid = danmaku.getUid();
        if (hadMatches.contains(uid) || !us->localNicknames.contains(uid))
            continue;
        QString nick = us->localNicknames.value(uid);
        if (nick.contains(text))
        {
            // 就是这个人
            triggerCmdEvent("FIND_USER_BY_UNAME", danmaku, true);
            return danmaku.getUid();
        }
        hadMatches.insert(uid);
    }

    localNotify("[未找到用户：" + text + "]");
    triggerCmdEvent("NOT_FIND_USER_BY_UNAME", LiveDanmaku(text), true);
    return "";
}

QString CodeRunner::uidToName(UIDT uid)
{
    // 查找弹幕和送礼
    for (int i = liveService->roomDanmakus.size()-1; i >= 0; i--)
    {
        const LiveDanmaku danmaku = liveService->roomDanmakus.at(i);
        if (!danmaku.is(MSG_DANMAKU) && !danmaku.is(MSG_GIFT))
            continue;

        if (danmaku.getUid() == uid)
        {
            // 就是这个人
            triggerCmdEvent("FIND_USER_BY_UID", danmaku, true);
            return danmaku.getNickname();
        }
    }

    // 查找专属昵称
    QSet<UIDT> hadMatches;
    for (int i = liveService->roomDanmakus.size()-1; i >= 0; i--)
    {
        const LiveDanmaku danmaku = liveService->roomDanmakus.at(i);
        if (!danmaku.is(MSG_DANMAKU) && !danmaku.is(MSG_GIFT))
            continue;
        if (danmaku.getUid() == uid)
        {
            // 就是这个人
            triggerCmdEvent("FIND_USER_BY_UID", danmaku, true);
            return danmaku.getNickname();
        }
        hadMatches.insert(uid);
    }

    localNotify("[未找到用户：" + uid + "]");
    triggerCmdEvent("NOT_FIND_USER_BY_UID", LiveDanmaku(uid), true);
    return uid;
}

/**
 * 一个智能的用户昵称转简单称呼
 */
QString CodeRunner::nicknameSimplify(const LiveDanmaku &danmaku) const
{
    QString name = danmaku.getNickname();
    QString simp = name;

    // 没有取名字的，就不需要欢迎了
    /*QRegularExpression defaultRe("^([bB]ili_\\d+|\\d+_[bB]ili)$");
    if (simp.indexOf(defaultRe) > -1)
    {
        return "";
    }*/

    // 去掉前缀后缀
    QStringList special{"~", "丶", "°", "゛", "-", "_", "ヽ", "丿", "'"};
    QStringList starts{"我叫", "叫我", "我是", "我就是", "可是", "一只", "一个", "是个", "是", "原来", "但是", "但",
                       "在下", "做", "隔壁", "的", "寻找", "为什么"};
    QStringList ends{"啊", "呢", "呀", "哦", "呐", "巨凶", "吧", "呦", "诶", "哦", "噢", "吖", "Official"};
    starts += special;
    ends += special;
    for (int i = 0; i < starts.size(); i++)
    {
        QString start = starts.at(i);
        if (simp.startsWith(start))
        {
            simp = simp.remove(0, start.length());
            i = 0; // 从头开始
        }
    }
    for (int i = 0; i < ends.size(); i++)
    {
        QString end = ends.at(i);
        if (simp.endsWith(end))
        {
            simp = simp.remove(simp.length() - end.length(), end.length());
            i = 0; // 从头开始
        }
    }

    // 默认名字
    QRegularExpression defRe("(\\d+)_[Bb]ili");
    QRegularExpressionMatch match;
    if (simp.indexOf(defRe, 0, &match) > -1)
    {
        simp = match.capturedTexts().at(1);
    }

    // 去掉首尾数字
    QRegularExpression snumRe("^\\d+(\\D+)\\d*$");
    if (simp.indexOf(snumRe, 0, &match) > -1
            && match.captured(1).indexOf(QRegExp("^[的是]")) == -1)
    {
        simp = match.capturedTexts().at(1);
    }
    snumRe = QRegularExpression("^(\\D+)\\d+$");
    if (simp.indexOf(snumRe, 0, &match) > -1
            && match.captured(1) != "bili_"
            && match.captured(1).indexOf(QRegExp("^[的是]")) == -1)
    {
        simp = match.capturedTexts().at(1);
    }

    // 一大串 中文enen
    // 注：日语正则 [\u0800-\u4e00] ，实测“一”也会算在里面……？
    QRegularExpression ceRe("([\u4e00-\u9fa5]{2,})([-_\\w\\d_\u0800-\u4dff]+)$");
    if (simp.indexOf(ceRe, 0, &match) > -1 && match.capturedTexts().at(1).length()*3 >= match.capturedTexts().at(2).length())
    {
        QString tmp = match.capturedTexts().at(1);
        if (!QString("的之の是叫有为奶在去着最很").contains(tmp.right(1)))
        {
            simp = tmp;
        }
    }
    // enen中文
    ceRe = QRegularExpression("^([-\\w\\d_\u0800-\u4dff]+)([\u4e00-\u9fa5]{2,})$");
    if (simp.indexOf(ceRe, 0, &match) > -1 && match.capturedTexts().at(1).length() <= match.capturedTexts().at(2).length()*3)
    {
        QString tmp = match.capturedTexts().at(2);
        if (!QString("的之の是叫有为奶在去着最").contains(tmp.right(1)))
        {
            simp = tmp;
        }
    }

    // xxx的xxx
    QRegularExpression deRe("^(.{2,})[的の]([\\w\\d_\\-\u4e00-\u9fa5]{2,})$");
    if (simp.indexOf(deRe, 0, &match) > -1 && match.capturedTexts().at(1).length() <= match.capturedTexts().at(2).length()*2)
    {
        QRegularExpression blankL("(我$)"), blankR("(名字|^确|最)");
        if (match.captured(1).indexOf(blankL) == -1
                && match.captured(2).indexOf(blankR) == -1) // 不包含黑名单
            simp = match.capturedTexts().at(2);
    }

    // 固定格式1
    QStringList extraExp{"^(.{2,}?)[-_]\\w{2,}$",
                        "^这个(.+)不太.+$", "^(.{3,})今天.+$", "最.+的(.{2,})$",
                         "^.*还.+就(.{2})$",
                         "^(.{2,})(.)不\\2.*",
                         "我不是(.{2,})$",
                         "^(.{2,}?)(永不|有点|才是|敲|很|能有|都|想|要|需|真的|从不|才不|不|跟你|和你|这\
|超好|是我|来自|是只|今天|我是).+",
                         "^(.{2,})-(.{2,})$",
                         "^(.{2,}?)[最很超特别是没不想好可以能要]*有.+$",
                         "^.*?(?:是个|看我)(.{2,})$"
                         "^(不做|只因)(.{2,})$",
                        "^x+(.{2,}x+)$",
                        "^(.{2,})(.+)\\1$",
                        "^不要(.{2,})"};
    for (int i = 0; i < extraExp.size(); i++)
    {
        QRegularExpression re(extraExp.at(i));
        if (simp.indexOf(re, 0, &match) > -1)
        {
            QString partName = match.capturedTexts().at(1);
            if (partName.contains(QRegularExpression("我|你|很|想|是|不")))
                continue;
            simp = partName;
            break;
        }
    }

    // 特殊字符
    simp = simp.replace(QRegularExpression("_|丨|丶|灬|ミ|丷|I"), "");

    // 身份后缀：xxx哥哥
    QRegularExpression gegeRe("^(.{2,}?)(大|小|老)?(公|婆|鸽鸽|哥哥|爸爸|爷爷|奶奶|妈妈|朋友|盆友|魔王|可爱|参上|大大|大人)$");
    if (simp.indexOf(gegeRe, 0, &match) > -1)
    {
        QString tmp = match.capturedTexts().at(1);
        simp = tmp;
    }

    // 重复词：AAAA
    QRegularExpression aaRe("(.)\\1{2,}");
    if (simp.indexOf(aaRe, 0, &match) > -1)
    {
        QString ch = match.capturedTexts().at(1);
        QString all = match.capturedTexts().at(0);
        simp = simp.replace(all, QString("%1%1").arg(ch));
    }

    // ABAB
    QRegularExpression abRe = QRegularExpression("(.{2,})\\1{1,}");
    if (simp.indexOf(abRe, 0, &match) > -1)
    {
        QString ch = match.capturedTexts().at(1);
        QString all = match.capturedTexts().at(0);
        simp = simp.replace(all, QString("%1").arg(ch));
    }

    // Name1Name2
    QRegularExpression sunameRe = QRegularExpression("^[A-Z]*?([A-Z][a-z0-9]+)[-_A-Z0-9]([\\w_\\-])*$");
    if (simp.indexOf(sunameRe, 0, &match) > -1)
    {
        QString ch = match.capturedTexts().at(1);
        QString all = match.capturedTexts().at(0);
        simp = simp.replace(all, QString("%1").arg(ch));
    }

    // 一长串数字
    QRegularExpression numRe("(\\d{3})\\d{3,}");
    if (simp.indexOf(numRe, 0, &match) > -1)
    {
        simp = simp.replace(match.captured(0), match.captured(1) + "…");
    }

    // 一长串英文
    QRegularExpression wRe("(\\w{5})\\w{3,}");
    if (simp.indexOf(wRe, 0, &match) > -1)
    {
        simp = simp.replace(match.captured(0), match.captured(1) + "…");
    }

    // 固定搭配替换
    QHash<QString, QString>repl;
    repl.insert("奈子", "奈X");
    repl.insert("走召", "超");
    repl.insert("リ巾", "帅");
    repl.insert("白勺", "的");
    for (auto it = repl.begin(); it != repl.end(); it++)
    {
        simp = simp.replace(it.key(), it.value());
    }

    if (simp.isEmpty())
        return name;
    return simp;
}

QString CodeRunner::numberSimplify(int number) const
{
    if (number < 10000)
        return QString::number(number);
    number = (number + 5000) / 10000;
    return QString::number(number) + "万";
}

bool CodeRunner::shallAutoMsg() const
{
    return !ui->sendAutoOnlyLiveCheck->isChecked() || (liveService->isLiving() /*&& popularVal > 1*/);
}

bool CodeRunner::shallAutoMsg(const QString &sl) const
{
    if (sl.contains("%living%"))
        return true;
    return shallAutoMsg();
}

bool CodeRunner::shallAutoMsg(const QString &sl, bool &manual)
{
    if (sl.contains("%living%"))
    {
        manual = true;
        return true;
    }
    return shallAutoMsg();
}

bool CodeRunner::shallSpeakText()
{
    bool rst = !ui->dontSpeakOnPlayingSongCheck->isChecked() || !musicWindow || !musicWindow->isPlaying();
    if (!rst && us->debugPrint)
        localNotify("[放歌中，跳过语音]");
    return rst;
}

bool CodeRunner::isWorking() const
{
    if (us->localMode)
        return false;
    return (shallAutoMsg() && (ui->autoSendWelcomeCheck->isChecked() || ui->autoSendGiftCheck->isChecked() || ui->autoSendAttentionCheck->isChecked()));
}
