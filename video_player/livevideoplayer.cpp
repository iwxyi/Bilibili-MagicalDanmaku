#include <QMessageBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QStyle>
#include <QtConcurrent/QtConcurrent>
#include "livevideoplayer.h"
#include "ui_livevideoplayer.h"
#include "facilemenu.h"
#include "picturebrowser.h"

LiveVideoPlayer::LiveVideoPlayer(QSettings &settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LiveVideoPlayer),
    settings(settings)
{
    ui->setupUi(this);
    setModal(false);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setMinimumSize(32, 32);

    // 设置模式
    useVideoWidget = settings.value("videoplayer/useVideoWidget", true).toBool();

    player = new QMediaPlayer(this);
    if (useVideoWidget)
    {
        player->setVideoOutput(ui->videoWidget);
        ui->label->hide();
    }
    else
    {
        ui->videoWidget->hide();
        videoSurface = new VideoSurface(this);
        player->setVideoOutput(videoSurface);
        connect(videoSurface, &VideoSurface::frameAvailable, this, [=](QVideoFrame &frame){
            QVideoFrame cloneFrame(frame);
            cloneFrame.map(QAbstractVideoBuffer::ReadOnly);
            videoSize = cloneFrame.size();
            QImage recvImage(cloneFrame.bits(), videoSize.width(), videoSize.height(), QVideoFrame::imageFormatFromPixelFormat(cloneFrame.pixelFormat()));
            cloneFrame.unmap();
            QPixmap pixmap = QPixmap::fromImage(recvImage);
            if (captureRunning)
                slotSaveFrameCapture(pixmap);
            ui->label->setPixmap(pixmap.scaled(ui->label->size(), Qt::KeepAspectRatio, transformation));
            ui->label->setMinimumSize(QSize(1, 1));
        });
    }

    connect(player, &QMediaPlayer::stateChanged, this, [=](QMediaPlayer::State state) {
        if (state == QMediaPlayer::PlayingState)
        {
            ui->playButton->setIcon(QIcon(":/icons/pause"));
            calcVideoRect();
            if (enablePrevCapture)
                startCapture();
        }
        else if (state == QMediaPlayer::PausedState)
        {
            ui->playButton->setIcon(QIcon(":/icons/play"));
            if (enablePrevCapture)
                stopCapture(false);
        }
        // qDebug() << "videoPlayer.stateChanged:" << state << QDateTime::currentDateTime();
    });
    connect(player, &QMediaPlayer::positionChanged, this, [=](qint64 position){
        // 直播的话，一般都是 position=0, duration=0
        // qDebug() << "position changed:" << position << player->duration();
    });

    // 设置控件大小
    int uw = qMax(style()->pixelMetric(QStyle::PM_TitleBarHeight), 32);
    ui->playButton->setFixedSize(uw, uw);
    ui->saveCapture1Button->setFixedSize(uw, uw);
    ui->saveCapture5sButton->setFixedSize(uw, uw);
    ui->saveCapture13sButton->setFixedSize(uw, uw);
    ui->saveCapture30sButton->setFixedSize(uw, uw);
    ui->saveCapture60sButton->setFixedSize(uw, uw);

    // 设置音量
    player->setVolume(settings.value("videoplayer/volume", 50).toInt());
    setWindowOpacity(settings.value("videoplayer/opacity", 100).toInt() / 100.0);

    // 设置置顶
    if (settings.value("videoplayer/top", false).toBool())
        switchOnTop();

    // 设置全屏
    if (settings.value("videoplayer/fullScreen", false).toBool())
        switchFullScreen();

    // 设置预先截图
    captureTimer = new QTimer(this);
    captureTimer->setInterval(100);
    connect(captureTimer, SIGNAL(timeout()), this, SLOT(slotSaveCurrentCapture()));
    captureInterval = settings.value("videoplayer/captureInterval", 100).toInt();
    enablePrevCapture = settings.value("videoplayer/capture", false).toBool();
    transformation = (Qt::TransformationMode)settings.value("videoplayer/transformation", 0).toInt();
    if (useVideoWidget)
        enablePrevCapture = false;
    if (!enablePrevCapture)
        showCaptureButtons(false);
    captureDir = QDir(QApplication::applicationDirPath() + "/captures");
}

LiveVideoPlayer::~LiveVideoPlayer()
{
    delete ui;
}

void LiveVideoPlayer::setRoomId(QString roomId)
{
    this->roomId = roomId;
    refreshPlayUrl();

    captureDir = QDir(QApplication::applicationDirPath() + "/capture/" + roomId);
}

