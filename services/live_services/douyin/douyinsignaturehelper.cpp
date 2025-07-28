#include "DouyinSignatureHelper.h"
#include "silentwebenginepage.h"
#include <QDebug>
#include <QCryptographicHash>
#include <QByteArray>
#include <QApplication>

DouyinSignatureHelper::DouyinSignatureHelper(QObject *parent)
    : QObject(parent), timer(new QTimer(this)),
    eventLoop(new QEventLoop(this)), isInitialized(false)
{
#ifdef ENABLE_WEBENGINE
    view = new QWebEngineView;
    view->setPage(new SilentWebEnginePage(this));

    // 页面加载完成后尝试获取签名
    connect(view, &QWebEngineView::loadFinished, this, [=](bool ok){
        if (ok) {
            // 启动定时器轮询 window.byted_acrawler 是否注入
            timer->start(200);
        }
    });
#endif

    // 定时器轮询
    connect(timer, &QTimer::timeout, this, &DouyinSignatureHelper::tryGetSignature);
}

QString DouyinSignatureHelper::getSignature(const QString &roomId, const QString &uniqueId)
{
    QMutexLocker locker(&mutex);

    // 初始化（如果需要）
    initializeIfNeeded();

    // 生成 X-MS-STUB
    pendingStub = getSTUB(roomId, uniqueId);
    pendingRoomId = roomId;
    pendingUniqueId = uniqueId;
    qDebug() << "Generated X-MS-STUB:" << pendingStub;

    // 重置结果和事件循环
    resultSignature.clear();
    eventLoop->quit(); // 确保之前的循环已退出

    // 启动定时器
    timer->start(200);

    // 阻塞等待结果
    eventLoop->exec();

    return resultSignature;
}

void DouyinSignatureHelper::initializeIfNeeded()
{
    if (!isInitialized) {
#ifdef ENABLE_WEBENGINE
        qInfo() << "开始加载抖音首页";
        // 加载抖音主页
        view->load(QUrl("https://live.douyin.com"));

        // 等待页面加载完成
        QEventLoop loadLoop;
        connect(view, &QWebEngineView::loadFinished, &loadLoop, &QEventLoop::quit);
        loadLoop.exec();
#endif

        isInitialized = true;
    }
}

QString DouyinSignatureHelper::getSTUB(const QString &roomId, const QString &uniqueId)
{
    const QString sdkVersion = "1.0.14-beta.0";
    QString stubStr = QString("live_id=1,aid=6383,version_code=180800,webcast_sdk_version=%1,room_id=%2,sub_room_id=,sub_channel_id=,did_rule=3,user_unique_id=%3,device_platform=web,device_type=,ac=,identity=audience")
                          .arg(sdkVersion)
                          .arg(roomId)
                          .arg(uniqueId);

    // MD5 哈希
    QByteArray hash = QCryptographicHash::hash(stubStr.toUtf8(), QCryptographicHash::Md5);
    qInfo() << "hash:" << hash.toHex();
    return hash.toHex();
}

void DouyinSignatureHelper::tryGetSignature()
{
#ifdef ENABLE_WEBENGINE
    // 检查 window.byted_acrawler 是否存在
    QString checkJs = "typeof window.byted_acrawler !== 'undefined'";
    view->page()->runJavaScript(checkJs, [=](const QVariant &result){
        if (result.toBool()) {
            timer->stop();
            if (!pendingUrl.isEmpty()) {
                // 新增：直接用url生成X-Bogus
                QString js = QString(
                    "window.byted_acrawler.frontierSign && window.byted_acrawler.frontierSign('%1')"
                ).arg(pendingUrl.replace("'", "\\'"));
                view->page()->runJavaScript(js, [=](const QVariant &res){
                    if (res.type() == QVariant::Map) {
                        QVariantMap map = res.toMap();
                        resultSignature = map.value("X-Bogus").toString();
                    } else if (res.type() == QVariant::String) {
                        resultSignature = res.toString();
                    }
                    pendingUrl.clear();
                    eventLoop->quit();
                });
            } else {
                // 旧逻辑：用X-MS-STUB生成
                QString js = QString(
                                 "window.byted_acrawler.frontierSign({'X-MS-STUB':'%1'})"
                                 ).arg(pendingStub);
                view->page()->runJavaScript(js, [=](const QVariant &res){
                    if (res.type() == QVariant::Map) {
                        QVariantMap map = res.toMap();
                        resultSignature = map.value("X-Bogus").toString();
                    }
                    eventLoop->quit();
                });
            }
        }
        // else: 继续轮询
    });
#endif
}
