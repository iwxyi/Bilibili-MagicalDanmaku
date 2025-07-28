#ifdef ENABLE_WEBENGINE
#include "WebLoginDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QLabel>
#include <QEventLoop>
#include <QTimer>

WebLoginDialog::WebLoginDialog(const QString &platformUrl, const QString &defaultCookie, QWidget *parent)
    : QDialog(parent), webView(new MyWebEngineView(this)), loginButton(new QPushButton(tr("我已登录"), this))
{
    setWindowTitle(tr("平台账号登录"));
    resize(1400, 900);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    tipLabel = new QLabel(tr("请在浏览器弹窗中登录，登录成功后点击我已登录按钮。"));
    mainLayout->addWidget(tipLabel);
    mainLayout->addWidget(webView, 1);
    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(loginButton);
    mainLayout->addLayout(bottomLayout);

    webView->page()->profile()->setHttpUserAgent(
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"
    );
    webView->setUrl(QUrl(platformUrl));
    if (!defaultCookie.isEmpty()) {
        // 可选：设置初始cookie
        // 这里略过，实际可用 QWebEngineProfile::cookieStore()
        // 以及登录过一次之后会保留Cookie，如果手动覆盖了就没了
    }
    connect(loginButton, &QPushButton::clicked, this, &WebLoginDialog::onLoginClicked);

    connect(webView->page()->profile()->cookieStore(), &QWebEngineCookieStore::cookieAdded, this, [=](const QNetworkCookie &c) {
        loadedCookies << QString::fromUtf8(c.toRawForm()); // 每次可能是多个分号分隔的key=val格式的
    });
}

void WebLoginDialog::onLoginClicked()
{
    fetchCookie();
    accept();
}

void WebLoginDialog::fetchCookie()
{
    // 分割与去重
    QStringList cookieList = loadedCookies.join("; ").split(";");
    QSet<QString> keys;
    QStringList newCookieList;
    for (int i = cookieList.size() - 1; i >= 0; i--)
    {
        QString full = cookieList[i].trimmed();
        QString key;
        if (full.contains("="))
            key = full.split("=")[0].trimmed();
        else
            key = full.trimmed();
        // 分离 key 判断是否重复
        if (!keys.contains(key))
        {
            keys.insert(key);
            newCookieList.append(full);
        }
    }
    // 取反，因为之前是逆序加上去的
    std::reverse(newCookieList.begin(), newCookieList.end());
    // 合并
    cookie = newCookieList.join("; ");
}

QString WebLoginDialog::getCookie() const
{
    qDebug() << "登录Cookie：" << cookie;
    return cookie;
}
#endif