void LiveVideoPlayer::slotLiveStart(QString roomId)
{
    if (this->roomId != roomId)
        return ;
    QTimer::singleShot(useVideoWidget ? 500 : 500, [=]{
        refreshPlayUrl();
    });
}

void LiveVideoPlayer::setPlayUrl(QString url)
{
    player->setMedia(QMediaContent((QUrl(url))));
    if (player->state() == QMediaPlayer::StoppedState)
        player->play();
}

void LiveVideoPlayer::refreshPlayUrl()
{
    if (roomId.isEmpty())
        return ;
    QString url = "http://api.live.bilibili.com/room/v1/Room/playUrl?cid=" + roomId
            + "&quality=4&qn=10000&platform=web&otype=json";
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray data = reply->readAll();
        manager->deleteLater();
        delete request;
        reply->deleteLater();

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        if (json.value("code").toInt() != 0)
        {
            qDebug() << ("返回结果不为0：") << json.value("message").toString();
            return ;
        }

        // 获取链接
        QJsonArray array = json.value("data").toObject().value("durl").toArray();
        if (!array.size())
        {
            QMessageBox::warning(this, "播放视频", "未找到可用的链接\n" + data);
            return ;
        }
        QString url = array.first().toObject().value("url").toString(); // 第一个链接
        qDebug() << "playUrl:" << url;
        setPlayUrl(url);
    });
    manager->get(*request);
}

void LiveVideoPlayer::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);
    restoreGeometry(settings.value("videoplayer/geometry").toByteArray());

    if (!player->media().isNull())
    {
        player->play();
    }
}

void LiveVideoPlayer::hideEvent(QHideEvent *e)
{
    settings.setValue("videoplayer/geometry", this->saveGeometry());

    if (player->state() == QMediaPlayer::PlayingState)
    {
        player->pause();
    }

    QDialog::hideEvent(e);
}

void LiveVideoPlayer::resizeEvent(QResizeEvent *e)
{
    QDialog::resizeEvent(e);
    calcVideoRect();
}

