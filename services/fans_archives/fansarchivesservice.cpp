#include "fansarchivesservice.h"
#include "myjson.h"
#include "chatgptutil.h"

#define INTERVAL_WORK 3000
#define INTERVAL_WAIT 60000

FansArchivesService::FansArchivesService(SqlService* sqlService, QObject* parent)
    : QObject(parent), sqlService(sqlService)
{
    timer = new QTimer(this);
    timer->setInterval(INTERVAL_WORK + 3000);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &FansArchivesService::onTimer);
}

void FansArchivesService::start()
{
    timer->start();
}

void FansArchivesService::onTimer()
{
    if (!sqlService || !sqlService->isOpen())
    {
        qWarning() << "数据库未打开，无法处理粉丝档案";
        emit signalError("数据库未打开，无法处理粉丝档案");
        return;
    }
    
    QString uid = sqlService->getNextFansArchive();
    if (uid.isEmpty())
    {
        if (timer->interval() != INTERVAL_WAIT)
            qInfo() << "没有找到需要处理的粉丝档案，暂缓工作";
        timer->setInterval(INTERVAL_WAIT); // 60秒判断一次
        timer->start();
        return;
    }
    if (uid == "0")
    {
        qWarning() << "使用了'0'作为uid的粉丝档案";
        return;
    }
    qInfo() << "准备处理粉丝档案：" << uid;

    // 获取现有粉丝档案
    MyJson archive = sqlService->getFansArchives(uid);
    QString currentArchive;
    if (!archive.isEmpty())
        currentArchive = archive.s("archive");
    qint64 startTime = 0;
    if (!currentArchive.isEmpty())
        startTime = archive.l("update_time");

    // 获取未处理的弹幕
    QList<MyJson> danmakuList = sqlService->getUserDanmakuList(uid, startTime, 100);
    QString lastUname;
    if (danmakuList.isEmpty())
    {
        qWarning() << "没有找到未处理的弹幕";
        return;
    }
    QStringList danmakuListStr;
    foreach (MyJson danmaku, danmakuList)
    {
        if (lastUname.isEmpty())
            lastUname = danmaku.s("uname");
        danmakuListStr.append(danmaku.s("create_time") + "  " + danmaku.s("uname") + "：" + danmaku.s("msg"));
    }
    qInfo() << "待处理的用户信息：" << lastUname << "弹幕数量：" << danmakuList.size();

    // 组合发送的文本
    QString prompt = R"(
你是一个专业的观众心理分析师，请根据用户所有历史弹幕内容，按以下维度生成中文字段的纯文本档案（请勿使用Markdown等）：
# 分析纬度（无法分析的纬度请忽略）：

【整体描述】
* 对于下面所有信息的总结概括，一目了然

【基础画像】
* 用户类型：活跃粉丝/潜水观众/节奏带动者等
* 活跃时段：工作日20-22点/周末全天（根据发言时间分布）
* 互动热度：0-100分（根据发言频率和互动反馈计算）
* 内容倾向：知识获取型/情感陪伴型/娱乐消遣型

【性格特征】
1. 核心性格：形容词+证据（如"社牛型（使用58个emoji/平均每句1.2个感叹号）"）  
2. 次要特质：带具体行为的关键词（如"细节控（常追问产品参数）"）  

【兴趣图谱】
- 近期高频兴趣：按时间衰减权重排序（如"3C数码→机械键盘（近7天讨论6次）"）  
- 潜在兴趣：根据关联话题推测（如"讨论键鼠时多次提及二次元角色→可能喜欢IP联名款"）  

【偏好触发点】
> 当主播做以下行为时，该用户互动率提升：
① 使用"卧槽"等标志性口头禅（触发概率+80%）
② 展示限定皮肤（停留时长提升3倍）

【MBTI性格推测】  
1. 主导倾向：  
- 外向(E)/内向(I)：根据主动发言频率、感叹号使用量判断  
  例："偏外向型（日均发言15条，87%含emoji）"  
- 直觉(N)/实感(S)：通过抽象词汇（如"可能性"）与具体描述（如"参数"）比例分析  

2. 辅助功能：  
- 思考(T)/情感(F)：观察理性建议与情感共鸣类发言占比  
  例："情感型（常发'主播注意嗓子'等关怀言论）"  
- 判断(J)/知觉(P)：根据发言规律性（固定时间签到）与即兴反应（突发玩梗）判断  

【职业路径推测】  
> 显性线索：  
- 直接声明："我是程序员/教师"（需脱敏处理为"IT从业者"）  
> 隐性线索：  
- 知识领域：频繁讨论法律条款→可能从事司法相关  
- 时间特征：凌晨活跃+讨论跨境时差→可能涉及外贸行业  
- 专业术语：使用"ROI""DAU"等→互联网运营相关  

【个人成长轨迹】  
1. 技能进化：  
- 学习曲线：从"怎么注册账号"到主动指导新观众  
- 兴趣迁移：二次元→硬核科技（跨度分析）  

2. 心智模式：  
- 挫折反应：遇主播失误时鼓励型（"下次会更好"）or 指责型  
- 目标变化：从追求娱乐到寻求知识沉淀（讨论书籍频率+200%）  

【人际关系图谱】  
① 直播间地位：  
- 意见领袖（发言后被引用次数）  
- 社交节点（同时与3+个粉丝群组互动）  

② 互动模式：  
- 主动关怀型（常@新观众解答问题）  
- 竞争意识型（喜欢挑战其他观众观点）  

【特殊关注】
! 矛盾特征标注（如"70%倾向理性消费，但曾为限量款冲动消费3次"）
! 成长预警："近两周从日活10条降至1条，流失风险高
! 记录个人明确说明的关键信息

# 随着不同种类的直播类型，垂直场景扩展：

## 娱乐主播特化版
【娱乐共鸣点】  
- 情感高潮时刻：引发用户刷礼物的内容类型（如"听到周杰伦歌曲时打赏概率+200%")  
- 最佳互动时段：周五20:00-22:00（历史打赏峰值时段）  
- 造梗能力：曾发起'退退退'梗引发3次刷屏  

