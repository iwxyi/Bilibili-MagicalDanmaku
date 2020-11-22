#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkCookie>
#include <QStringListModel>
#include <QTimer>
#include <QScrollBar>
#include "videolyricscreator.h"
#include "ui_videolyricscreator.h"

VideoLyricsCreator::VideoLyricsCreator(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::VideoLyricsCreator),
    settings("settings.ini", QSettings::Format::IniFormat)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);

    userCookie = settings.value("lyrics/userCookie", "").toString();
    lyricsSample = settings.value("lyrics/danmakuSample", "").toString();

    ui->lyricsEdit->setPlaceholderText("[00:23.13]飘泊的雪 摇曳回风\n[00:26.77]诗意灵魂 更迭情人"
                                          "\n[00:30.40]总惯用轻浮的茂盛 掩抹深沉\n[00:36.98]......");

    sendTimer = new QTimer(this);
    sendTimer->setInterval(5000);
    connect(sendTimer, SIGNAL(timeout()), this, SLOT(sendNextLyric()));
}

VideoLyricsCreator::~VideoLyricsCreator()
{
    delete ui;
}

void VideoLyricsCreator::on_sendButton_clicked()
{
    if (sending)
    {
        setSendingState(false);
        return ;
    }

    lyricList = ui->lyricsEdit->toPlainText().split("\n", QString::SkipEmptyParts);

    // 判断歌词格式
    QRegularExpression re("^\\[\\d+[:\\.]\\d+\\.\\d+\\].*$");
    foreach (QString s, lyricList)
    {
        if (s.indexOf(re) == -1)
        {
            QMessageBox::information(this, "检查格式", "格式错误，当前行：\n" + s);
            return ;
        }
    }

    if (userCookie.isEmpty() || lyricsSample.isEmpty())
    {
        QMessageBox::information(this, "账号", "请在菜单中设置账号数据");
        return ;
    }

    if (videoId.isEmpty() || videoAVorBV.isEmpty() || videoCid.isEmpty())
    {
        QMessageBox::information(this, "数据", "未设置全部数据:"+videoAVorBV+" "+videoId+"\n"+videoCid);
        return ;
    }

    // 初始化数据
    failedList.clear();
    totalSending = 0;
    offsetMSecond = ui->offsetSpin->value();
    ui->lyricsEdit->verticalScrollBar()->setSliderPosition(0);
    sendSample = lyricsSample;
    qDebug() << "原始：" << sendSample;
    sendSample.replace(QRegularExpression("(^|&)(aid|bvid)=.*?(&|$)"),
                       "\\1"+videoAVorBV+"="+videoId+"\\3");
    sendSample.replace(QRegularExpression("((?:^|&)oid=).*?(&|$)"),
                       "\\1"+videoCid+"\\2");
    qDebug() << "新的：" << sendSample;

    // 开始挨个发送
    setSendingState(true);
}

/**
 * 格式说明：https://github.com/SocialSisterYi/bilibili-API-collect/blob/master/danmaku/action.md
 */
void VideoLyricsCreator::on_actionSet_Lyrics_Danmaku_Sample_triggered()
{
    // 示例：
    // type=1&oid=237625969&msg=%E5%96%B5%E5%96%B5%E5%96%B5
    // &bvid=BV1Mk4y11732&progress=31505&color=16777215&fontsize=25&pool=1&mode=4
    // &rnd=1606031544129302&plat=1&csrf=2570b86da5d84cac811d9645edb94956
    bool ok = false;
    QString s = QInputDialog::getText(this, "设置字幕样例", "先前发送弹幕", QLineEdit::Normal, lyricsSample, &ok);
    if (!ok)
        return ;

    settings.setValue("lyrics/danmakuSample", lyricsSample = s);
}

void VideoLyricsCreator::on_actionSet_Sample_Help_triggered()
{
    QString steps = "发送弹幕前需按以下步骤注入登录信息：\n\n";
    steps += "步骤一：\n浏览器登录bilibili账号，并进入视频页面\n\n";
    steps += "步骤二：\n按下F12（开发者调试工具），找到右边顶部的“Network”项\n\n";
    steps += "步骤三：\n浏览器上发送字幕，Network中多出一条“post”，点它，看右边“Headers”中的代码\n\n";
    steps += "步骤四：\n复制“Request Headers”下的“cookie”冒号后的一长串内容，粘贴到本程序“设置Cookie”中\n\n";
    steps += "步骤五：\n点击“Form Data”右边的“view source”，复制它下面一串内容到本程序“设置字幕样例”中\n\n";
    steps += "设置好网址、要发送的歌词，即可设置字幕！";
    QMessageBox::information(this, "账号设置", steps);
}

void VideoLyricsCreator::on_actionSet_Cookie_triggered()
{
    bool ok = false;
    QString s = QInputDialog::getText(this, "设置Cookie", "设置用户登录的cookie", QLineEdit::Normal, userCookie, &ok);
    if (!ok)
        return ;

    settings.setValue("lyrics/userCookie", userCookie = s);
}

// https://github.com/SocialSisterYi/bilibili-API-collect/blob/master/video/info.md
void VideoLyricsCreator::getVideoInfo(QString key, QString id)
{
    QString url = "http://api.bilibili.com/x/web-interface/view?"+key+"=" + id;
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString() << data;
            return ;
        }
        QJsonObject json = document.object();
        if (json.value("code").toInt() != 0)
        {
            qDebug() << ("返回结果不为0：") << json.value("message").toString();
            return ;
        }

        videoPages = json.value("data").toObject().value("pages").toArray();
        QStringList sl;
        foreach (QJsonValue val, videoPages)
        {
            QJsonObject page = val.toObject();
            sl << page.value("part").toString();
        }
        ui->listView->setModel(new QStringListModel(sl));

        videoCid = QString::number(static_cast<qint64>(videoPages.at(pageIndex).toObject().value("cid").toDouble()));
        ui->cidLabel->setText(videoPages.at(pageIndex).toObject().value("part").toString());
        qDebug() << pageIndex << "cid" << videoCid;
    });
    manager->get(*request);
}