void LiveVideoPlayer::on_videoWidget_customContextMenuRequested(const QPoint&)
{
    auto setHd = [=]{
        QSize hint = useVideoWidget ? ui->videoWidget->sizeHint() : videoSize;
        if (hint.height() < 10 || hint.width() < 10)
            return ;
        QWidget* aw = useVideoWidget ? (QWidget*)(ui->videoWidget) : (QWidget*)(ui->label);
        // QSize minSize = aw->minimumSize();
        QSize maxSize = aw->maximumSize();
        aw->setFixedSize(hint);
        this->layout()->activate();
        this->adjustSize();
        aw->setMinimumSize(QSize(1, 1));
        aw->setMaximumSize(maxSize);
    };

    auto setScale = [=]{
        QSize hint = useVideoWidget ? ui->videoWidget->sizeHint() : videoSize;
        if (hint.height() < 10 || hint.width() < 10)
            return ;
        // 根据宽高自动修改窗口大小
        QWidget* aw = useVideoWidget ? (QWidget*)(ui->videoWidget) : (QWidget*)(ui->label);
        int w = aw->width(), h = aw->height();
        double prob = double(hint.width()) / hint.height(); // 宽高比
        double p = double(w) / h;
        int addin = !ui->playButton->isHidden() ? ui->playButton->height() : 0;
        if (p >= prob) // 更宽，调节宽度
        {
            this->resize(h * prob +addin, h);
        }
        else // 更高，调节高度
        {
            this->resize(w, w / prob);
        }
    };

    newFacileMenu;
    menu->addAction(QIcon(":/icons/play2"), "播放", [=]{
        player->play();
    })->hide(player->state() == QMediaPlayer::PlayingState);
    menu->addAction(QIcon(":/icons/pause2"), "暂停", [=]{
        player->pause();
    })->hide(player->state() != QMediaPlayer::PlayingState);

    menu->addAction(QIcon(":icons/refresh2"), "刷新", [=]{
        refreshPlayUrl();
    });

    menu->split()->addAction(QIcon(":/icons/hd"), "原画尺寸", setHd);

    menu->addAction(QIcon(":/icons/scale"), "适配缩放", setScale);

    menu->addAction(QIcon(":/icons/fullscreen"), "全屏播放", [=]{
        switchFullScreen();
    })->check((useVideoWidget&& ui->videoWidget->isFullScreen())
              || (!useVideoWidget && ui->label->isFullScreen()));

    menu->addAction(QIcon(":/icons/on_top"), "窗口置顶", [=]{
        switchOnTop();
    })->check(windowFlags() & Qt::WindowStaysOnTopHint);

    FacileMenu* volumeMenu = menu->split()->addMenu(QIcon(":/icons/volume2"), "调节音量");
    volumeMenu->addNumberedActions("%1", 0, 110, [=](FacileMenuItem* item, int val){
        item->check(val == (player->volume()+5) / 10 * 10);
    }, [=](int vol){
        settings.setValue("videoplayer/volume", vol);
        player->setVolume(vol);
    }, 10);

    menu->addAction(QIcon(":/icons/mute2"), "一键静音", [=]{
        if (player->volume())
            player->setVolume(0);
        else
            player->setVolume(settings.value("videoplayer/volume", 50).toInt());
    })->check(!player->volume());

    FacileMenu* opacityMenu = menu->addMenu(QIcon(":/icons/opacity"), "窗口透明");
    opacityMenu->addNumberedActions("%1", 0, 110, [=](FacileMenuItem* item, int val){
        item->check(val == int((this->windowOpacity() + 0.005) * 10) * 10);
    }, [=](int opa){
        settings.setValue("videoplayer/opacity", opa);
        setWindowOpacity(opa / 100.0);
    }, 10);

    menu->split()->addAction(QIcon(":/icons/capture"), "截图模式", [=]{
        settings.setValue("videoplayer/useVideoWidget", !useVideoWidget);
        this->close();
        emit signalRestart();
    })->check(!useVideoWidget);

    menu->addAction(QIcon(":/icons/linear"), "线性缩放", [=]{
        transformation = transformation ? Qt::FastTransformation : Qt::SmoothTransformation;
        settings.setValue("videoplayer/transformation", transformation);
    })->check(!transformation)->hide(useVideoWidget);

    menu->addAction(QIcon(":/icons/previous"), "预先截图", [=]{
        enablePrevCapture = !settings.value("videoplayer/capture", false).toBool();
        QSize sz = this->size();
        int deltaHeight = ui->playButton->height();
        if (enablePrevCapture)
        {
            if (player->state() == QMediaPlayer::PlayingState)
                startCapture();
        }
        else
        {
            stopCapture(true);
        }
        showCaptureButtons(enablePrevCapture);
        settings.setValue("videoplayer/capture", enablePrevCapture);
        sz.setHeight(sz.height() + (enablePrevCapture ? deltaHeight : -deltaHeight));
        if (!this->isMaximized() && !this->isFullScreen())
            this->resize(sz);
    })->check(enablePrevCapture)->hide(useVideoWidget);

    FacileMenu* frameMenu = menu->addMenu(QIcon(":/icons/frame"), "捕获帧率");
    menu->lastAction()->hide(useVideoWidget);
    QStringList frameText{"10帧", "30帧", "60帧", "自定义"};
    int state = 3;
    if (captureInterval == 100)
        state = 0;
    else if (captureInterval == 33)
        state = 1;
    else if (captureInterval == 16)
        state = 2;
    frameMenu->addOptions(frameText, state, [=](int index){
        if (index == 0)
            captureInterval = 100;
        else if (index == 1)
            captureInterval = 33;
        else if (index == 2)
            captureInterval = 16;
        else // 自己输入帧率
        {
            bool ok = false;
            double input = QInputDialog::getDouble(this, "设置帧率", "设置每秒钟截图多少次，越大所占内存越高", 1000.0 / captureInterval, 0.0001, 1000, 1, &ok);
            if (!ok)
                return ;
            if (input < 0.0001)
                input = 0.0001;
            captureInterval = int(1000.0 / input);
        }
        settings.setValue("videoplayer/captureInterval", captureInterval);
    });

    menu->addAction(QIcon(":/icons/capture_manager"), "截图管理", [=]{
        if (!pictureBrowser)
        {
            pictureBrowser = new PictureBrowser(settings, nullptr);
            pictureBrowser->readDirectory(captureDir.absolutePath());
        }

        pictureBrowser->show();
    })->hide(useVideoWidget);

    menu->exec();
}

void LiveVideoPlayer::on_playButton_clicked()
{
    if (player->state() == QMediaPlayer::PlayingState)
        player->pause();
    else
        player->play();
}