【社交影响力】  
* 控场指数：根据弹幕跟风比例计算（如"发言后有30%观众模仿接梗"） 

## 带货主播特化版

【购物人格】  
- 决策类型：数据党（87%发言含参数对比）/ 纠结型（平均询问3次同类商品）  
- 价格敏感度：提及"优惠"频率是平均值的2.3倍  
- 待挖掘需求：曾5次讨论手机续航但未下单  

【行为预警】  
! 当出现"再想想"等犹豫词时，有65%概率需要促销刺激  
! 连续询问材质参数时，推荐实验室检测报告最有效

## 游戏主播特化版

【游戏特性】  
- 操作关注点：特别在意连招失误（发送捂脸表情概率+300%）  
- 赛事倾向：MOBA战术分析型（常讨论BP策略）  
- 参与意愿：79%概率会报名周末水友赛  

【情绪燃点】  
> 当出现以下情况时可能退场：  
① 连续3局落地成盒（发言频率下降90%）  
② 解说偏离核心机制（停留时长缩短50%）

## 职场成长类主播特化
【职业发展分析】  
- 职场焦虑点：高频词"35岁危机"/"转行"  
- 技能缺口：多次询问"Python学习路径"  
- 人脉需求：主动寻求"同城产品经理交流"  

【进阶建议】  
> 可推送资源：  
① 本地行业交流会（检测到用户坐标上海+金融从业）  
② 提升课程（与其讨论过的"数据分析"相关）  

## 教育亲子类主播特化
【家庭角色推测】  
- 家长类型：鸡娃型（常问学习方法）/ 佛系型（讨论兴趣培养）  
- 孩子学段：通过"中考""幼小衔接"等关键词反推  

【教育焦虑曲线】  
! 敏感时段：开学前两周咨询量提升300%  
! 决策依赖：常要求"直接给书单"而非方法论  

## 情感交友类主播特化
【亲密关系模式】  
- 依恋类型：焦虑型（频繁确认"这样正常吗"）  
- 社交障碍：检测到"社恐""不敢要微信"等关键词  

【脱单助力点】  
> 优势挖掘：  
① 共情能力强（常安慰其他单身观众）  
② 兴趣标签清晰（明确表示"喜欢徒步旅行"） 
"\n\n
    )";

    QString sendText;
    if (!archive.isEmpty())
    {
        sendText += "以下是当前已有的档案，请根据该档案，结合近期的弹幕内容，更新档案内容并生成新的档案：\n\n" + archive.s("archive");
    }

    sendText += "以下是近期的弹幕内容，请结合弹幕更新档案内容：\n\n```\n" + danmakuListStr.join("\n") + "\n```";

    // AI实例
    ChatGPTUtil* chatgpt = new ChatGPTUtil(this);
    chatgpt->setStream(false);
    connect(chatgpt, &ChatGPTUtil::signalResponseError, this, [=](const QByteArray& ba) {
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(ba, &error);
        if (error.error == QJsonParseError::NoError)
        {
            QJsonObject json = document.object();
            if (json.contains("error") && json.value("error").isObject())
                json = json.value("error").toObject();
            if (json.contains("message"))
            {
                int code = json.value("code").toInt();
                QString type = json.value("type").toString();
                qCritical() << (json.value("message").toString() + "\n\n错误码：" + QString::number(code) + "  " + type);
            }
            else
            {
                qCritical() << QString(ba);
            }
        }
        else
        {
            qCritical() << QString(ba);
        }
    });
    connect(chatgpt, &ChatGPTUtil::finished, this, [=]{
        chatgpt->deleteLater();

        timer->setInterval(INTERVAL_WORK); // 3秒后继续处理下一个粉丝档案
        timer->start();
    });
    connect(chatgpt, &ChatGPTUtil::signalResponseText, this, [=](const QString& text) {
        qInfo() << "AI生成档案：" << text;
        QString formatText = text;
        if (formatText.startsWith("```") && formatText.endsWith("```"))
        {
            int index = formatText.indexOf("\n");
            if (index != -1)
                formatText = formatText.mid(index + 1);
            index = formatText.lastIndexOf("\n");
            if (index != -1)
                formatText = formatText.left(index);
        }
        // 保存档案
        sqlService->insertFansArchive(uid, lastUname, formatText);
    });

    // 生成data
    QList<ChatBean> chats;
    chats.append(ChatBean("system", prompt));
    if (!sendText.isEmpty())
        chats.append(ChatBean("user", sendText));
    chatgpt->getResponse(chats);
}