void VideoLyricsCreator::sendNextLyric()
{
    // 全部发送完了
    if (!lyricList.size())
    {
        setSendingState(false);
        if (failedList.size())
        {
            QMessageBox::information(this, "发送结果", QString("发送成功：%1\n发送失败：%2\n\n失败项已放入到歌词编辑框中")
                                     .arg(totalSending).arg(failedList.size()));
            ui->lyricsEdit->setPlainText(failedList.join("\n"));
        }
        else
        {
            QMessageBox::information(this, "发送结果", QString("%1 行歌词已全部发送完毕")
                                     .arg(totalSending));
        }
        sendTimer->stop();
        return ;
    }

    QString s = lyricList.takeFirst();
    QRegularExpression re("^\\[(\\d{1,3})[:\\.](\\d{1,2})\\.(\\d{1,2})\\](.*)$");
    QRegularExpressionMatch match;
    if (s.indexOf(re, 0, &match) == -1)
    {
        failedList.append(s);
        qDebug() << "发送失败，无法解析格式：" << s;
        return ;
    }

    int minute = match.captured(1).toInt();
    int second= match.captured(2).toInt();
    int msecond = match.captured(3).toInt();
    QString lyricStr = match.captured(4);

    int time = minute * 60000 + second * 1000 + msecond;
    sendLyrics(time + offsetMSecond, lyricStr);
    ui->lyricsEdit->setPlainText(lyricList.join("\n"));
}

void VideoLyricsCreator::sendLyrics(qint64 time, QString text)
{
    qDebug() << ">>>>>>>>>>发送歌词：" << time << text;
    text = text.trimmed();
    if (text.isEmpty()) // 空歌词不需要发送
        return ;

    QUrl url("http://api.bilibili.com/x/v2/dm/post");

    // 建立对象
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::CookieHeader, getCookies());
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");

    // 设置数据
    QString datas = sendSample;
    datas.replace(QRegularExpression("((?:^|&)msg=).*?(&|$)"), "\\1"+text.toUtf8().toPercentEncoding()+"\\2");
    datas.replace(QRegularExpression("((?:^|&)progress=).*?(&|$)"), "\\1"+QString::number(time)+"\\2");
    QByteArray ba(datas.toStdString().data());

    // 连接槽
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray data = reply->readAll();
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject object = document.object();
        if (object.value("code").toInt() != 0)
        {
            qDebug() << ("warning: 发送失败：") << object.value("message").toString() << datas;
            return ;
        }

        // 进度
        totalSending++;
        ui->lyricsLabel->setText(QString("已发送：%1/%2").arg(totalSending)
                                .arg(totalSending + lyricList.size()+ failedList.size()));
    });

    manager->post(*request, ba);
}

void VideoLyricsCreator::setSendingState(bool state)
{
    ui->urlEdit->setEnabled(!state);
    ui->lyricsEdit->setEnabled(!state);
    ui->offsetSpin->setEnabled(!state);
    ui->listView->setEnabled(!state);

    sending = state;
    if (state)
    {
        sendTimer->start();
        ui->sendButton->setText("终止发送");
    }
    else
    {
        sendTimer->stop();
        ui->sendButton->setText("开始发送");
        ui->lyricsLabel->setText("歌词字幕");
    }
}

QVariant VideoLyricsCreator::getCookies()
{
    QList<QNetworkCookie> cookies;

    // 设置cookie
    QString cookieText = userCookie;
    QStringList sl = cookieText.split(";");
    foreach (auto s, sl)
    {
        s = s.trimmed();
        int pos = s.indexOf("=");
        QString key = s.left(pos);
        QString val = s.right(s.length() - pos - 1);
        cookies.push_back(QNetworkCookie(key.toUtf8(), val.toUtf8()));
    }

    // 请求头里面加入cookies
    QVariant var;
    var.setValue(cookies);
    return var;
}

void VideoLyricsCreator::on_urlEdit_editingFinished()
{
    QString url = ui->urlEdit->text();
    if (!url.isEmpty())
    {
        int pos = url.lastIndexOf("/");
        if (pos == 0)
            videoId = url;
        else
            videoId = url.right(url.length() - pos - 1);
        videoAVorBV = videoId.toLower().startsWith("bv") ? "bvid" : "aid"; // 是AV还是BV
        pageIndex = 0;

        QRegularExpression re("^(.+)\\?p=(\\d+)$");
        QRegularExpressionMatch match;
        if (videoId.indexOf(re, 0, &match) > -1)
        {
            videoId = match.captured(1);
            pageIndex = match.captured(2).toInt() - 1;
            qDebug() << "当前页：" << pageIndex;
        }

        qDebug() << "视频：" << videoAVorBV << videoId;
        getVideoInfo(videoAVorBV, videoId);
    }
}

void VideoLyricsCreator::on_listView_clicked(const QModelIndex &index)
{
    pageIndex = index.row();
    videoCid = QString::number(static_cast<qint64>(videoPages.at(pageIndex).toObject().value("cid").toDouble()));
    qDebug() << pageIndex << "cid" << videoCid;
    ui->cidLabel->setText(videoPages.at(pageIndex).toObject().value("part").toString());
}