void LiveVideoPlayer::on_saveCapture1Button_clicked()
{
    if (!capturePixmaps->size())
    {
        saveCapture(); // 直接截图
    }
    else // 只保存第一张图
    {
        QPixmap *pixmap = capturePixmaps->last().second;
        captureDir.mkpath(captureDir.absolutePath());
        pixmap->save(timeToPath(QDateTime::currentMSecsSinceEpoch()));
    }
}

void LiveVideoPlayer::on_saveCapture5sButton_clicked()
{
    saveCapture(5);
}

void LiveVideoPlayer::on_saveCapture13sButton_clicked()
{
    saveCapture(13);
}

void LiveVideoPlayer::on_saveCapture30sButton_clicked()
{
    saveCapture(30);
}

void LiveVideoPlayer::on_saveCapture60sButton_clicked()
{
    saveCapture(60);
}

void LiveVideoPlayer::switchFullScreen()
{
    if (useVideoWidget)
    {
        bool fullScreen = !ui->videoWidget->isFullScreen();
        if (fullScreen && (windowFlags() & Qt::WindowStaysOnTopHint))
        {
            switchOnTop();
            settings.setValue("videoplayer/top", true);
        }
        else if (!fullScreen && settings.value("videoplayer/top", false).toBool()
                  && !(windowFlags() & Qt::WindowStaysOnTopHint))
        {
            switchOnTop();
        }
        ui->videoWidget->setFullScreen(fullScreen);
        settings.setValue("videoplayer/fullScreen", fullScreen);

    }
    else
    {
        bool fullScreen = !ui->label->isFullScreen();
        if (fullScreen) // 切换到fullScreen
        {
            if (windowFlags() & Qt::WindowStaysOnTopHint)
            {
                switchOnTop();
                settings.setValue("videoplayer/top", true);
            }

            this->layout()->removeWidget(ui->label);
            ui->label->setWindowFlag(Qt::Window);
            ui->label->showFullScreen();
        }
        else
        {
            if (settings.value("videoplayer/top", false).toBool() && !(windowFlags() & Qt::WindowStaysOnTopHint))
            {
                switchOnTop();
            }

            ui->label->setWindowFlag(Qt::Window, false);
            static_cast<QVBoxLayout*>(this->layout())->insertWidget(1, ui->label, 1);
        }
        settings.setValue("videoplayer/fullScreen", fullScreen);
    }
}

void LiveVideoPlayer::switchOnTop()
{
    Qt::WindowFlags flags = windowFlags();
    bool toTop = !(flags & Qt::WindowStaysOnTopHint);
    if (!toTop)
        setWindowFlag(Qt::WindowStaysOnTopHint, false);
    else
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
    show();
    settings.setValue("videoplayer/top", toTop);
}

/**
 * 位置改变时，计算视频的位置
 */
void LiveVideoPlayer::calcVideoRect()
{
    QSize hint = ui->videoWidget->sizeHint(); // 视频大小
    QSize size = ui->videoWidget->size();
    if (size.width() < 1 || size.height() < 1)
        return ;
    if (hint.width() < 8 || hint.height() < 8)
        hint = size;
    double hintProb = double(hint.width()) / hint.height();
    double sizeProb = double(size.width()) / size.height();
    if (hintProb >= sizeProb) // 上下黑
    {
        videoRect = QRect(0, (size.height() - size.width() / hintProb) / 2,
                          size.width(), size.width() / hintProb);
    }
    else // 左右黑
    {
        videoRect = QRect((size.width() - size.height() * hintProb) / 2, 0,
                          size.height() * hintProb, size.height());
    }
    videoRect.moveTopLeft(videoRect.topLeft() + ui->videoWidget->pos());
}

void LiveVideoPlayer::slotSaveCurrentCapture()
{
    if (!capturePixmaps)
    {
        qWarning() << "未初始化capturePixmaps";
        return ;
    }

    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    QPixmap* pixmap = new QPixmap(videoRect.size());
    // ui->videoWidget->render(pixmap, QPoint(0, 0), videoRect);
    this->render(pixmap, QPoint(0, 0), videoRect);
    if (!pixmap->isNull())
        capturePixmaps->append(QPair<qint64, QPixmap*>(timestamp, pixmap));
    // qDebug() << "预先截图" << videoRect << timestamp << capturePixmaps->size();

    while (capturePixmaps->size())
    {
        if (capturePixmaps->first().first + captureMaxLong < timestamp)
            delete capturePixmaps->takeFirst().second;
        else
            break;
    }
}

void LiveVideoPlayer::slotSaveFrameCapture(const QPixmap &pixmap)
{
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    if (timestamp - prevCaptureTimestamp < captureInterval) // 避免太过频繁，因为是截图，最大30帧就可以了
        return ;
    prevCaptureTimestamp = timestamp;
    QPixmap p = pixmap.copy(pixmap.rect());
    QPixmap* pp = new QPixmap(p);
    capturePixmaps->append(QPair<qint64, QPixmap*>(timestamp, pp));
    // qDebug() << "预先截图" << videoRect << timestamp << capturePixmaps->size();

    while (capturePixmaps->size())
    {
        if (capturePixmaps->first().first + captureMaxLong < timestamp)
        {
            delete capturePixmaps->takeFirst().second;
        }
        else
            break;
    }
}

void LiveVideoPlayer::startCapture()
{
    if (!capturePixmaps)
        capturePixmaps = new QList<QPair<qint64, QPixmap*>>();
    captureRunning = true;
//    captureTimer->start();
}

void LiveVideoPlayer::stopCapture(bool clear)
{
    captureRunning = false;
//    captureTimer->stop();
    if (clear && capturePixmaps)
    {
        foreach (auto pair, *capturePixmaps)
        {
            delete pair.second;
        }
        delete capturePixmaps;
        capturePixmaps = nullptr;
    }
}

void LiveVideoPlayer::saveCapture()
{
    if (!capturePixmaps)
    {
        qWarning() << "未初始化capturePixmaps";
        return ;
    }

    QPixmap pixmap = QPixmap(this->size());
    pixmap.fill(Qt::transparent);
    this->render(&pixmap, QPoint(0, 0), QRect(0,0,width(), height()));
    captureDir.mkpath(captureDir.absolutePath());
    pixmap.save(timeToPath(QDateTime::currentMSecsSinceEpoch()));
}

void LiveVideoPlayer::saveCapture(int second)
{
    if (!capturePixmaps)
    {
        qWarning() << "未初始化capturePixmaps";
        return ;
    }

    // 重新开始一轮新的
    auto list = this->capturePixmaps;
    this->capturePixmaps = nullptr;
    startCapture();

    // 子线程保存图片
    try {
        QtConcurrent::run([=]{
            int delta = second * 1000;
            qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();

            QDir dir = captureDir;
            QString dirName = timeToFileName(currentTimestamp);
            dir.mkpath(dir.absolutePath() + "/" + dirName);
            dir.cd(dirName);

            // 保存录制参数
            QSettings params(dir.absoluteFilePath(CAPTURE_PARAM_FILE), QSettings::IniFormat);
            params.setValue("gif/interval", captureTimer->interval());

            // 计算要保存的起始位置
            int maxSize = list->size();
            int start = maxSize;
            while (start > 0 && list->at(start-1).first + delta >= currentTimestamp)
                start--;

            // 清理无用的
            for (int i = 0; i < start; i++)
                delete list->at(i).second;

            // 确保有保存的项
            if (start >= maxSize)
                return ;

            // 记录保存时间
            params.setValue("time/start", list->at(start).first);
            params.setValue("time/end", list->at(maxSize-1).first);
            params.sync();

            // 开始保存
            for (int i = start; i < maxSize; i++)
            {
                auto cap = list->at(i);
                QString fileName = dir.absoluteFilePath(timeToFileName(cap.first));
                cap.second->save(fileName + ".jpg", "jpg");
                delete cap.second;
            }
            qDebug() << "已保存" << (maxSize-start) << "张预先截图";
            delete list;
        });
    } catch (...) {
        qDebug() << "创建保存线程失败，请增加间隔（降低帧率）";
    }
}

void LiveVideoPlayer::showCaptureButtons(bool show)
{
    QList<QWidget*> ws {
        ui->playButton,
                ui->saveCapture1Button,
                ui->saveCapture5sButton,
                ui->saveCapture13sButton,
                ui->saveCapture30sButton,
                ui->saveCapture60sButton,
                ui->widget
    };

    foreach (QWidget* w, ws)
    {
        w->setVisible(show);
    }
}

QString LiveVideoPlayer::timeToPath(const qint64 &time)
{
    return captureDir.filePath(timeToFileName(time) + ".jpg");
}

QString LiveVideoPlayer::timeToFileName(const qint64 &time)
{
    return QDateTime::fromMSecsSinceEpoch(time)
            .toString("yyyy-MM-dd hh-mm-ss.zzz");
}

void LiveVideoPlayer::on_label_customContextMenuRequested(const QPoint &pos)
{
    on_videoWidget_customContextMenuRequested(pos);
}
