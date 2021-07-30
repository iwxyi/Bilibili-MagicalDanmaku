#include "orderplayerwindow.h"
#include "ui_orderplayerwindow.h"
#include "stringutil.h"
#include "fileutil.h"
#include "importsongsdialog.h"
#include "netutil.h"

OrderPlayerWindow::OrderPlayerWindow(QWidget *parent)
    : OrderPlayerWindow(QApplication::applicationDirPath() + "/", parent)
{
}

OrderPlayerWindow::OrderPlayerWindow(QString dataPath, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::OrderPlayerWindow),
      settings(dataPath + "musics.ini", QSettings::Format::IniFormat),
      musicsFileDir(dataPath+"musics"), localMusicsFileDir(dataPath+"local_musics"),
      player(new QMediaPlayer(this)),
      desktopLyric(new DesktopLyricWidget(settings, nullptr)),
      expandPlayingButton(new InteractiveButtonBase(this))
{
    starting = true;
    ui->setupUi(this);
    ui->lyricWidget->setSettings(&settings);

    connect(ui->lyricWidget, SIGNAL(signalRowChanged()), this, SIGNAL(signalLyricChanged()));

    QHeaderView* header = ui->searchResultTable->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setMinimumSectionSize(QFontMetrics(this->font()).horizontalAdvance("哈哈哈哈哈哈"));
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    header->setStyleSheet("QHeaderView { background-color: transparent; }");
    ui->searchResultTable->verticalHeader()->setStyleSheet("QHeaderView { background-color: transparent; }");

    new NoFocusDelegate(ui->searchResultTable, 4);
    new NoFocusDelegate(ui->orderSongsListView);
    new NoFocusDelegate(ui->normalSongsListView);
    new NoFocusDelegate(ui->favoriteSongsListView);
    new NoFocusDelegate(ui->listSongsListView);
    new NoFocusDelegate(ui->historySongsListView);

    QString vScrollBarSS("QScrollBar:vertical"
                         "{"
                         "    width:7px;"
                         "    background:rgba(128,128,128,0%);"
                         "    margin:0px,0px,0px,0px;"
                         "    padding-top:0px;"
                         "    padding-bottom:0px;"
                         "}"
                         "QScrollBar::handle:vertical"
                         "{"
                         "    width:7px;"
                         "    background:rgba(128,128,128,32%);"
                         "    border-radius:3px;"
                         "    min-height:20;"
                         "}"
                         "QScrollBar::handle:vertical:hover"
                         "{"
                         "    width:7px;"
                         "    background:rgba(128,128,128,50%);"
                         "    min-height:20;"
                         "}"
                         "QScrollBar::sub-line:vertical"
                         "{"
                         "    height:9px;width:8px;"
                         "    border-image:url(:/images/a/1.png);"
                         "    subcontrol-position:top;"
                         "}"
                         "QScrollBar::add-line:vertical"
                         "{"
                         "    height:9px;width:8px;"
                         "    border-image:url(:/images/a/3.png);"
                         "    subcontrol-position:bottom;"
                         "}"
                         "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical"
                         "{"
                         "    background:rgba(0,0,0,0%);"
                         "    border-radius:3px;"
                         "}");

    QString hScrollBarSS("QScrollBar:horizontal"
                         "{"
                         "    height:7px;"
                         "    background:rgba(128,128,128,0%);"
                         "    margin:0px,0px,0px,0px;"
                         "    padding-left:0px;"
                         "    padding-right:0px;"
                         "}"
                         "QScrollBar::handle:horizontal"
                         "{"
                         "    height:7px;"
                         "    background:rgba(128,128,128,32%);"
                         "    border-radius:3px;"
                         "    min-width:20;"
                         "}"
                         "QScrollBar::handle:horizontal:hover"
                         "{"
                         "    height:7px;"
                         "    background:rgba(128,128,128,50%);"
                         "    min-width:20;"
                         "}"
                         "QScrollBar::sub-line:horizontal"
                         "{"
                         "    width:9px;height:8px;"
                         "    border-image:url(:/images/a/1.png);"
                         "    subcontrol-position:top;"
                         "}"
                         "QScrollBar::add-line:horizontal"
                         "{"
                         "    width:9px;height:8px;"
                         "    border-image:url(:/images/a/3.png);"
                         "    subcontrol-position:bottom;"
                         "}"
                         "QScrollBar::add-page:horizontal,QScrollBar::sub-page:horizontal"
                         "{"
                         "    background:rgba(0,0,0,0%);"
                         "    border-radius:3px;"
                         "}");
    ui->orderSongsListView->verticalScrollBar()->setStyleSheet(vScrollBarSS);
    ui->orderSongsListView->horizontalScrollBar()->setStyleSheet(hScrollBarSS);
    ui->favoriteSongsListView->verticalScrollBar()->setStyleSheet(vScrollBarSS);
    ui->favoriteSongsListView->horizontalScrollBar()->setStyleSheet(hScrollBarSS);
    ui->normalSongsListView->verticalScrollBar()->setStyleSheet(vScrollBarSS);
    ui->normalSongsListView->horizontalScrollBar()->setStyleSheet(hScrollBarSS);
    ui->historySongsListView->verticalScrollBar()->setStyleSheet(vScrollBarSS);
    ui->historySongsListView->horizontalScrollBar()->setStyleSheet(hScrollBarSS);
    ui->scrollArea->verticalScrollBar()->setStyleSheet(vScrollBarSS);
    ui->searchResultTable->verticalScrollBar()->setStyleSheet(vScrollBarSS);
    ui->searchResultTable->horizontalScrollBar()->setStyleSheet(hScrollBarSS);
    ui->listTabWidget->removeTab(LISTTAB_PLAYLIST); // TOOD: 歌单部分没做好，先隐藏
    ui->titleButton->setText(settings.value("music/title", "雪以音乐").toString());

    const int btnR = 5;
    ui->titleButton->setRadius(btnR);
    ui->musicSourceButton->setRadius(btnR);
    ui->searchButton->setRadius(btnR);
    ui->settingsButton->setRadius(btnR);
    ui->playButton->setRadius(btnR);
    ui->nextSongButton->setRadius(btnR);
    ui->volumeButton->setRadius(btnR);
    ui->circleModeButton->setRadius(btnR);
    ui->desktopLyricButton->setRadius(btnR);
    expandPlayingButton->setRadius(btnR);
    ui->musicSourceButton->setSquareSize();
    ui->searchButton->setSquareSize();
    ui->settingsButton->setSquareSize();
    ui->playButton->setSquareSize();
    ui->nextSongButton->setSquareSize();
    ui->circleModeButton->setSquareSize();
    ui->desktopLyricButton->setSquareSize();

    if (settings.value("music/hideTab", false).toBool())
        ui->listTabWidget->tabBar()->hide();

    musicSource = static_cast<MusicSource>(settings.value("music/source", 0).toInt());
    if (musicSource == UnknowMusic)
        musicSource = NeteaseCloudMusic;
    setMusicIconBySource();

    QPalette pa;
    pa.setColor(QPalette::Highlight, QColor(100, 149, 237, 88));
    this->setPalette(pa);

    connect(ui->searchResultTable->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(sortSearchResult(int)));

    player->setNotifyInterval(100);
    connect(player, &QMediaPlayer::positionChanged, this, [=](qint64 position){
        ui->playingCurrentTimeLabel->setText(msecondToString(position));
        slotPlayerPositionChanged();
    });
    connect(player, &QMediaPlayer::durationChanged, this, [=](qint64 duration){
        ui->playProgressSlider->setMaximum(static_cast<int>(duration));
        ui->playingAllTimeLabel->setText(msecondToString(duration));
        if (setPlayPositionAfterLoad)
        {
            player->setPosition(setPlayPositionAfterLoad);
            setPlayPositionAfterLoad = 0;
        }
    });
    connect(player, &QMediaPlayer::mediaStatusChanged, this, [=](QMediaPlayer::MediaStatus status){
        if (status == QMediaPlayer::EndOfMedia)
        {
            slotSongPlayEnd();
        }
        else if (status == QMediaPlayer::InvalidMedia)
        {
            qWarning() << "无效媒体：" << playingSong.simpleString();
            playNext();
        }
    });
    connect(player, &QMediaPlayer::stateChanged, this, [=](QMediaPlayer::State state){
        if (state == QMediaPlayer::PlayingState)
        {
            ui->playButton->setIcon(QIcon(":/icons/pause"));
        }
        else
        {
            ui->playButton->setIcon(QIcon(":/icons/play"));
        }
    });

    connect(ui->lyricWidget, SIGNAL(signalAdjustLyricTime(QString)), this, SLOT(adjustCurrentLyricTime(QString)));

    connect(ui->playProgressSlider, &ClickSlider::signalClickMove, this, [=](int position){
        player->setPosition(position);
    });
    connect(ui->volumeSlider, &ClickSlider::signalClickMove, this, [=](int position){
        player->setVolume(position);
        settings.setValue("music/volume", position);
    });

    musicsFileDir.mkpath(musicsFileDir.absolutePath());
    QTime time;
    time= QTime::currentTime();
    qsrand(uint(time.msec()+time.second()*1000));

    bool lyricStack = settings.value("music/lyricStream", false).toBool();
    if (lyricStack)
        ui->bodyStackWidget->setCurrentWidget(ui->lyricsPage);

    blurBg = settings.value("music/blurBg", blurBg).toBool();
    blurAlpha = settings.value("music/blurAlpha", blurAlpha).toInt();
    themeColor = settings.value("music/themeColor", themeColor).toBool();
    doubleClickToPlay = settings.value("music/doubleClickToPlay", false).toBool();
    accompanyMode = settings.value("music/accompanyMode", false).toBool();

    // 读取数据
    ui->listTabWidget->setCurrentIndex(settings.value("orderplayerwindow/tabIndex").toInt());
    restoreSongList("music/order", orderSongs);
    restoreSongList("music/favorite", favoriteSongs);
    restoreSongList("music/normal", normalSongs);
    restoreSongList("music/history", historySongs);
    setSongModelToView(orderSongs, ui->orderSongsListView);
    setSongModelToView(favoriteSongs, ui->favoriteSongsListView);
    setSongModelToView(normalSongs, ui->normalSongsListView);
    setSongModelToView(historySongs, ui->historySongsListView);

    int volume = settings.value("music/volume", 50).toInt();
    bool mute = settings.value("music/mute", false).toBool();
    if (mute)
    {
        volume = 0;
        ui->volumeButton->setIcon(QIcon(":/icons/mute"));
        ui->volumeSlider->setSliderPosition(volume);
    }
    ui->volumeSlider->setSliderPosition(volume);
    player->setVolume(volume);

    circleMode = static_cast<PlayCircleMode>(settings.value("music/mode", 0).toInt());
    if (circleMode == OrderList)
        ui->circleModeButton->setIcon(QIcon(":/icons/order_list"));
    else
        ui->circleModeButton->setIcon(QIcon(":/icons/single_circle"));

    connectDesktopLyricSignals();

    bool showDesktopLyric = settings.value("music/desktopLyric", false).toBool();
    if (showDesktopLyric)
    {
        desktopLyric->show();
        ui->desktopLyricButton->setIcon(QIcon(":/icons/lyric_show"));
    }
    else
    {
        desktopLyric->hide();
        ui->desktopLyricButton->setIcon(QIcon(":/icons/lyric_hide"));
    }

    autoSwitchSource = settings.value("music/autoSwitchSource", true).toBool();

    // 读取cookie
    songBr = settings.value("music/br", 320000).toInt();
    neteaseCookies = settings.value("music/neteaseCookies").toString();
    neteaseCookiesVariant = getCookies(neteaseCookies);
    qqmusicCookies = settings.value("music/qqmusicCookies").toString();
    qqmusicCookiesVariant = getCookies(qqmusicCookies);
    unblockQQMusic = settings.value("music/unblockQQMusic").toBool();
    auto getRandom = [=]{ return "qwertyuiopasdfghjklzxcvbnm1234567890"[qrand() % 36]; };
    kugouCookies = "kg_mid=";
    for (int i = 0; i < 10; i++)
        kugouCookies += getRandom();
    kugouCookiesVariant = getCookies(kugouCookies);
    getNeteaseAccount();
    getQQMusicAccount();

    // 默认背景
    prevBlurBg = QPixmap(32, 32);
    prevBlurBg.fill(QColor(240, 248, 255)); // 窗口背景颜色

    // 还原上次播放的歌曲
    Song currentSong = Song::fromJson(settings.value("music/currentSong").toJsonObject());
    if (currentSong.isValid())
    {
        startPlaySong(currentSong);// 还原位置

        qint64 playPosition = settings.value("music/playPosition", 0).toLongLong();
        if (playPosition)
        {
            setPlayPositionAfterLoad = playPosition;
            slotPlayerPositionChanged();
        }

        // 不自动播放
        player->stop();
    }
    else
    {
        // 设置默认封面
        /*prevBlurBg = currentBlurBg = QPixmap(":/bg/bg");
        prevBgAlpha = 255;
        currentBgAlpha = qMin(blurAlpha, 255);
        QTimer::singleShot(200, [=]{
            startBgAnimation(3000);
        });*/

        prevBlurBg = currentBlurBg = QPixmap(":/bg/bg");
        prevBgAlpha = 0;
        currentBgAlpha = 255;
        startBgAnimation(1500);
        QTimer::singleShot(1800, this, [=]{
            prevBlurBg = QPixmap(":/bg/bg");
            prevBgAlpha = currentBgAlpha;
            currentBgAlpha = qMin(blurAlpha, 255);
            startBgAnimation(2500);
        });
    }
    starting = false;
    settings.setValue("music/playPosition", 0);

    connect(expandPlayingButton, SIGNAL(clicked()), this, SLOT(slotExpandPlayingButtonClicked()));
    expandPlayingButton->show();

    // 还原搜索结果
    QString searchKey = settings.value("music/searchKey").toString();
    if (!searchKey.isEmpty())
    {
        QTimer::singleShot(100, [=]{
            if (!currentSong.isValid())
                ui->bodyStackWidget->setCurrentWidget(ui->searchResultPage);

            ui->searchEdit->setText(searchKey);
            searchMusic(searchKey);
        });
    }


    restoreGeometry(settings.value("orderplayerwindow/geometry").toByteArray());
    restoreState(settings.value("orderplayerwindow/state").toByteArray());

    if (settings.value("music/desktopLyric", false).toBool())
        desktopLyric->show();

    clearHoaryFiles();
}

OrderPlayerWindow::~OrderPlayerWindow()
{
    delete ui;
    desktopLyric->deleteLater();
}

const Song &OrderPlayerWindow::getPlayingSong() const
{
    return playingSong;
}

const SongList &OrderPlayerWindow::getOrderSongs() const
{
    return orderSongs;
}

const QStringList OrderPlayerWindow::getSongLyrics(int rowCount) const
{
    return ui->lyricWidget->getLyrics(rowCount);
}

int OrderPlayerWindow::userOrderCount(QString by)
{
    int count = 0;
    foreach (Song song, orderSongs)
    {
        if (song.addBy == by)
            count++;
    }
    return count;
}

const QPixmap OrderPlayerWindow::getCurrentSongCover() const
{
    if (!playingSong.isValid())
        return QPixmap();
    return coverPath(playingSong);
}

void OrderPlayerWindow::on_searchEdit_returnPressed()
{
    QString text = ui->searchEdit->text();
    if (text.isEmpty())
        return ;
    settings.setValue("music/searchKey", text);
    searchMusic(text);
    ui->bodyStackWidget->setCurrentWidget(ui->searchResultPage);
}

void OrderPlayerWindow::on_searchButton_clicked()
{
    on_searchEdit_returnPressed();
}

/**
 * 点歌并且添加到末尾
 */
void OrderPlayerWindow::slotSearchAndAutoAppend(QString key, QString by)
{
    if (!playingSong.isValid() && (!playAfterDownloaded.isValid() || !downloadingSong.isValid()))
        emit signalOrderSongStarted();
    ui->searchEdit->setText(key);
    if (accompanyMode)
    {
        // 在前面加上“伴奏”俩字
        key = "伴奏 " + key;
    }
    searchMusic(key, by, true);
}

/**
 * 提升用户歌曲的序列
 * 队列中的第一首歌除外
 */
void OrderPlayerWindow::improveUserSongByOrder(QString username, int promote)
{
    for (int i = 1; i < orderSongs.size(); i++)
    {
        Song song = orderSongs.at(i);
        if (song.addBy != username)
            continue;

        // 提升这首歌
        int newIndex = i - promote;
        if (i < 0)
            i = 0;
        orderSongs.removeAt(i--);
        orderSongs.insert(newIndex, song);
        saveSongList("music/order", orderSongs);
        setSongModelToView(orderSongs, ui->orderSongsListView);
        emit signalOrderSongImproved(song, i, newIndex);
        break;
    }
}

void OrderPlayerWindow::cutSongIfUser(QString username)
{
    if (!playingSong.isValid())
        return ;
    if (playingSong.addBy != username)
        return ;
    emit signalOrderSongCutted(playingSong);
    on_nextSongButton_clicked();
}

void OrderPlayerWindow::cutSong()
{
    if (!playingSong.isValid())
        return ;
    emit signalOrderSongCutted(playingSong);
    on_nextSongButton_clicked();
}

/**
 * 搜索音乐
 */
void OrderPlayerWindow::searchMusic(QString key, QString addBy, bool notify)
{
    if (key.trimmed().isEmpty())
        return ;
    MusicSource source = musicSource; // 需要暂存一个备份，因为可能会变回去
    QString url;
    auto keyBa = key.toUtf8().toPercentEncoding();
    switch (source) {
    case UnknowMusic:
    case LocalMusic:
        return ;
    case NeteaseCloudMusic:
        url = NETEASE_SERVER + "/search?keywords=" + keyBa + "&limit=80";
        break;
    case QQMusic:
        url = QQMUSIC_SERVER + "/search?key=" + keyBa + "&limit=80";
        break;
    case MiguMusic:
        url = MIGU_SERVER + "/search?keyword=" + keyBa + "&pageSize=100";
        break;
    case KugouMusic:
        QByteArray allBa = "%E5%85%A8%E9%83%A8";
        // url = "http://msearchcdn.kugou.com/api/v3/search/song?showtype=14&highlight=em&pagesize=30&tag_aggr=1&tagtype=" + allBa + "&plat=0&sver=5&keyword=" + keyBa + "&correct=1&api_ver=1&version=9108&page=1&area_code=1&tag=1&with_res_tag=1";
        url = "http://mobilecdn.kugou.com/api/v3/search/song?keyword=" + keyBa + "&page=1&pagesize=50";
        break;
    }
    fetch(url, [=](QJsonObject json){
        bool insertOnce = this->insertOrderOnce;
        this->insertOrderOnce = false;
        currentResultOrderBy = addBy;
        prevOrderSong = Song();

        QJsonObject response;
        switch (source) {
        case UnknowMusic:
        case LocalMusic:
            return ;
        case NeteaseCloudMusic:
            if (json.value("code").toInt() != 200)
            {
                qWarning() << "网易云返回结果不为200：" << json.value("message").toString();
                return ;
            }
            break;
        case QQMusic:
        {
            if (json.value("result").toInt() != 100)
            {
                qWarning() << "QQ搜索返回结果不为0：" << json;
                return ;
            }
            break;
        }
        case MiguMusic:
            if (json.value("result").toInt() != 100)
            {
                qWarning() << "咪咕搜索返回结果不为1000：" << json.value("result").toInt();
                return ;
            }
            break;
        case KugouMusic:
            if (json.value("errCode").toInt() != 0)
            {
                qWarning() << "酷狗搜索返回结果不为0：" << json.value("errCode").toInt() << json.value("error").toString();
                return ;
            }
            break;
        }

        QJsonArray songs;
        switch (source) {
        case UnknowMusic:
        case LocalMusic:
            return ;
        case NeteaseCloudMusic:
            songs = json.value("result").toObject().value("songs").toArray();
            break;
        case QQMusic:
        case MiguMusic:
            songs = json.value("data").toObject().value("list").toArray();
            break;
        case KugouMusic:
            songs = json.value("data").toObject().value("info").toArray();
            break;
        }
        searchResultSongs.clear();
        switch (source) {
        case UnknowMusic:
        case LocalMusic:
            return ;
        case NeteaseCloudMusic:
            foreach (QJsonValue val, songs)
                searchResultSongs << Song::fromJson(val.toObject());
            break;
        case QQMusic:
            foreach (QJsonValue val, songs)
                searchResultSongs << Song::fromQQMusicJson(val.toObject());
            break;
        case MiguMusic:
            foreach (QJsonValue val, songs)
                searchResultSongs << Song::fromMiguMusicJson(val.toObject());
            break;
        case KugouMusic:
            foreach (QJsonValue val, songs)
                searchResultSongs << Song::fromKugouMusicJson(val.toObject());
            break;
        }

        setSearchResultTable(searchResultSongs);

        // 判断历史，优先 收藏 > 空闲 > 搜索
        Song song;
        if (!addBy.isEmpty() && key.trimmed().length() >= 2)
        {
            QString kk = transToReg(key);
            QStringList words = kk.split(QRegExp("[- ]+"), QString::SkipEmptyParts);
            QString exp = words.join(".*");
            QRegularExpression re(exp);

            SongList mySongs = favoriteSongs + normalSongs;
            foreach (Song s, mySongs)
            {
                if ((s.name + s.artistNames).contains(re))
                {
                    song = s;
                    qInfo() << "优先播放收藏/空闲歌单歌曲：" << song.name << exp;
                    break;
                }
            }
        }

        // 没有搜索结果，也没有能播放的歌曲，直接返回
        if (!searchResultSongs.size() && !song.isValid())
            return ;

        // 从点歌的槽进来的
        if (!addBy.isEmpty())
        {
            if (!song.isValid()) // 历史中没找到，就从搜索结果中找
                song = getSuiableSong(key);
            song.setAddDesc(addBy);
            prevOrderSong = song;

            // 添加到点歌列表
            if (playingSong == song || orderSongs.contains(song)) // 重复点歌
                return ;

            if (insertOnce) // 可能是换源过来的
            {
                if (isNotPlaying()) // 很可能是换源过来的
                    startPlaySong(song);
                else
                    appendNextSongs(SongList{song});
            }
            else
                appendOrderSongs(SongList{song});

            // 发送点歌成功的信号
            if (notify)
            {
                qint64 sumLatency = isNotPlaying() ? 0 : player->duration() - player->position();
                for (int i = 0; i < orderSongs.size()-1; i++)
                {
                    if (orderSongs.at(i).id == song.id) // 同一首歌，如果全都不同，那就是下一首
                        break;
                    sumLatency += orderSongs.at(i).duration;
                }
                emit signalOrderSongSucceed(song, sumLatency, orderSongs.size());
            }
        }
        else if (insertOnce) // 强制播放啊
        {
            Song song = getSuiableSong(key);
            if (isNotPlaying()) // 很可能是换源过来的
                startPlaySong(song);
            else
                appendNextSongs(SongList{song});
        }
    }, source);
}

void OrderPlayerWindow::searchMusicBySource(QString key, MusicSource source, QString addBy)
{
    MusicSource originSource = musicSource;
    musicSource = source;
    searchMusic(key, addBy);
    musicSource = originSource;
}

/**
 * 搜索结果数据到Table
 */
void OrderPlayerWindow::setSearchResultTable(SongList songs)
{
    QTableWidget* table = ui->searchResultTable;
    table->clear();
    searchResultPlayLists.clear();
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    enum {
        titleCol,
        artistCol,
        albumCol,
        durationCol
    };
    table->setColumnCount(4);
    QStringList headers{"标题", "艺术家", "专辑", "时长"};
    table->setHorizontalHeaderLabels(headers);

    QFontMetrics fm(font());
    int fw = fm.horizontalAdvance("哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈哈");
    auto createItem = [=](QString s){
        QTableWidgetItem *item = new QTableWidgetItem();
        if (s.length() > 16 && fm.horizontalAdvance(s) > fw)
        {
            item->setToolTip(s);
            s = s.left(15) + "...";
        }
        item->setText(s);
        return item;
    };

    table->setRowCount(songs.size());
    for (int row = 0; row < songs.size(); row++)
    {
        Song song = songs.at(row);
        table->setItem(row, titleCol, createItem(song.name));
        table->setItem(row, artistCol, createItem(song.artistNames));
        table->setItem(row, albumCol, createItem(song.album.name));
        table->setItem(row, durationCol, createItem(msecondToString(song.duration)));
    }
    table->verticalScrollBar()->setSliderPosition(0); // 置顶

    QTimer::singleShot(0, [=]{
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    });
}

void OrderPlayerWindow::setSearchResultTable(PlayListList playLists)
{
    Q_UNUSED(playLists)
    QTableWidget* table = ui->searchResultTable;
    table->clear();
    searchResultSongs.clear();

}

void OrderPlayerWindow::addFavorite(SongList songs)
{
    QPoint center = ui->listTabWidget->tabBar()->tabRect(LISTTAB_FAVORITE).center();
    foreach (auto song, songs)
    {
        if (favoriteSongs.contains(song))
        {
            qInfo() << "歌曲已收藏：" << song.simpleString();
            continue;
        }
        favoriteSongs.append(song);
        showTabAnimation(center, "+1");
        qInfo() << "添加收藏：" << song.simpleString();
    }
    saveSongList("music/favorite", favoriteSongs);
    setSongModelToView(favoriteSongs, ui->favoriteSongsListView);
}

void OrderPlayerWindow::removeFavorite(SongList songs)
{
    QPoint center = ui->listTabWidget->tabBar()->tabRect(LISTTAB_FAVORITE).center();
    foreach (Song song, songs)
    {
        if (favoriteSongs.removeOne(song))
        {
            showTabAnimation(center, "-1");
            qInfo() << "取消收藏：" << song.simpleString();
        }
        if (song.source == LocalMusic && !song.filePath.startsWith("file:///"))
        {
            if (!orderSongs.contains(song)
                    && !normalSongs.contains(song))
            {
                deleteFile(songPath(song));
                historySongs.removeOne(song);
            }
        }
    }
    saveSongList("music/favorite", favoriteSongs);
    setSongModelToView(favoriteSongs, ui->favoriteSongsListView);
}

void OrderPlayerWindow::addNormal(SongList songs)
{
    QPoint center = ui->listTabWidget->tabBar()->tabRect(LISTTAB_NORMAL).center();
    foreach (Song song, songs)
    {
        normalSongs.removeOne(song);
    }
    for (int i = songs.size()-1; i >= 0; i--)
    {
        normalSongs.insert(0, songs.at(i));
        showTabAnimation(center, "+1");
    }
    randomSongList.clear();
    saveSongList("music/normal", normalSongs);
    setSongModelToView(normalSongs, ui->normalSongsListView);
}

void OrderPlayerWindow::removeNormal(SongList songs)
{
    QPoint center = ui->listTabWidget->tabBar()->tabRect(LISTTAB_NORMAL).center();
    foreach (Song song, songs)
    {
        if (normalSongs.removeOne(song))
            showTabAnimation(center, "-1");
        if (song.source == LocalMusic && !song.filePath.startsWith("file:///"))
        {
            if (!orderSongs.contains(song)
                    && !favoriteSongs.contains(song))
            {
                deleteFile(songPath(song));
                historySongs.removeOne(song);
            }
        }
    }
    randomSongList.clear();
    saveSongList("music/normal", normalSongs);
    setSongModelToView(normalSongs, ui->normalSongsListView);
}

void OrderPlayerWindow::removeOrder(SongList songs)
{
    QPoint center = ui->listTabWidget->tabBar()->tabRect(LISTTAB_ORDER).center();
    foreach (Song song, songs)
    {
        if (orderSongs.removeOne(song))
            showTabAnimation(center, "-1");
        if (song.source == LocalMusic && !song.filePath.startsWith("file:///"))
        {
            if (!normalSongs.contains(song)
                    && !favoriteSongs.contains(song))
            {
                deleteFile(songPath(song));
                historySongs.removeOne(song);
            }
        }
    }
    saveSongList("music/order", orderSongs);
    setSongModelToView(orderSongs, ui->orderSongsListView);
}

void OrderPlayerWindow::saveSongList(QString key, const SongList &songs)
{
    QJsonArray array;
    foreach (Song song, songs)
        array.append(song.toJson());
    settings.setValue(key, array);
}

void OrderPlayerWindow::restoreSongList(QString key, SongList &songs)
{
    QJsonArray array = settings.value(key).toJsonArray();
    foreach (QJsonValue val, array)
        songs.append(Song::fromJson(val.toObject()));
}

/**
 * 更新Model到ListView
 */
void OrderPlayerWindow::setSongModelToView(const SongList &songs, QListView *listView)
{
    QStringList sl;
    foreach (Song song, songs)
    {
        sl << song.simpleString();
    }
    QAbstractItemModel *model = listView->model();
    if (model)
        delete model;
    model = new QStringListModel(sl);
    listView->setModel(model);

    if (listView == ui->orderSongsListView)
        emit signalOrderSongModified(songs);
}

/**
 * 音乐文件的路径
 */
QString OrderPlayerWindow::songPath(const Song &song) const
{
    switch (song.source) {
    case UnknowMusic:
        return musicsFileDir.absoluteFilePath("unknow_" + snum(song.id) + ".mp3");
    case NeteaseCloudMusic:
        return musicsFileDir.absoluteFilePath("netease_" + snum(song.id) + ".mp3");
    case QQMusic:
        return musicsFileDir.absoluteFilePath("qq_" + snum(song.id) + ".mp3");
    case MiguMusic:
        return musicsFileDir.absoluteFilePath("migu_" + snum(song.id) + ".mp3");
    case KugouMusic:
        return musicsFileDir.absoluteFilePath("kugou_" + song.mid + ".mp3");
    case LocalMusic:
        if (song.filePath.startsWith("file:///"))
            return QUrl(song.filePath).toLocalFile();
        return localMusicsFileDir.absoluteFilePath(song.filePath);
    }
    return "";
}

QString OrderPlayerWindow::lyricPath(const Song &song) const
{
    switch (song.source) {
    case UnknowMusic:
        return musicsFileDir.absoluteFilePath("unknow_" + snum(song.id) + ".lrc");
    case NeteaseCloudMusic:
        return musicsFileDir.absoluteFilePath("netease_" + snum(song.id) + ".lrc");
    case QQMusic:
        return musicsFileDir.absoluteFilePath("qq_" + snum(song.id) + ".lrc");
    case MiguMusic:
        return musicsFileDir.absoluteFilePath("migu_" + snum(song.id) + ".lrc");
    case KugouMusic:
        return musicsFileDir.absoluteFilePath("kugou_" + song.mid + ".lrc");
    case LocalMusic:
        QFileInfo info(songPath(song));
        if (!info.isFile())
            return "";
        QString baseName = info.baseName();
        if (useMyDirOfLyricsAndCover)
            return musicsFileDir.absoluteFilePath("local_" + baseName + ".lrc");
        else
            return info.dir().absoluteFilePath(baseName + ".lrc");
    }
    return "";
}

QString OrderPlayerWindow::coverPath(const Song &song) const
{
    switch (song.source) {
    case UnknowMusic:
        return musicsFileDir.absoluteFilePath("unknow_" + snum(song.id) + ".jpg");
    case NeteaseCloudMusic:
        return musicsFileDir.absoluteFilePath("netease_" + snum(song.id) + ".jpg");
    case QQMusic:
        return musicsFileDir.absoluteFilePath("qq_" + snum(song.id) + ".jpg");
    case MiguMusic:
        return musicsFileDir.absoluteFilePath("migu_" + snum(song.id) + ".jpg");
    case KugouMusic:
        return musicsFileDir.absoluteFilePath("kugou_" + song.mid + ".jpg");
    case LocalMusic:
        QFileInfo info(songPath(song));
        if (!info.isFile())
            return "";
        QString baseName = info.baseName();
        if (useMyDirOfLyricsAndCover)
            return musicsFileDir.absoluteFilePath("local_" + baseName + ".jpg");
        else
            return info.dir().absoluteFilePath(baseName + ".jpg");
    }
    return "";
}

/**
 * 歌曲是否已经被下载了
 */
bool OrderPlayerWindow::isSongDownloaded(Song song)
{
    return QFileInfo(songPath(song)).exists();
}

QString OrderPlayerWindow::msecondToString(qint64 msecond)
{
    if (!msecond)
        return "";
    return QString("%1:%2").arg(msecond/1000 / 60, 2, 10, QLatin1Char('0'))
            .arg(msecond/1000 % 60, 2, 10, QLatin1Char('0'));
}

void OrderPlayerWindow::activeSong(Song song)
{
    if (doubleClickToPlay)
        startPlaySong(song);
    else
        appendOrderSongs(SongList{song});
}

bool OrderPlayerWindow::isNotPlaying() const
{
    return player->state() != QMediaPlayer::PlayingState
            && (!playingSong.isValid() || player->position() == 0);
}

/**
 * 从结果中获取最适合的歌曲
 * 优先：歌名-歌手
 * 其次：歌名 歌手
 * 默认：搜索结果第一首歌
 */
Song OrderPlayerWindow::getSuiableSong(QString key) const
{
    Q_ASSERT(searchResultSongs.size());
    key = key.trimmed();

    // 歌名 - 歌手
    if (key.indexOf("-"))
    {
        QRegularExpression re("^\\s*(.+?)\\s*-\\s*(.+?)\\s*$");
        QRegularExpressionMatch match;
        if (key.indexOf(re, 0, &match) > -1)
        {
            QString name = match.captured(1);   // 歌名
            QString author = match.captured(2); // 歌手

            // 歌名、歌手全匹配
            foreach (Song song, searchResultSongs)
            {
                if (song.name == name && song.artistNames == author)
                    return song;
            }

            // 歌名全匹配、歌手包含
            foreach (Song song, searchResultSongs)
            {
                if (song.name == name && song.artistNames.contains(author))
                    return song;
            }

            // 歌名包含、歌手全匹配
            foreach (Song song, searchResultSongs)
            {
                if (song.name.contains(name) && song.artistNames == author)
                    return song;
            }

            // 歌名包含、歌手包含
            foreach (Song song, searchResultSongs)
            {
                if (song.name.contains(name) && song.artistNames.contains(author))
                    return song;
            }
        }
    }

    // 歌 名 歌 手
    if (key.contains(" "))
    {
        int pos = key.lastIndexOf(" ");
        while (pos > -1)
        {
            QString name = key.left(pos).trimmed();
            QString author = key.right(key.length() - pos - 1).trimmed();

            // 歌名、歌手全匹配
            foreach (Song song, searchResultSongs)
            {
                if (song.name == name && song.artistNames == author)
                    return song;
            }

            // 歌名全匹配、歌手包含
            foreach (Song song, searchResultSongs)
            {
                if (song.name == name && song.artistNames.contains(author))
                    return song;
            }

            // 歌名包含(cover)、歌手全匹配
            foreach (Song song, searchResultSongs)
            {
                if (song.name.contains(name) && song.artistNames == author)
                    return song;
            }

            // 歌名包含、歌手包含
            foreach (Song song, searchResultSongs)
            {
                if (song.name.contains(name) && song.artistNames.contains(author))
                    return song;
            }

            pos = key.lastIndexOf(" ", pos-1);
        }
    }

    // 没指定歌手，随便来一个就行
    return searchResultSongs.first();
}

void OrderPlayerWindow::showEvent(QShowEvent *e)
{
    QMainWindow::showEvent(e);
    repaint();

    ui->splitter->restoreState(settings.value("orderplayerwindow/splitterState").toByteArray());
    auto sizes = ui->splitter->sizes();
    if (sizes.at(0)+1 >= sizes.at(1))
    {
        int sum = sizes.at(0) + sizes.at(1);
        int w = sum / 3;
        ui->splitter->setSizes(QList<int>{w, sum - w});
    }

    adjustExpandPlayingButton();
}

void OrderPlayerWindow::closeEvent(QCloseEvent *)
{
    settings.setValue("orderplayerwindow/geometry", this->saveGeometry());
    settings.setValue("orderplayerwindow/state", this->saveState());
    settings.setValue("orderplayerwindow/splitterState", ui->splitter->saveState());
    settings.setValue("music/playPosition", player->position());

    if (player->state() == QMediaPlayer::PlayingState)
        player->pause();

    // 保存位置
    if (!desktopLyric->isHidden())
        desktopLyric->close();

    emit signalWindowClosed();
}

void OrderPlayerWindow::resizeEvent(QResizeEvent *)
{
    adjustExpandPlayingButton();
    // setBlurBackground(currentCover); // 太消耗性能了
}

void OrderPlayerWindow::paintEvent(QPaintEvent *e)
{
    QMainWindow::paintEvent(e);

    if (blurBg)
    {
        QPainter painter(this);
        if (!currentBlurBg.isNull())
        {
            painter.setOpacity(double(currentBgAlpha) / 255);
            painter.drawPixmap(rect(), currentBlurBg);
        }
        if (!prevBlurBg.isNull() && prevBgAlpha)
        {
            painter.setOpacity(double(prevBgAlpha) / 255);
            painter.drawPixmap(rect(), prevBlurBg);
        }
    }
}

void OrderPlayerWindow::setLyricScroll(int x)
{
    this->lyricScroll = x;
}

int OrderPlayerWindow::getLyricScroll() const
{
    return this->lyricScroll;
}

void OrderPlayerWindow::setAppearBgProg(int x)
{
    this->currentBgAlpha = x;
    update();
}

int OrderPlayerWindow::getAppearBgProg() const
{
    return this->currentBgAlpha;
}

void OrderPlayerWindow::setDisappearBgProg(int x)
{
    this->prevBgAlpha = x;
    update();
}

int OrderPlayerWindow::getDisappearBgProg() const
{
    return this->prevBgAlpha;
}

void OrderPlayerWindow::showTabAnimation(QPoint center, QString text)
{
    center = ui->listTabWidget->mapTo(this, center);
    QColor color = QColor::fromHsl(rand()%360,rand()%256,rand()%200); // 深色的颜色
    NumberAnimation *animation = new NumberAnimation(text, color, this);
    animation->setCenter(center + QPoint(rand() % 32 - 16, rand() % 32 - 16));
    animation->startAnimation();
}

void OrderPlayerWindow::setPaletteBgProg(double x)
{
    this->paletteAlpha = x;
}

double OrderPlayerWindow::getPaletteBgProg() const
{
    return paletteAlpha;
}

/**
 * 搜索结果双击
 * 还没想好怎么做……
 */
void OrderPlayerWindow::on_searchResultTable_cellActivated(int row, int)
{
    if (searchResultSongs.size())
    {
        Song song = searchResultSongs.at(row);
        removeOrder(SongList{song});
        activeSong(song);
    }
    else if (searchResultPlayLists.size())
    {

    }
}

/**
 * 搜索结果的菜单
 */
void OrderPlayerWindow::on_searchResultTable_customContextMenuRequested(const QPoint &)
{
    auto items = ui->searchResultTable->selectedItems();

    // 是歌曲搜索结果
    if (searchResultSongs.size())
    {
        QList<Song> songs;
        foreach (auto item, items)
        {
            int row = ui->searchResultTable->row(item);
            int col = ui->searchResultTable->column(item);
            if (col != 0)
                continue;
            songs.append(searchResultSongs.at(row));
        }
        int row = ui->searchResultTable->currentRow();
        Song currentSong;
        if (row > -1)
            currentSong = searchResultSongs.at(row);

        FacileMenu* menu = new FacileMenu(this);

        if (!currentResultOrderBy.isEmpty())
        {
            currentSong.setAddDesc(currentResultOrderBy);
            menu->addAction(currentResultOrderBy)->disable();
            menu->addAction("换这首歌", [=]{
                int index = orderSongs.indexOf(prevOrderSong);
                if (index == -1)
                {
                    if (playingSong == prevOrderSong) // 正在播放
                    {
                        startPlaySong(currentSong);
                        prevOrderSong = currentSong;
                        return ;
                    }
                    else // 可能被手动删除了
                    {
                        orderSongs.insert(0, currentSong);
                    }
                }
                else // 等待播放，自动替换
                {
                    orderSongs[index] = currentSong;
                }
                prevOrderSong = currentSong;
                saveSongList("music/order", orderSongs);
                setSongModelToView(orderSongs, ui->orderSongsListView);
                addDownloadSong(currentSong);
                downloadNext();
            })->disable(!currentSong.isValid() || !prevOrderSong.isValid());
            menu->split();
        }

        menu->addAction("立即播放", [=]{
            startPlaySong(currentSong);
        })->disable(songs.size() != 1);

        menu->addAction("下一首播放", [=]{
            appendNextSongs(songs);
        })->disable(!currentSong.isValid());

        menu->addAction("添加到播放列表", [=]{
            appendOrderSongs(songs);
        })->disable(!currentSong.isValid());

        menu->addAction("添加空闲播放", [=]{
            addNormal(songs);
        })->disable(!currentSong.isValid());

        menu->split()->addAction("收藏", [=]{
            if (!favoriteSongs.contains(currentSong))
                addFavorite(songs);
            else
                removeFavorite(songs);
        })->disable(!currentSong.isValid())
                ->text(favoriteSongs.contains(currentSong), "从收藏中移除", "添加到收藏");

        menu->addAction("导入歌单链接", [=]{
            inputPlayList();
        });

        menu->addAction("批量导入歌曲", [=]{
            openMultiImport();
        })->hide();

        menu->exec();
    }
    // 是歌单搜索结果
    else if (searchResultPlayLists.size())
    {
        PlayListList lists;
        foreach (auto item, items)
        {
            int row = ui->searchResultTable->row(item);
            int col = ui->searchResultTable->column(item);
            if (col != 0)
                continue;
            lists.append(searchResultPlayLists.at(row));
        }
        int row = ui->searchResultTable->currentRow();
        PlayList currentList;
        if (row > -1)
            currentList = searchResultPlayLists.at(row);

        FacileMenu* menu = new FacileMenu(this);

        menu->addAction("添加到我的歌单", [=]{

        });

        menu->exec();
    }
    else
    {

        FacileMenu* menu = new FacileMenu(this);
        menu->addAction("导入歌单链接", [=]{
            inputPlayList();
        });
        menu->addAction("批量导入歌曲", [=]{
            openMultiImport();
        })->hide();
        menu->exec();
    }
}

/**
 * 立即播放
 */
void OrderPlayerWindow::startPlaySong(Song song)
{
    if (isSongDownloaded(song))
    {
        playLocalSong(song);
    }
    else if (song.source == LocalMusic)
    {
        qWarning() << "本地文件不存在：" << song.name << songPath(song);
        QTimer::singleShot(0, [=]{
            playNext();
        });
    }
    else
    {
        playAfterDownloaded = song;
        toDownloadSongs.removeOne(song);
        downloadSong(song);
    }
}

void OrderPlayerWindow::playNext()
{
    if (!orderSongs.size()) // 播放列表全部结束
    {
        // 播放空闲列表
        playNextRandomSong();
        return ;
    }

    Song song = orderSongs.takeFirst();
    saveSongList("music/order", orderSongs);
    setSongModelToView(orderSongs, ui->orderSongsListView);

    startPlaySong(song);
    emit signalOrderSongPlayed(song);
}

/**
 * 添加到点歌队列（末尾）
 */
void OrderPlayerWindow::appendOrderSongs(SongList songs)
{
    QPoint center = ui->listTabWidget->tabBar()->tabRect(LISTTAB_ORDER).center();
    bool shallDownload = songs.size() < 10;
    foreach (Song song, songs)
    {
        if (orderSongs.contains(song))
            continue;
        orderSongs.append(song);
        if (shallDownload)
        {
            addDownloadSong(song);
            showTabAnimation(center, "+1");
        }
    }

    if (!shallDownload)
        showTabAnimation(center, "+" + QString::number(songs.size()));

    if (isNotPlaying() && orderSongs.size())
    {
        qInfo() << "当前未播放，开始播放列表";
        startPlaySong(orderSongs.takeFirst());
    }

    saveSongList("music/order", orderSongs);
    setSongModelToView(orderSongs, ui->orderSongsListView);

    downloadNext();
}

/**
 * 下一首播放（点歌队列置顶）
 */
void OrderPlayerWindow::appendNextSongs(SongList songs)
{
    QPoint center = ui->listTabWidget->tabBar()->tabRect(LISTTAB_ORDER).center();
    bool shallDownload = songs.size() < 10;
    foreach (Song song, songs)
    {
        if (orderSongs.contains(song))
            orderSongs.removeOne(song);
        orderSongs.insert(0, song);
        if (shallDownload)
        {
            addDownloadSong(song);
            showTabAnimation(center, "+1");
        }
    }

    if (!shallDownload)
        showTabAnimation(center, "+" + QString::number(songs.size()));

    // 一般不会自动播放，除非没有在放的歌
    /*if (isNotPlaying() && !playingSong.isValid() && songs.size())
    {
        qInfo() << "当前未播放，开始播放本首歌";
        startPlaySong(orderSongs.takeFirst());
    }*/

    saveSongList("music/order", orderSongs);
    setSongModelToView(orderSongs, ui->orderSongsListView);

    downloadNext();
}

/**
 * 立刻开始播放音乐
 */
void OrderPlayerWindow::playLocalSong(Song song)
{
    qInfo() << "开始播放：" << song.simpleString() << song.id << song.mid;
    if (!isSongDownloaded(song))
    {
        qWarning() << "error: 未下载歌曲" << song.simpleString() << "开始下载";
        playAfterDownloaded = song;
        downloadSong(song);
        return ;
    }

    // 设置信息
    auto max16 = [=](QString s){
        if (s.length() > 16)
            s = s.left(15) + "...";
        return s;
    };
    ui->playingNameLabel->setText(max16(song.name));
    ui->playingArtistLabel->setText(max16(song.artistNames));

    // 设置封面
    if (QFileInfo(coverPath(song)).exists())
    {
        QPixmap pixmap(coverPath(song), "1"); // 这里读取要加个参数，原因未知……
        if (pixmap.isNull())
            qWarning() << "warning: 本地封面是空的" << song.simpleString() << coverPath(song);
        else if (coveringSong != song)
        {
            pixmap = pixmap.scaledToHeight(ui->playingCoverLabel->height());
            ui->playingCoverLabel->setPixmap(pixmap);
            coveringSong = song;
            setCurrentCover(pixmap);
        }
    }
    else
    {
        downloadSongCover(song);
    }

    // 设置歌词
    if (QFileInfo(lyricPath(song)).exists())
    {
        QFile file(lyricPath(song));
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        QString lyric;
        QString line;
        while (!stream.atEnd())
        {
            line = stream.readLine();
            lyric.append(line+"\n");
        }
        file.close();

        setCurrentLyric(lyric);
    }
    else
    {
        setCurrentLyric("");
        downloadSongLyric(song);
    }

    // 开始播放
    playingSong = song;
    player->setMedia(QUrl::fromLocalFile(songPath(song)));
    player->setPosition(0);
    player->play();
    emit signalSongPlayStarted(song);
    setWindowTitle(song.name);

    // 添加到历史记录
    historySongs.removeOne(song);
    historySongs.insert(0, song);
    saveSongList("music/history", historySongs);
    setSongModelToView(historySongs, ui->historySongsListView);

    // 保存当前歌曲
    settings.setValue("music/currentSong", song.toJson());

    emit signalCurrentSongChanged(playingSong);
}

/**
 * 放入下载队列，准备下载（并不立即下载）
 */
void OrderPlayerWindow::addDownloadSong(Song song)
{
    if (isSongDownloaded(song) || toDownloadSongs.contains(song) || downloadingSong == song)
        return ;
    toDownloadSongs.append(song);
}

/**
 * 放入下载队列、或一首歌下载完毕，开始下载下一个
 */
void OrderPlayerWindow::downloadNext()
{
    if (downloadingSong.isValid() || !toDownloadSongs.size())
        return ;
    Song song = toDownloadSongs.takeFirst();
    if (!song.isValid())
        return downloadNext();
    downloadSong(song);
}

/**
 * 下载音乐
 */
void OrderPlayerWindow::downloadSong(Song song)
{
    if (isSongDownloaded(song))
        return ;
    downloadingSong = song;
    bool unblockQQMusic = this->unblockQQMusic; // 保存状态，避免下载的时候改变

    QString url;
    switch (song.source) {
    case UnknowMusic:
    case LocalMusic:
        return ;
    case NeteaseCloudMusic:
        url = NETEASE_SERVER + "/song/url?id=" + snum(song.id) + "&br=" + snum(songBr);
        break;
    case QQMusic:
        if (unblockQQMusic)
            url = "http://www.douqq.com/qqmusic/qqapi.php?mid=" + song.mid;
        else
            url = QQMUSIC_SERVER + "/song/url?id=" + song.mid;
        break;
    case MiguMusic:
        if (!song.url.isEmpty())
        {
            downloadSongMp3(song, song.url);
            return ;
        }
        url = MIGU_SERVER + "/song?cid=" + song.mid;
        break;
    case KugouMusic:
        QString hash = song.mid; // hash
        url = "https://wwwapi.kugou.com/yy/index.php?r=play/getdata&hash=" + song.mid + "&album_id=" + QString::number(song.album.id);
        break;
    }

    MUSIC_DEB << "获取歌曲下载链接信息：" << song.simpleString() << url;
    fetch(url, [=](QNetworkReply* reply){
        QByteArray baData = reply->readAll();

        if (song.source == QQMusic && unblockQQMusic)
        {
            // 这个API是野生找的，需要额外处理
            baData.replace("\\\"", "\"").replace("\\\\", "\\").replace("\\/", "/");
            if (baData.startsWith("\""))
                baData.replace(0, 1, "");
            if (baData.endsWith("\""))
                baData.replace(baData.length()-1, 1, "");
        }

        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(baData, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qWarning() << "解析歌曲信息出错" << error.errorString() << baData << url;
            return ;
        }

        QJsonObject json = document.object();
        QString fileUrl;
        switch (song.source) {
        case UnknowMusic:
        case LocalMusic:
            return ;
        case NeteaseCloudMusic:
        {
            if (json.value("code").toInt() != 200)
            {
                qWarning() << "网易云歌曲链接返回结果不为200：" << json.value("message").toString();
                return ;
            }

            QJsonArray array = json.value("data").toArray();
            if (!array.size())
            {
                qWarning() << "未找到歌曲：" << song.simpleString();
                downloadingSong = Song();
                downloadNext();
                return ;
            }
            json = array.first().toObject();
            fileUrl = JVAL_STR(url);
            int br = JVAL_INT(br); // 比率320000
            int size = JVAL_INT(size);
            QString type = JVAL_STR(type); // mp3
            QString encodeType = JVAL_STR(encodeType); // mp3
            MUSIC_DEB << "    网易云信息：" << br << size << type << fileUrl;
            if (size == 0)
            {
                downloadSongFailed(song);
                return ;
            }
            break;
        }
        case QQMusic:
        {
            if (unblockQQMusic) // 野生试听API
            {
                /*{
                    "mid": "001fApzM4Rymgq",
                    "m4a": "http:\/\/aqqmusic.tc.qq.com\/amobile.music.tc.qq.com\/C400001fApzM4Rymgq.m4a?guid=2095717240&vkey=23EB99E476D5EB7936AF440461D097689A00AE211B2D1725B9703A9459A1239301BF19E0A098B30CC9F4503C8DBDA65FD85E60E133944F13&uin=0&fromtag=38",
                    "mp3_l": "http:\/\/mobileoc.music.tc.qq.com\/M500001fApzM4Rymgq.mp3?guid=2095717240&vkey=23EB99E476D5EB7936AF440461D097689A00AE211B2D1725B9703A9459A1239301BF19E0A098B30CC9F4503C8DBDA65FD85E60E133944F13&uin=0&fromtag=53",
                    "mp3_h": "http:\/\/mobileoc.music.tc.qq.com\/M800001fApzM4Rymgq.mp3?guid=2095717240&vkey=23EB99E476D5EB7936AF440461D097689A00AE211B2D1725B9703A9459A1239301BF19E0A098B30CC9F4503C8DBDA65FD85E60E133944F13&uin=0&fromtag=53",
                    "ape": "http:\/\/mobileoc.music.tc.qq.com\/A000001fApzM4Rymgq.ape?guid=2095717240&vkey=23EB99E476D5EB7936AF440461D097689A00AE211B2D1725B9703A9459A1239301BF19E0A098B30CC9F4503C8DBDA65FD85E60E133944F13&uin=0&fromtag=53",
                    "flac": "http:\/\/mobileoc.music.tc.qq.com\/F000001fApzM4Rymgq.flac?guid=2095717240&vkey=23EB99E476D5EB7936AF440461D097689A00AE211B2D1725B9703A9459A1239301BF19E0A098B30CC9F4503C8DBDA65FD85E60E133944F13&uin=0&fromtag=53",
                    "songname": "\u6000\u74a7\u8d74\u524d\u5c18",
                    "albumname": "\u603b\u507f\u76f8\u601d",
                    "singername": "\u53f8\u590f",
                    "pic": "https:\/\/y.gtimg.cn\/music\/photo_new\/T002R300x300M0000022qglb0tOda5_1.jpg?max_age=2592000",
                    "mv": "\u6682\u65e0MV",
                    "lrc": "[ti:]\n[ar:]\n[al:]\n[by:\u65f6\u95f4\u6233\u5de5\u5177V2.0_20200505]\n[offset:0]\n[00:00.00]\u6000\u74a7\u8d74\u524d\u5c18\n[00:01.50]\u4f5c\u8bcd\uff1a\u6d41\u5149\n[00:03.01]\u4f5c\/\u7f16\u66f2\uff1a\u4e00\u53ea\u9c7c\u5361\n[00:04.51]\u6f14\u5531\uff1a\u53f8\u590f\n[00:06.02]\u4fee\u97f3\uff1a\u6c64\u5706w\n[00:07.53]\u6df7\u97f3\uff1aWuli\u5305\u5b50\n[00:09.03]\u6587\u6848\uff1a\u8c22\u77e5\u8a00\n[00:10.54]\u7f8e\u5de5\uff1a\u4f5c\u8086\n[00:12.04]\u5236\u4f5c\u4eba\uff1a\u6c90\u4e88\n[00:13.55]\u51fa\u54c1\uff1a\u7c73\u6f2b\u4f20\u5a92\n[00:15.06]\u5236\u4f5c\uff1a\u8d4f\u4e50\u8ba1\u5212\n[00:16.56]\u827a\u672f\u7edf\u7b79\uff1a\u697c\u5c0f\u6728\n[00:18.07]\u53d1\u884c\uff1a\u7396\u8bb8\n[00:19.59]\u9879\u76ee\u76d1\u7763\uff1a\u5b59\u534e\u5b81\n[00:27.51]\u522b\u6765\u5df2\u662f\u767e\u5e74\u8eab\n[00:33.39]\u788c\u788c\u7ea2\u5c18\u8001\u82b1\u6708\u60c5\u6839\n[00:40.08]\u9752\u9752\u9b02\u4ed8\u9752\u9752\u575f\n[00:43.56]\u76f8\u601d\u4e24\u5904\u5173\u5c71\u963b\u68a6\u9b42\n[00:52.77]\u541b\u5b50\u6380\u5e18\u773a\u6625\u6df1\n[00:58.71]\u6c49\u768b\u89e3\u4f69\u6000\u74a7\u9057\u4f73\u4eba\n[01:05.34]\u98de\u5149\u4e0d\u6765\u96ea\u76c8\u6a3d\n[01:11.28]\u4f73\u4eba\u53bb\u4e5f\u70df\u6708\u4ff1\u6c89\u6c89\n[01:15.15]\u5979\u6709\u51b0\u96ea\u9b42\u9b44\u4e03\u5e74\u9010\u70df\u6ce2\n[01:23.76]\u6f14\u4e00\u526f\u591a\u60c5\u8eab\u7ea2\u5c18\u91cc\u6d88\u78e8\n[01:30.30]\u6b4c\u6b7b\u751f\u5951\u9614\u5531\u4e0e\u5b50\u6210\u8bf4\n[01:36.15]\u5374\u5c06\u75f4\u5fc3\u4e00\u63e1\u6258\u4f53\u540c\u5c71\u963f\n[02:08.52]\u541b\u5b50\u6380\u5e18\u773a\u6625\u6df1\n[02:14.46]\u6c49\u768b\u89e3\u4f69\u6000\u74a7\u9057\u4f73\u4eba\n[02:21.12]\u98de\u5149\u4e0d\u6765\u96ea\u76c8\u6a3d\n[02:27.03]\u4f73\u4eba\u53bb\u4e5f\u70df\u6708\u4ff1\u6c89\u6c89\n[02:32.67]\u5979\u6709\u51b0\u96ea\u9b42\u9b44\u4e03\u5e74\u9010\u70df\u6ce2\n[02:39.00]\u6f14\u4e00\u526f\u591a\u60c5\u8eab\u7ea2\u5c18\u91cc\u6d88\u78e8\n[02:46.08]\u6b4c\u6b7b\u751f\u5951\u9614\u5531\u4e0e\u5b50\u6210\u8bf4\n[02:51.99]\u5374\u5c06\u75f4\u5fc3\u4e00\u63e1\u6258\u4f53\u540c\u5c71\u963f\n[02:56.13]\u5979\u6709\u51b0\u96ea\u9b42\u9b44\u4e03\u5e74\u9010\u70df\u6ce2\n[03:05.04]\u6f14\u4e00\u526f\u591a\u60c5\u8eab\u7ea2\u5c18\u91cc\u6d88\u78e8\n[03:11.37]\u6b4c\u6b7b\u751f\u5951\u9614\u5531\u4e0e\u5b50\u6210\u8bf4\n[03:17.22]\u5374\u5c06\u75f4\u5fc3\u4e00\u63e1\u6258\u4f53\u540c\u5c71\u963f\n[04:00.00]"
                }*/
                QString m4a = json.value("m4a").toString(); // 视频？但好像能直接播放
                QString mp3_l = json.value("mp3_l").toString(); // 普通品质
                QString mp3_h = json.value("mp3_h").toString(); // 高品质
                QString ape = json.value("ape").toString(); // 高品无损
                QString flac = json.value("flac").toString(); // 无损音频

                // 实测只有 m4a 能播放……
                if (!m4a.isEmpty())
                    fileUrl = m4a;
                else if (!mp3_h.isEmpty())
                    fileUrl = mp3_h;
                else if (!mp3_l.isEmpty())
                    fileUrl = mp3_l;
                else if (!ape.isEmpty())
                    fileUrl = ape;
                else if (!flac.isEmpty())
                    fileUrl = flac;
            }
            else // QQMUSIC
            {
                if (json.value("result").toInt() != 100)
                {
                    qWarning() << "QQ歌曲链接返回结果不为100：" << json;
                    switchSource(song);
                    return ;
                }

                fileUrl = json.value("data").toString();
            }

            if (fileUrl.isEmpty())
            {
                downloadSongFailed(song);
                return ;
            }

            break;
        }
        case MiguMusic:
            if (json.value("result").toInt() != 100)
            {
                qWarning() << "咪咕歌曲链接返回结果不为100：" << json;
                switchSource(song);
                return ;
            }
            fileUrl = json.value("data").toObject().value("url").toString();
            if (fileUrl.isEmpty())
            {
                downloadSongFailed(song);
                return ;
            }
            break;
        case KugouMusic:
            if (json.value("err_code").toInt() != 0)
            {
                qWarning() << "酷狗歌曲链接返回结果不为0：" << json;
                switchSource(song);
                return ;
            }
            fileUrl = json.value("data").toObject().value("play_url").toString();
            fileUrl.replace("https://", "http://");
            if (fileUrl.isEmpty())
            {
                downloadSongFailed(song);
                return ;
            }
            break;
        }
        MUSIC_DEB << "歌曲下载直链：" << fileUrl;

        downloadSongMp3(song, fileUrl);
    }, song.source);

    downloadSongLyric(song);
    downloadSongCover(song);
}

void OrderPlayerWindow::downloadSongFailed(Song song)
{
    if (!song.addBy.isEmpty())
    {
        emit signalOrderSongNoCopyright(song);
    }
    qWarning() << "无法下载，可能没有版权" << song.simpleString();
    if (playAfterDownloaded == song)
    {
        if (orderSongs.contains(song))
        {
            orderSongs.removeOne(song);
            saveSongList("music/order", orderSongs);
            setSongModelToView(orderSongs, ui->orderSongsListView);
        }

        if (autoSwitchSource /* && song.source == musicSource */
                /*&& !song.addBy.isEmpty()*/) // 只有点歌才自动换源，普通播放自动跳过
        {
            slotSongPlayEnd(); // 先停止播放，然后才会开始播放新的；否则会插入到下一首
            insertOrderOnce = true;
            if (!switchSource(song, true))
                playNext();
        }
        else
        {
            playNext();
        }
    }

    downloadingSong = Song();
    downloadNext();
}

void OrderPlayerWindow::downloadSongMp3(Song song, QString url)
{
    fetch(url, [=](QNetworkReply* reply){
        QByteArray mp3Ba = reply->readAll();
        if (mp3Ba.isEmpty())
        {
            qWarning() << "无法下载歌曲，可能缺少版权：" << song.simpleString() << song.id << song.mid << song.source;
            downloadingSong = Song();
            downloadNext();
            return ;
        }

        // 保存到文件
        QFile file(songPath(song));
        file.open(QIODevice::WriteOnly);
        file.write(mp3Ba);
        file.flush();
        file.close();

        emit signalSongDownloadFinished(song);
        MUSIC_DEB << "歌曲mp3下载完成：" << song.simpleString();

        if (playAfterDownloaded == song)
            playLocalSong(song);

        downloadingSong = Song();
        downloadNext();
    });
}

void OrderPlayerWindow::downloadSongLyric(Song song)
{
    if (QFileInfo(lyricPath(song)).exists())
        return ;

    QString url;
    switch (song.source) {
    case UnknowMusic:
    case LocalMusic:
        return ;
    case NeteaseCloudMusic:
        url = NETEASE_SERVER + "/lyric?id=" + snum(song.id);
        break;
    case QQMusic:
        url = QQMUSIC_SERVER + "/lyric?songmid=" + song.mid;
        break;
    case MiguMusic:
        url = MIGU_SERVER + "/song?cid=" + song.mid;
        break;
    case KugouMusic:
        url = "https://m.kugou.com/app/i/krc.php?cmd=100&hash=" + song.mid + "&timelength=1";
        break;
    }
    MUSIC_DEB << "歌词信息链接：" << url;

    fetch(url, [=](QNetworkReply *reply){
        QByteArray ba = reply->readAll();
        QJsonObject json;
        QString lrc;

        // 酷狗音乐直接返回歌词文本，不需要解析JSON
        if (song.source != KugouMusic)
        {
            QJsonParseError error;
            QJsonDocument document = QJsonDocument::fromJson(ba, &error);
            if (error.error != QJsonParseError::NoError)
            {
                qWarning() << "解析歌词JSON出错：" << error.errorString() << url;
                return ;
            }
            json = document.object();
        }

        switch (song.source) {
        case UnknowMusic:
        case LocalMusic:
            return ;
        case NeteaseCloudMusic:
            if (json.value("code").toInt() != 200)
            {
                qWarning() << "网易云歌词返回结果不为200：" << json;
                return ;
            }
            lrc = json.value("lrc").toObject().value("lyric").toString();
            break;
        case QQMusic:
            lrc = json.value("data").toObject().value("lyric").toString();
            break;
        case MiguMusic:
            if (json.value("result").toInt() != 100)
            {
                qWarning() << "咪咕歌词返回结果不为100：" << json.value("result").toInt();
                return ;
            }
            lrc = json.value("data").toObject().value("lyric").toString();

            // 如果没有自带封面，则在歌词这里同步下载
            if (song.album.picUrl.isEmpty())
            {
                QString picUrl = json.value("data").toObject().value("picUrl").toString();
                MUSIC_DEB << "咪咕同步下载封面" << picUrl;
                downloadSongCoverJpg(song, picUrl);
            }
            break;
        case KugouMusic:
            lrc = ba;
            break;
        }
        if (!lrc.isEmpty())
        {
            QFile file(lyricPath(song));
            file.open(QIODevice::WriteOnly);
            QTextStream stream(&file);
            stream.setCodec("UTF-8");
            stream << lrc;
            file.flush();
            file.close();

            MUSIC_DEB << "下载歌词完成：" << song.simpleString();
            if (playAfterDownloaded == song || playingSong == song)
            {
                setCurrentLyric(lrc);
            }

            emit signalLyricDownloadFinished(song);
        }
        else
        {
            qWarning() << "warning: 下载的歌词是空的" << song.simpleString() << url;
        }
    }, song.source);
}

void OrderPlayerWindow::downloadSongCover(Song song)
{
    if (QFileInfo(coverPath(song)).exists())
        return ;

    QString url;
    switch (song.source) {
    case UnknowMusic:
    case LocalMusic:
        return ;
    case NeteaseCloudMusic:
        url = NETEASE_SERVER + "/song/detail?ids=" + snum(song.id);
        break;
    case QQMusic:
        url = "https://y.gtimg.cn/music/photo_new/T002R300x300M000" + song.album.mid + ".jpg";
        MUSIC_DEB << "QQ专辑封面信息url:" << url;
        downloadSongCoverJpg(song, url);
        return ;
    case MiguMusic:
        if (!song.album.picUrl.isEmpty())
        {
            MUSIC_DEB << "咪咕专辑封面信息url：" << song.album.picUrl;
            downloadSongCoverJpg(song, song.album.picUrl);
            return ;
        }
        // url = MIGU_SERVER + "/song?cid=" + song.mid; // 在下载歌词的时候同步下载封面
        return ;
    case KugouMusic:
        // url = "http://mobilecdnbj.kugou.com/api/v3/album/info?albumid=" + QString::number(song.album.id);
        url = "https://www.kugou.com/yy/index.php?r=play/getdata&hash=" + song.mid;
        break;
    }

    MUSIC_DEB << "封面信息url:" << url;
    fetch(url, [=](QJsonObject json){
        QString url;
        switch (song.source) {
        case UnknowMusic:
        case LocalMusic:
            return ;
        case NeteaseCloudMusic:
        {
            if (json.value("code").toInt() != 200)
            {
                qWarning() << ("网易云封面返回结果不为200：") << json.value("message").toString();
                return ;
            }
            QJsonArray array = json.value("songs").toArray();
            if (!array.size())
            {
                qWarning() << "未找到歌曲：" << song.simpleString();
                return ;
            }

            json = array.first().toObject();
            url = json.value("al").toObject().value("picUrl").toString();
            break;
        }
        case QQMusic:
        case MiguMusic:
            return ;
        case KugouMusic:
        {
            if (json.value("err_code").toInt() != 0)
            {
                qWarning() << ("酷狗封面返回结果不为0：") << json.value("error").toString();
                return ;
            }
            // errcode=0, 返回结果：http://imge.kugou.com/stdmusic/{size}/20160907/20160907182908304289.jpg
            // url = json.value("data").toObject().value("imgurl").toString();
            // url.replace("{size}", "100");

            // err_code=0, 返回结果：http://imge.kugou.com/stdmusic/20200909/20200909133708480475.jpg
            url = json.value("data").toObject().value("img").toString();
            break;
        }
        }

        MUSIC_DEB << "封面地址：" << url;

        downloadSongCoverJpg(song, url);
    }, song.source);
}

void OrderPlayerWindow::downloadSongCoverJpg(Song song, QString url)
{
    fetch(url, [=](QNetworkReply* reply){
        QByteArray baData1 = reply->readAll();
        QPixmap pixmap;
        pixmap.loadFromData(baData1);
        if (!pixmap.isNull())
        {
            QFile file(coverPath(song));
            file.open(QIODevice::WriteOnly);
            file.write(baData1);
            file.flush();
            file.close();

            MUSIC_DEB << "封面下载完成:" << pixmap.size() << "   "
                      << (playAfterDownloaded == song) << (playingSong == song)
                      << (coveringSong == song);
            emit signalCoverDownloadFinished(song);

            // 正是当前要播放的歌曲
            if (playAfterDownloaded == song || playingSong == song)
            {
                if (coveringSong != song)
                {
                    pixmap = pixmap.scaledToHeight(ui->playingCoverLabel->height());
                    ui->playingCoverLabel->setPixmap(pixmap);
                    coveringSong = song;
                    setCurrentCover(pixmap);
                }
            }
        }
        else
        {
            qWarning() << "warning: 下载的封面是空的" << song.simpleString() << url;
        }
    });
}

/**
 * 设置当前歌曲的歌词到歌词控件
 */
void OrderPlayerWindow::setCurrentLyric(QString lyric)
{
    desktopLyric->setLyric(lyric);
    ui->lyricWidget->setLyric(lyric);
    emit signalLyricChanged();
}

void OrderPlayerWindow::inputPlayList()
{
    QString def = "";
    QString clip = QApplication::clipboard()->text();
    if (clip.startsWith("http"))
        def = clip;
    QString url = QInputDialog::getText(this, "查看歌单", "支持网易云音乐、QQ音乐的分享链接", QLineEdit::Normal, def);
    if (url.isEmpty())
        return ;
    openPlayList(url);
}

void OrderPlayerWindow::openPlayList(QString shareUrl)
{
    MusicSource source = UnknowMusic;
    QString playlistUrl;
    QString id;
    qInfo() << "歌单URL：" << shareUrl;
    shareUrl = shareUrl.trimmed();
    shareUrl.replace(QRegularExpression("#.?"), ""); // 去掉页面标签
    if (shareUrl.contains("music.163.com")) // 网易云音乐的分享
    {
        // http://music.163.com/playlist?id=425710029&userid=306646638
        // https://music.163.com/#/playlist?id=5035490932&userid=330951134
        // http://music.163.com/playlist/5035490932/330951134/?userid=330951134
        source = NeteaseCloudMusic;
        QRegularExpressionMatch match;
        if (shareUrl.indexOf(QRegularExpression("\\bid=(\\d+)"), 0, &match) > -1
                || shareUrl.indexOf(QRegularExpression("playlist/(\\d+)"), 0, &match) > -1)
        {
            id = match.captured(1);
            playlistUrl = NETEASE_SERVER + "/playlist/detail?id=" + id;
        }
        else
        {
            qWarning() << "无法解析歌单链接：" << shareUrl;
            QMessageBox::warning(this, "打开歌单", "无法解析的歌单地址");
            return ;
        }
    }
    else if (shareUrl.contains("c.y.qq.com/base/fcgi-bin/u?__=")) // QQ音乐压缩
    {
        // https://c.y.qq.com/base/fcgi-bin/u?__=HdLP6Kp
        if (!shareUrl.startsWith("http"))
            shareUrl = "https://" + shareUrl;

        // 检测重定向
        fetch(shareUrl, [=](QNetworkReply* reply){
            QString url = reply->rawHeader("Location");
            return openPlayList(url);
        }, QQMusic);
        return ;
    }
    else if (shareUrl.contains("qq.com") && shareUrl.contains(QRegularExpression("[\\?&]id=(\\d+)"))) // QQ音乐短网址第一次重定向
    {
        // https://y.qq.com/w/taoge.html?ADTAG=erweimashare&channelId=10036163&id=7845417918&openinqqmusic=1
        source = QQMusic;
        QRegularExpression re("[\\?&]id=(\\d+)");
        QRegularExpressionMatch match;
        if (shareUrl.indexOf(re, 0, &match) == -1)
        {
            qWarning() << "无法解析的网易云音乐歌单链接：" << shareUrl;
            QMessageBox::warning(this, "打开歌单", "无法解析的网易云音乐歌单地址");
            return ;
        }
        id = match.captured(1);
        playlistUrl = QQMUSIC_SERVER + "/songlist?id=" + id;
    }
    else if (shareUrl.contains("y.qq.com") && shareUrl.contains("playlist/")) // QQ音乐短网址第二次重定向（网页打开的）
    {
        // https://y.qq.com/n/yqq/playlist/7845417918.html
        // https://y.qq.com/n/ryqq/playlist/7845417918.html
        source = QQMusic;
        QRegularExpressionMatch match;
        if (shareUrl.indexOf(QRegularExpression("playlist/(\\d+)"), 0, &match) > -1)
        {
            id = match.captured(1);
            playlistUrl = QQMUSIC_SERVER + "/songlist?id=" + id;
        }
        else
        {
            qWarning() << "无法解析的QQ音乐歌单链接：" << shareUrl;
            QMessageBox::warning(this, "打开歌单", "无法解析的QQ音乐歌单地址");
            return ;
        }
    }
    else if (shareUrl.contains("music.migu.cn"))
    {
        // https://music.migu.cn/v3/music/playlist/172881646?origin=1001001
        source = MiguMusic;
        QRegularExpressionMatch match;
        if (shareUrl.indexOf(QRegularExpression("playlist/(\\d+)"), 0, &match) > -1)
        {
            id = match.captured(1);
            playlistUrl = MIGU_SERVER + "/playlist?id=" + id;
        }
        else
        {
            qWarning() << "无法解析的咪咕音乐歌单链接：" << shareUrl;
            QMessageBox::warning(this, "打开歌单", "无法解析的咪咕音乐歌单地址");
            return ;
        }
    }
    else if (shareUrl.contains("kugou.com"))
    {
        // 精选辑：https://www.kugou.com/yy/special/single/3339907.html
        // 分享的：https://www.kugou.com/share/zlist.html?listid=20&type=0&uid=57714277&sign=11995f9016bb8c5a7e6b4f532d0b6954..........&xxx=xxx#hash=9F982A36638B21D884B25A11C8AAF135
        source = KugouMusic;
        QRegularExpressionMatch match;
        if (shareUrl.indexOf(QRegularExpression("special/single/(\\d+)"), 0, &match) > -1)
        {
            id = match.captured(1);
            playlistUrl = "https://m.kugou.com/plist/list/" + id + "?json=true";
        }
        else if (shareUrl.contains("share/zlist.html"))
        {
            // https://m.kugou.com/zlist/list?listid=25&type=0&uid=25230245&sign=c0c7fe2caee03445b51bf8961308174b&_t=1528116907&pagesize=30&json=&page=1&token=2b631b0f122f5dc5c0ca758374c4d190eb8bde2ea382bf7a29bacfd5bb439489
            playlistUrl = shareUrl;
            playlistUrl.replace("share/zlist.html", "zlist/list")
                    .replace("www.kugou.com", "m.kugou.com");
            playlistUrl.append("&pagesize=100"); // 调整到一页最大
        }
        else
        {
            qWarning() << "无法解析的酷狗音乐歌单链接：" << shareUrl;
            QMessageBox::warning(this, "打开歌单", "无法解析的酷狗音乐歌单地址");
            return ;
        }
    }
    else
    {
        qWarning() << "无法解析的歌单链接：" << shareUrl;
        QMessageBox::warning(this, "打开歌单", "无法解析的歌单地址");
        return ;
    }

    MUSIC_DEB << "歌单接口：" << playlistUrl;
    fetch(playlistUrl, [=](QJsonObject json){
        SongList songs;
        switch (source) {
        case UnknowMusic:
        case LocalMusic:
            return ;
        case NeteaseCloudMusic:
        {
            if (json.value("code").toInt() != 200)
            {
                qWarning() << ("歌单返回结果不为200：") << json.value("message").toString();
                return ;
            }
            // QJsonArray array = json.value("playlist").toObject().value("tracks").toArray(); // tracks是不完整的，需要使用 trackIds
            // 获取 trackIds
            QJsonObject playlistObj = json.value("playlist").toObject();
            QString name = playlistObj.value("name").toString();
            ui->searchEdit->setText(name);
            QJsonArray idsArray = playlistObj.value("trackIds").toArray();
            QStringList ids;
            foreach (QJsonValue val, idsArray)
            {
                ids << QString::number(static_cast<qint64>(val.toObject().value("id").toDouble()));
            }
            qInfo() << "歌单：" << name << "   数量：" << ids.size();

            // 通过Id获取歌曲
            fetch(NETEASE_SERVER + "/song/detail?timestamp=" + snum(QDateTime::currentSecsSinceEpoch()),
                  QStringList{"ids", ids.join(",")},
                  [=](QJsonObject json) {
                QJsonArray array = json.value("songs").toArray();
                searchResultSongs.clear();
                foreach (QJsonValue val, array)
                {
                    searchResultSongs << Song::fromNeteaseShareJson(val.toObject());
                }
                setSearchResultTable(searchResultSongs);
                ui->bodyStackWidget->setCurrentWidget(ui->searchResultPage);
            }, source);
            break;
        }
        case QQMusic:
        {
            if (json.value("result").toInt() != 100)
            {
                qWarning() << "QQ歌单返回结果不为100：" << json;
                return ;
            }
            QJsonArray array = json.value("data").toObject().value("songlist").toArray();
            searchResultSongs.clear();
            foreach (QJsonValue val, array)
            {
                searchResultSongs << Song::fromQQMusicJson(val.toObject());
            }
            setSearchResultTable(searchResultSongs);
            ui->bodyStackWidget->setCurrentWidget(ui->searchResultPage);
            break;
        }
        case MiguMusic:
        {
            if (json.value("result").toInt() != 100)
            {
                qWarning() << "咪咕歌单返回结果不为100：" << json;
                return ;
            }
            QJsonArray array = json.value("data").toObject().value("list").toArray();
            searchResultSongs.clear();
            foreach (QJsonValue val, array)
            {
                searchResultSongs << Song::fromMiguMusicJson(val.toObject());
            }
            setSearchResultTable(searchResultSongs);
            ui->bodyStackWidget->setCurrentWidget(ui->searchResultPage);
            break;
        }
        case KugouMusic:
        {
            if (json.value("err_code").toInt() != 0)
            {
                qWarning() << "酷狗歌单返回结果不为0：" << json;
                return ;
            }
            auto list = json.value("list").toObject();
            if (list.contains("list"))
                list = json.value("list").toObject();
            QJsonArray array = list.value("info").toArray();
            searchResultSongs.clear();
            foreach (QJsonValue val, array)
            {
                searchResultSongs << Song::fromKugouShareJson(val.toObject());
            }

            int count = list.value("count").toInt(); // 总数量
            int pageSize = list.value("pagesize").toInt(); // 一页数量
            int page = list.value("page").toInt(); // 当前页
            if (count > 0 && pageSize > 0) // 继续加载后面几页
            {
                int pageCount = (count + pageSize - 1) / pageSize;
                if (page < pageCount)
                {
                    for (int i = page + 1; i <= pageCount; i++)
                    {
                        qInfo() << "加载歌单页：" << i << "/" << pageCount;
                        auto source = NetUtil::getWebData(playlistUrl + "&page=" + QString::number(i));
                        MyJson json(source);
                        auto list = json.o("list");
                        if (list.contains("list"))
                            list = list.o("list");
                        list.each("info", [=](MyJson j) {
                            searchResultSongs << Song::fromKugouShareJson(j);
                        });
                    }
                }
            }

            setSearchResultTable(searchResultSongs);
            ui->bodyStackWidget->setCurrentWidget(ui->searchResultPage);
            break;
        }
        }
    }, source);

}

void OrderPlayerWindow::openMultiImport()
{
    if (importingSongNames.size())
    {
        QMessageBox::information(this, "批量导入歌名/本地音乐文件", "当前剩余导入数量：" + snum(importingSongNames.size()) + "\n请等待导入结束");
        return ;
    }

    ImportSongsDialog* isd = new ImportSongsDialog(&settings, this);
    isd->show();
    connect(isd, &ImportSongsDialog::importMusics, this, [=](int format, const QStringList& lines) {
        this->importFormat = format;
        importSongs(lines);
    });
}

void OrderPlayerWindow::clearDownloadFiles()
{
    QList<QFileInfo> files = musicsFileDir.entryInfoList(QDir::Files);
    foreach (QFileInfo info, files)
    {
        QFile f;
        f.remove(info.absoluteFilePath());
    }
}

void OrderPlayerWindow::clearHoaryFiles()
{
    qint64 current = QDateTime::currentSecsSinceEpoch();
    QList<QFileInfo> files = musicsFileDir.entryInfoList(QDir::Files);
    foreach (QFileInfo info, files)
    {
        if (info.lastModified().toSecsSinceEpoch() + 604800 < current) // 七天前的
        {
            QFile f;
            f.remove(info.absoluteFilePath());
        }
    }
}

void OrderPlayerWindow::playNextRandomSong()
{
    if (!normalSongs.size()) // 空闲列表没有歌曲
        return ;

    // int r = qrand() % normalSongs.size();
    // startPlaySong(normalSongs.at(r));

    if (!randomSongList.size())
        generalRandomSongList();

    startPlaySong(randomSongList.takeFirst());
}

/**
 * 洗牌算法：https://blog.csdn.net/zhuhengv/article/details/50496576
 */
void OrderPlayerWindow::generalRandomSongList()
{
    // 流派不好搞，直接按名字来吧
    QHash<QString, QList<Song>> shuffles;
    for (int i = 0; i < normalSongs.size(); i++)
    {
        Song song = normalSongs.at(i);
        QString name = song.artistNames;
        if (shuffles.contains(name))
            shuffles[name].append(song);
        else
            shuffles.insert(name, QList<Song>{song});
    }

    // 取最大数量
    int maxLen = 0;
    foreach (const QList<Song>& songs, shuffles)
    {
        maxLen = qMax(maxLen, songs.size());
    }

    // 随机打散
    for (auto it = shuffles.begin(); it != shuffles.end(); it++)
    {
        SongList& songs = it.value();
        unsigned seed = unsigned(std::chrono::system_clock::now ().time_since_epoch ().count());
        std::shuffle(songs.begin(), songs.end(), std::default_random_engine (seed));

        // 插入空的Song，使每一层尽量分布均匀
        int delta = maxLen - songs.size();
        while (delta--)
        {
            songs.insert(qrand() % (songs.size() + 1), Song());
        }
    }

    // 随机取样
    randomSongList.clear();
    for (int i = 0; i < maxLen; i++)
    {
        for (auto it = shuffles.begin(); it != shuffles.end(); it++)
        {
            const Song& song = it.value().at(i);
            if (!song.isValid())
                continue;
            randomSongList.append(song);
        }
    }
}

void OrderPlayerWindow::adjustExpandPlayingButton()
{
    QRect rect(ui->playingCoverLabel->mapTo(this, QPoint(0,0)), QSize(ui->listTabWidget->width(), ui->playingCoverLabel->height()));
    expandPlayingButton->setGeometry(rect);
    expandPlayingButton->raise();
}

void OrderPlayerWindow::connectDesktopLyricSignals()
{
    connect(desktopLyric, &DesktopLyricWidget::signalhide, this, [=]{
        ui->desktopLyricButton->setIcon(QIcon(":/icons/lyric_hide"));
        settings.setValue("music/desktopLyric", false);
    });
    connect(desktopLyric, &DesktopLyricWidget::signalSwitchTrans, this, [=]{
        desktopLyric->close();
        desktopLyric->deleteLater();

        desktopLyric = new DesktopLyricWidget(settings, nullptr);
        connectDesktopLyricSignals();
        desktopLyric->show();

        if (playingSong.isValid())
        {
            Song song = playingSong;
            if (QFileInfo(lyricPath(song)).exists())
            {
                QFile file(lyricPath(song));
                file.open(QIODevice::ReadOnly | QIODevice::Text);
                QTextStream stream(&file);
                stream.setCodec("UTF-8");
                QString lyric;
                QString line;
                while (!stream.atEnd())
                {
                    line = stream.readLine();
                    lyric.append(line+"\n");
                }
                file.close();

                desktopLyric->setLyric(lyric);
                desktopLyric->setPosition(player->position());
            }
        }
    });
}

void OrderPlayerWindow::setCurrentCover(const QPixmap &pixmap)
{
    currentCover = pixmap;
    // 设置歌曲
    int side = qMin(pixmap.width(), pixmap.height());
    QPixmap appFace = QPixmap(side, side);
    appFace.fill(Qt::transparent);
    QPainter painter(&appFace);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    QPainterPath path;
    path.addEllipse(0, 0, side, side);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, side, side, pixmap);
    setWindowIcon(appFace);

    // 设置主题
    if (themeColor)
        setThemeColor(currentCover);
    if (blurBg)
        setBlurBackground(currentCover);
}

void OrderPlayerWindow::setBlurBackground(const QPixmap &bg)
{
    if (bg.isNull())
        return ;

    // 当前图片变为上一张图
    prevBgAlpha = currentBgAlpha;
    prevBlurBg = currentBlurBg;

    // 开始模糊
    const int radius = qMax(20, qMin(width(), height())/5);
    QPixmap pixmap = bg;
    pixmap = pixmap.scaled(this->width()+radius*2, this->height() + radius*2, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage img = pixmap.toImage(); // img -blur-> painter(pixmap)
    QPainter painter( &pixmap );
    qt_blurImage( &painter, img, radius, true, false );

    // 裁剪掉边缘（模糊后会有黑边）
    int c = qMin(bg.width(), bg.height());
    c = qMin(c/2, radius);
    QPixmap clip = pixmap.copy(c, c, pixmap.width()-c*2, pixmap.height()-c*2);

    // 抽样获取背景，设置之后的透明度
    qint64 rgbSum = 0;
    QImage image = clip.toImage();
    int w = image.width(), h = image.height();
    const int m = 16;
    for (int y = 0; y < m; y++)
    {
        for (int x = 0; x < m; x++)
        {
            QColor c = image.pixelColor(w*x/m, h*x/m);
            rgbSum += c.red() + c.green() + c.blue();
        }
    }
    int addin = int(rgbSum * blurAlpha / (255*3*m*m));

    // 半透明
    currentBlurBg = clip;
    currentBgAlpha = qMin(255, blurAlpha + addin);

    startBgAnimation();
}

void OrderPlayerWindow::startBgAnimation(int duration)
{
    // 出现动画
    QPropertyAnimation* ani1 = new QPropertyAnimation(this, "appearBgProg");
    ani1->setStartValue(0);
    ani1->setEndValue(currentBgAlpha);
    ani1->setDuration(duration);
    ani1->setEasingCurve(QEasingCurve::OutCubic);
    connect(ani1, &QPropertyAnimation::finished, this, [=]{
        ani1->deleteLater();
    });
    currentBgAlpha = 0;
    ani1->start();

    // 消失动画
    QPropertyAnimation* ani2 = new QPropertyAnimation(this, "disappearBgProg");
    ani2->setStartValue(prevBgAlpha);
    ani2->setEndValue(0);
    ani2->setDuration(duration);
    ani2->setEasingCurve(QEasingCurve::OutQuad);
    connect(ani2, &QPropertyAnimation::finished, this, [=]{
        prevBlurBg = QPixmap();
        ani2->deleteLater();
        update();
    });
    ani2->start();
}

void OrderPlayerWindow::setThemeColor(const QPixmap &cover)
{
    QColor bg, fg, sbg, sfg;
    auto colors = ImageUtil::extractImageThemeColors(cover.toImage(), 7);
    ImageUtil::getBgFgSgColor(colors, &bg, &fg, &sbg, &sfg);

    prevPa = BFSColor::fromPalette(palette());
    currentPa = BFSColor(QList<QColor>{bg, fg,sbg, sfg});

    QPropertyAnimation* ani = new QPropertyAnimation(this, "paletteProg");
    ani->setStartValue(0);
    ani->setEndValue(1.0);
    ani->setDuration(500);
    connect(ani, &QPropertyAnimation::valueChanged, this, [=](const QVariant& val){
        double d = val.toDouble();
        BFSColor bfs = prevPa + (currentPa - prevPa) * d;
        QColor bg, fg, sbg, sfg;
        bfs.toColors(&bg, &fg, &sbg, &sfg);

        QPalette pa;
        pa.setColor(QPalette::Window, bg);
        pa.setColor(QPalette::Background, bg);
        pa.setColor(QPalette::Button, bg);

        pa.setColor(QPalette::Foreground, fg);
        pa.setColor(QPalette::Text, fg);
        pa.setColor(QPalette::ButtonText, fg);
        pa.setColor(QPalette::WindowText, fg);

        pa.setColor(QPalette::Highlight, sbg);
        pa.setColor(QPalette::HighlightedText, sfg);

        this->setPalette(pa);
        ui->orderSongsListView->setPalette(pa);
        ui->normalSongsListView->setPalette(pa);
        ui->favoriteSongsListView->setPalette(pa);
        ui->listSongsListView->setPalette(pa);
        ui->historySongsListView->setPalette(pa);
        ui->searchResultTable->setPalette(pa);
        ui->searchResultTable->horizontalHeader()->setPalette(pa);

        ui->lyricWidget->setColors(sfg, fg);
        desktopLyric->setColors(sfg, fg);
        ui->playingNameLabel->setPalette(pa);
        ui->titleButton->setPalette(pa);
        ui->titleButton->setTextColor(fg);
    });
    connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
    ani->start();

    // 菜单直接切换，不进行动画
    QColor halfSg = sfg;
    halfSg.setAlpha(halfSg.alpha() / 2);
    FacileMenu::normal_bg = bg;
    FacileMenu::hover_bg = halfSg;
    FacileMenu::press_bg = sfg;
    FacileMenu::text_fg = fg;

    MUSIC_DEB << "当前颜色：" << bg << fg << sfg;
}

/**
 * 参考链接：https://blog.csdn.net/weixin_37608233/article/details/82930197
 * 仅供测试
 */
void OrderPlayerWindow::readMp3Data(const QByteArray &array)
{
    // ID3V2 标签头
    std::string header = array.mid(0, 3).toStdString(); // [3] 必须为ID3
    char ver = *array.mid(3, 1).data(); // [1] 版本号03=v2.3, 04=v2.4
    char revision = *array.mid(4, 1).data(); // [1] 副版本号
    char flag = *array.mid(5, 1).data(); // [1] 标志
    char* sizes = array.mid(6, 4).data(); // [4] 标签大小，包括标签帧和标签头（不包括扩展标签头的10字节）
    int size = (sizes[0]<<24)+(sizes[1]<<16)+(sizes[2]<<8)+sizes[3];
    qInfo() << QString::fromStdString(header) << size;
    Q_UNUSED(ver)
    Q_UNUSED(revision)
    Q_UNUSED(flag)
    Q_UNUSED(sizes)

    QHash<QString, QString>frameIds;
    frameIds.insert("TIT2", "标题");
    frameIds.insert("TPE1", "作者");
    frameIds.insert("TALB", "专辑");
    frameIds.insert("TRCK", "音轨"); // 直接数字 // ???格式：N/M 其中N为专集中的第N首，M为专集中共M首，N和M为ASCII码表示的数字
    frameIds.insert("TYER", "年代"); // 用ASCII码表示的数字
    frameIds.insert("TCON", "类型"); // 直接用字符串表示
    frameIds.insert("COMM", "备注"); // 格式："eng\0备注内容"，其中eng表示备注所使用的自然语言
    frameIds.insert("APIC", "专辑图片"); // png文件标志的前两个字节为89 50；jpeg文件标志的前两个字节为FF,D8；
    frameIds.insert("TSSE", "专辑图片"); // Lavf56.4.101

    // 标签帧
    int pos = 10; // 标签头10字节
    while (pos < size)
    {
        std::string frameId = array.mid(pos, 4).toStdString(); // [4] 帧标志
        char* sizes = array.mid(pos+4, 4).data(); // [4] 帧内容大小（不包括帧头）
        char* frameFlag = array.mid(pos+8, 2).data(); // [2] 存放标志
        int frameSize = (sizes[0]<<24)+(sizes[1]<<16)+(sizes[2]<<8)+sizes[3];
        Q_UNUSED(frameFlag)
        qDebug() << "pos =" << pos << "    id =" << QString::fromStdString(frameId) << "   size =" << frameSize;
        pos += 10; // 帧标签10字节
        if (frameIds.contains(QString::fromStdString(frameId)))
        {
            QByteArray ba;
            if (*array.mid(pos, 1).data() == 0 && *array.mid(pos+frameSize-1, 1).data()==0)
                ba = array.mid(pos+1, frameSize-2);
            else
                ba = array.mid(pos, frameSize);
            qDebug() << frameIds.value(QString::fromStdString(frameId)) << ba;
        }
        else if (frameSize < 1000)
            qDebug() << array.mid(pos, frameSize);
        else
            qDebug() << array.mid(pos, 100) << "...";
        pos += frameSize; // 帧内容x字节
    }

}

void OrderPlayerWindow::setMusicIconBySource()
{
    switch (musicSource) {
    case UnknowMusic:
    case LocalMusic:
        return ;
    case NeteaseCloudMusic:
        ui->musicSourceButton->setIcon(QIcon(":/musics/netease"));
        ui->musicSourceButton->setToolTip("当前播放源：网易云音乐\n点击切换");
        break;
    case QQMusic:
        ui->musicSourceButton->setIcon(QIcon(":/musics/qq"));
        ui->musicSourceButton->setToolTip("当前播放源：QQ音乐\n点击切换");
        break;
    case MiguMusic:
        ui->musicSourceButton->setIcon(QIcon(":/musics/migu"));
        ui->musicSourceButton->setToolTip("当前播放源：咪咕音乐\n点击切换");
        break;
    case KugouMusic:
        ui->musicSourceButton->setIcon(QIcon(":/musics/kugou"));
        ui->musicSourceButton->setToolTip("当前播放源：酷狗音乐\n点击切换");
        break;
    }
}

/**
 * 切换音源并播放
 * 网易云：=> QQ音乐 => 咪咕 => 酷狗
 * QQ音乐：=> 网易云 => 咪咕 => 酷狗
 * 咪咕：=> 网易云 => QQ音乐 => 酷狗
 * 酷狗：=> 网易云 => QQ音乐 => 咪咕
 */
bool OrderPlayerWindow::switchSource(Song song, bool play)
{
    MusicSource res = UnknowMusic;
    switch (song.source) {
    case UnknowMusic:
    case LocalMusic:
        res = NeteaseCloudMusic;
        break;
    case NeteaseCloudMusic:
        if (musicSource == QQMusic)
            res = MiguMusic;
        else
            res = QQMusic;
        break;
    case QQMusic:
        if (musicSource == NeteaseCloudMusic || musicSource == KugouMusic)
            res= MiguMusic;
        else if (musicSource == MiguMusic)
            res = KugouMusic;
        else
            res = NeteaseCloudMusic;
        break;
    case MiguMusic:
        if (musicSource == MiguMusic)
            res = NeteaseCloudMusic;
        else
            res = KugouMusic;
        break;
    case KugouMusic:
        if (musicSource == KugouMusic)
            res = NeteaseCloudMusic;
        else
            res = musicSource;
        break;
    }

    if(res == musicSource) // 轮了一圈，又回来了
    {
        qWarning() << "所有平台都不支持，已结束";
        if (play)
            playNext();
        return false;
    }

    QString searchKey = song.name + " " + song.artistNames;
    // searchKey.replace(QRegularExpression("[（\\(].+[）\\)]"), ""); // 删除备注
    qWarning() << "无法播放：" << song.name << "，开始换源：" << song.source << "->" << res << searchKey;
    searchMusicBySource(searchKey, res, song.addBy);
    return true;
}

void OrderPlayerWindow::fetch(QString url, NetStringFunc func, MusicSource cookie)
{
    fetch(url, [=](QNetworkReply* reply){
        func(reply->readAll());
    }, cookie);
}

void OrderPlayerWindow::fetch(QString url, NetJsonFunc func, MusicSource cookie)
{
    fetch(url, [=](QNetworkReply* reply){
        QJsonParseError error;
        QByteArray ba = reply->readAll();
        if (cookie == KugouMusic)
        {
            if (ba.startsWith("<!--KG_TAG_RES_START-->"))
                ba = ba.right(ba.length() - QString("<!--KG_TAG_RES_START-->").length());
            if (ba.endsWith("<!--KG_TAG_RES_END-->"))
                ba = ba.left(ba.length() - QString("<!--KG_TAG_RES_END-->").length());
        }
        QJsonDocument document = QJsonDocument::fromJson(ba, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qWarning() << error.errorString() << url;
            qWarning() << ba.left(300);
            return ;
        }
        func(document.object());
    }, cookie);
}

void OrderPlayerWindow::fetch(QString url, NetReplyFunc func, MusicSource cookie)
{
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    if (cookie == UnknowMusic)
        cookie = musicSource;
    switch (cookie) {
    case UnknowMusic:
    case LocalMusic:
        break;
    case NeteaseCloudMusic:
        if (!neteaseCookies.isEmpty() && url.startsWith(NETEASE_SERVER))
            request->setHeader(QNetworkRequest::CookieHeader, neteaseCookiesVariant);
        break;
    case QQMusic:
        if (!qqmusicCookies.isEmpty() && url.startsWith(QQMUSIC_SERVER))
            request->setHeader(QNetworkRequest::CookieHeader, qqmusicCookiesVariant);
        break;
    case MiguMusic:
        break;
    case KugouMusic:
        if (!kugouCookies.isEmpty() && url.contains("kugou"))
            request->setHeader(QNetworkRequest::CookieHeader, kugouCookiesVariant);
        break;
    }

    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        manager->deleteLater();
        delete request;

        func(reply);
        reply->deleteLater();
    });
    manager->get(*request);
}

void OrderPlayerWindow::fetch(QString url, QStringList params, NetJsonFunc func, MusicSource cookie)
{
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    if (cookie == UnknowMusic)
        cookie = musicSource;
    switch (cookie) {
    case UnknowMusic:
    case LocalMusic:
        break;
    case NeteaseCloudMusic:
        if (!neteaseCookies.isEmpty() && url.startsWith(NETEASE_SERVER))
            request->setHeader(QNetworkRequest::CookieHeader, neteaseCookiesVariant);
        break;
    case QQMusic:
        if (!qqmusicCookies.isEmpty() && url.startsWith(QQMUSIC_SERVER))
            request->setHeader(QNetworkRequest::CookieHeader, qqmusicCookiesVariant);
        break;
    case MiguMusic:
    case KugouMusic:
        break;
    }

    QString data;
    for (int i = 0; i < params.size(); i++)
    {
        if (i & 1) // 用户数据
            data += QUrl::toPercentEncoding(params.at(i));
        else // 固定变量
            data += (i==0?"":"&") + params.at(i) + "=";
    }
    QByteArray ba(data.toLatin1());
    // MUSIC_DEB << url + "?" + data;

    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError)
        {
            qWarning() << error.errorString() << url;
            return ;
        }
        func(document.object());

        manager->deleteLater();
        delete request;
        reply->deleteLater();
    });
    manager->post(*request, ba);
}

QVariant OrderPlayerWindow::getCookies(QString cookieString)
{
    QList<QNetworkCookie> cookies;

    // 设置cookie
    QString cookieText = cookieString;
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

void OrderPlayerWindow::getNeteaseAccount()
{
    if (neteaseCookies.isEmpty())
    {
        neteaseNickname = "";
        return ;
    }
    fetch(NETEASE_SERVER + "/login/status", [=](MyJson json) {
        // qInfo() << json;
        if (json.code() != 200) // 也是301
        {
            neteaseNickname = "";
            qWarning() << "网易云账号：" << json.msg();
            return ;
        }
        neteaseNickname = json.o("profile").s("nickname");
        qInfo() << "网易云账号：" << neteaseNickname << "登录成功";
    }, NeteaseCloudMusic);
}

void OrderPlayerWindow::getQQMusicAccount()
{
    if (qqmusicCookies.isEmpty())
    {
        qqmusicNickname = "";
        return ;
    }
    QString uin = getStrMid(qqmusicCookies, " uin=", ";");
    fetch(QQMUSIC_SERVER + "/user/detail?id=" + uin, [=](MyJson json) {
        // qInfo() << json;
        if (json.i("result") == 301)
        {
            qqmusicNickname = "";
            qqmusicCookiesVariant = QVariant();
            qWarning() << "QQ音乐账号：" << json.s("errMsg") << json.s("info");
            if (json.s("errMsg") == "未登录") // 登录失效了
                settings.setValue("music/qqmusicCookies", qqmusicCookies = "");
            return ;
        }
        qqmusicNickname = json.data().o("creator").s("nick");
        qInfo() << "QQ音乐账号：" << qqmusicNickname << "登录成功";
    }, QQMusic);
}

void OrderPlayerWindow::importSongs(const QStringList &lines)
{
    QDir().mkpath(localMusicsFileDir.absolutePath());
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    // 从字符串中解析歌名/歌手
    auto setName = [=](QString fileName, Song& song) -> void {
        QRegularExpression re;
        if (importFormat == 0) // 歌名
        {
            song.name = fileName;
        }
        else
        {
            QString exp;
            QString songName;
            QString artNames;

            QStringList exps = {
                "^(.+)$", // 歌名
                "^(.+)-(.+?)$", // 歌名-歌手
                "^(.+)-(.+?)$", // 歌手-歌名
                "^(.+) (.+?)$", // 歌名-歌手
                "^(.+) (.+?)$", // 歌手-歌名
                "^\\[(.+)\\]\\s*(.+?)$", // 歌名-歌手
            };
            QList<int> songLeft = {1, 3};

            if (importFormat < 0 || importFormat >= exps.size())
            {
                qWarning() << "错误的导入格式代码：" << importFormat;
                return ;
            }

            QRegularExpressionMatch match;
            if (fileName.indexOf(QRegularExpression(exps.at(importFormat)), 0, &match) == -1)
            {
                qWarning() << "无法导入：" << fileName;
                return ;
            }
            if (songLeft.contains(importFormat))
            {
                songName = match.captured(1);
                artNames = match.captured(2);
            }
            else
            {
                songName = match.captured(2);
                artNames = match.captured(1);
            }

            song.name = songName;
            QStringList names = artNames.split(QRegExp("[/、]"), QString::SkipEmptyParts);
            foreach (auto name, names)
            {
                song.artists.append(Artist(name));
            }
            song.artistNames = names.join("/");
        }

    };

    int importCount = 0;
    QList<Song> songs;
    foreach (auto line, lines)
    {
        line = line.trimmed();
        if (line.isEmpty())
            continue;

        Song song;
        if (line.startsWith("file:///")) // 如果是本地文件
        {
            QString path = QUrl(line).toLocalFile();
            QFileInfo info(path);
            QString baseName = info.baseName();
            QString fileName = info.fileName();
            QString suffix = info.suffix();
            if (!info.exists())
            {
                qWarning() << "不存在的文件：" << path;
                continue;
            }
            if (QStringList{}.contains(suffix))
            {
                qWarning() << "不支持的音乐文件格式：" << path;
                continue;
            }

            // 开始导入文件
            if (importAbsolutPath) // 绝对路径，即不导入文件
            {
                song.filePath = "file:///" + path;
            }
            else
            {
                QString newPath = localMusicsFileDir.absoluteFilePath(fileName);
                copyFile(path, newPath, true);
                song.filePath = fileName;
            }

            song.source = LocalMusic;
            song.id = timestamp * 10000 + importCount;
            setName(baseName, song);
            songs.append(song);
        }
        else // 不是文件，就当做歌名了！
        {
            setName(line, song);
            importingSongNames.append(song);
        }

        importCount++;
    }

    auto chi = ui->listTabWidget->currentWidget()->children();
    if (chi.contains(ui->orderSongsListView))
        importingList = &orderSongs;
    else if (chi.contains(ui->normalSongsListView))
        importingList = &normalSongs;
    else if (chi.contains(ui->favoriteSongsListView))
        importingList = &favoriteSongs;
    else
        importingList = nullptr;

    if (!importingList)
    {
        qWarning() << "无法确定要添加到哪个列表";
        return ;
    }

    if (songs.size())
    {
        if (importingList == &orderSongs)
            appendOrderSongs(songs);
        else if (importingList == &normalSongs)
            addNormal(songs);
        else if (importingList == &favoriteSongs)
            addFavorite(songs);
    }

    if (importingSongNames.size())
        importNextSongByName();
}

/**
 * 判断 importingSongNames 有没有下一个
 * 如果有，则继续导入
 */
void OrderPlayerWindow::importNextSongByName()
{

}

/**
 * 列表项改变
 */
void OrderPlayerWindow::on_listTabWidget_currentChanged(int index)
{
    if (starting)
        return ;
    settings.setValue("orderplayerwindow/tabIndex", index);
}

/**
 * 改变排序
 */
void OrderPlayerWindow::sortSearchResult(int col)
{
    Q_UNUSED(col)
}

/**
 * 播放进度条被拖动
 */
void OrderPlayerWindow::on_playProgressSlider_sliderReleased()
{
    int position = ui->playProgressSlider->sliderPosition();
    player->setPosition(position);
}

/**
 * 播放进度条被拖动
 */
void OrderPlayerWindow::on_playProgressSlider_sliderMoved(int position)
{
    player->setPosition(position);
}

/**
 * 音量进度被拖动
 */
void OrderPlayerWindow::on_volumeSlider_sliderMoved(int position)
{
    player->setVolume(position);
    settings.setValue("music/volume", position);
}

/**
 * 暂停/播放/随机推荐
 */
void OrderPlayerWindow::on_playButton_clicked()
{
    if (player->state() == QMediaPlayer::PlayingState) // 暂停播放
    {
        player->pause();
    }
    else // 开始播放
    {
        if (!playingSong.isValid())
        {
            playNext();
            return ;
        }
        player->play();
    }
}

/**
 * 静音/恢复音量
 */
void OrderPlayerWindow::on_volumeButton_clicked()
{
    int volume = ui->volumeSlider->sliderPosition();
    if (volume == 0) // 恢复音量
    {
        volume = settings.value("music/volume", 50).toInt();
        if (volume == 0)
            volume = 50;
        ui->volumeButton->setIcon(QIcon(":/icons/volume"));
        ui->volumeSlider->setSliderPosition(volume);
        settings.setValue("music/mute", false);
        settings.setValue("music/volume", volume);
    }
    else // 静音
    {
        volume = 0;
        ui->volumeButton->setIcon(QIcon(":/icons/mute"));
        ui->volumeSlider->setSliderPosition(0);
        settings.setValue("music/mute", true);
    }
    player->setVolume(volume);
}

/**
 * 循环方式
 */
void OrderPlayerWindow::on_circleModeButton_clicked()
{
    if (circleMode == OrderList)
    {
        circleMode = SingleCircle;
        ui->circleModeButton->setIcon(QIcon(":/icons/single_circle"));
    }
    else
    {
        circleMode = OrderList;
        ui->circleModeButton->setIcon(QIcon(":/icons/order_list"));
    }
    settings.setValue("music/mode", circleMode);
}

void OrderPlayerWindow::slotSongPlayEnd()
{
    emit signalSongPlayFinished(playingSong);
    // 根据循环模式
    if (circleMode == OrderList) // 列表顺序
    {
        // 清除播放
        player->stop();
        playingSong = Song();
        settings.setValue("music/currentSong", "");
        ui->playingNameLabel->clear();
        ui->playingArtistLabel->clear();
        ui->playingCoverLabel->clear();
        coveringSong = Song();
        setCurrentLyric("");

        // 没有歌了，真正清空
        if (!orderSongs.size() && !normalSongs.size())
        {
            ui->playProgressSlider->setSliderPosition(0);
            // ui->playProgressSlider->setMaximum(0);
            ui->playingCurrentTimeLabel->setText("00:00");
            // ui->playingAllTimeLabel->setText("05:20");

            setCurrentCover(QPixmap(":bg/bg"));
            emit signalOrderSongEnded();
            emit signalCurrentSongChanged(Song());
        }
        else
        {
            // 还有下一首歌
            playNext();
        }
    }
    else if (circleMode == SingleCircle) // 单曲循环
    {
        // 不用管，会自己放下去
        player->setPosition(0);
        player->play();
    }
}

void OrderPlayerWindow::on_orderSongsListView_customContextMenuRequested(const QPoint &)
{
    auto indexes = ui->orderSongsListView->selectionModel()->selectedRows(0);
    SongList songs;
    foreach (auto index, indexes)
        songs.append(orderSongs.at(index.row()));
    int row = ui->orderSongsListView->currentIndex().row();
    Song currentSong;
    if (row > -1 && row < orderSongs.size())
        currentSong = orderSongs.at(row);

    FacileMenu* menu = new FacileMenu(this);

    if (!currentSong.addBy.isEmpty())
    {
        currentSong.setAddDesc(currentSong.addBy);
        menu->addAction(currentSong.addBy)->disable();
        menu->split();
    }

    menu->addAction("立即播放", [=]{
        Song song = orderSongs.takeAt(row);
        startPlaySong(song);
        orderSongs.removeOne(song);
        saveSongList("music/order", orderSongs);
        setSongModelToView(orderSongs, ui->orderSongsListView);
    })->disable(songs.size() != 1 || !currentSong.isValid());

    menu->addAction("下一首播放", [=]{
        appendNextSongs(songs);
    })->disable(!songs.size());

    menu->split()->addAction("添加空闲播放", [=]{
        addNormal(songs);
    })->disable(!songs.size());

    menu->addAction("收藏", [=]{
        if (!favoriteSongs.contains(currentSong))
            addFavorite(songs);
        else
            removeFavorite(songs);
    })->disable(!songs.size())
            ->text(favoriteSongs.contains(currentSong), "从收藏中移除", "添加到收藏");

    menu->split()->addAction("上移", [=]{
        orderSongs.swapItemsAt(row, row-1);
        saveSongList("music/order", orderSongs);
        setSongModelToView(orderSongs, ui->orderSongsListView);
    })->disable(songs.size() != 1 || row < 1);

    menu->addAction("下移", [=]{
        orderSongs.swapItemsAt(row, row+1);
        saveSongList("music/order", orderSongs);
        setSongModelToView(orderSongs, ui->orderSongsListView);
    })->disable(songs.size() != 1 || row >= orderSongs.size()-1);

    menu->split()->addAction("删除", [=]{
        removeOrder(songs);
    })->disable(!songs.size());

    menu->split()->addAction("导入歌单链接", [=]{
        inputPlayList();
    });

    menu->addAction("批量导入歌曲", [=]{
        openMultiImport();
    });

    if (currentSong.isValid())
        menu->split()->addAction(currentSong.sourceName())->disable();

    menu->exec();
}

void OrderPlayerWindow::on_favoriteSongsListView_customContextMenuRequested(const QPoint &)
{
    auto indexes = ui->favoriteSongsListView->selectionModel()->selectedRows(0);
    SongList songs;
    foreach (auto index, indexes)
        songs.append(favoriteSongs.at(index.row()));
    int row = ui->favoriteSongsListView->currentIndex().row();
    Song currentSong;
    if (row > -1 && row < favoriteSongs.size())
        currentSong = favoriteSongs.at(row);

    FacileMenu* menu = new FacileMenu(this);

    menu->addAction("立即播放", [=]{
        Song song = favoriteSongs.at(row);
        startPlaySong(song);
    })->disable(songs.size() != 1 || !currentSong.isValid());

    menu->split()->addAction("下一首播放", [=]{
        appendNextSongs(songs);
    })->disable(!songs.size());

    menu->addAction("添加到播放列表", [=]{
        appendOrderSongs(songs);
    })->disable(!songs.size());

    menu->addAction("添加空闲播放", [=]{
        addNormal(songs);
    })->disable(!songs.size());

    menu->split()->addAction("上移", [=]{
        favoriteSongs.swapItemsAt(row, row-1);
        saveSongList("music/order", favoriteSongs);
        setSongModelToView(favoriteSongs, ui->favoriteSongsListView);
    })->disable(songs.size() != 1 || row < 1);

    menu->addAction("下移", [=]{
        favoriteSongs.swapItemsAt(row, row+1);
        saveSongList("music/order", favoriteSongs);
        setSongModelToView(favoriteSongs, ui->favoriteSongsListView);
    })->disable(songs.size() != 1 || row >= favoriteSongs.size()-1);

    menu->split()->addAction("移出收藏", [=]{
        removeFavorite(songs);
    })->disable(!songs.size());

    menu->split()->addAction("导入歌单链接", [=]{
        inputPlayList();
    });

    menu->addAction("批量导入歌曲", [=]{
        openMultiImport();
    });

    if (currentSong.isValid())
        menu->split()->addAction(currentSong.sourceName())->disable();

    menu->exec();
}

void OrderPlayerWindow::on_listSongsListView_customContextMenuRequested(const QPoint &)
{
    auto indexes = ui->favoriteSongsListView->selectionModel()->selectedRows(0);
    int row = ui->listSongsListView->currentIndex().row();
    PlayList pl;
    if (row > -1)
    {
    }

    FacileMenu* menu = new FacileMenu(this);

    menu->addAction("加载歌单", [=]{

    });

    menu->addAction("播放里面歌曲", [=]{

    });

    menu->split()->addAction("删除歌单", [=]{

    });

    menu->exec();
}

void OrderPlayerWindow::on_normalSongsListView_customContextMenuRequested(const QPoint &)
{
    auto indexes = ui->normalSongsListView->selectionModel()->selectedRows(0);
    SongList songs;
    foreach (auto index, indexes)
        songs.append(normalSongs.at(index.row()));
    int row = ui->normalSongsListView->currentIndex().row();
    Song currentSong;
    if (row > -1 && row < normalSongs.size())
        currentSong = normalSongs.at(row);

    FacileMenu* menu = new FacileMenu(this);

    menu->addAction("立即播放", [=]{
        Song song = normalSongs.at(row);
        startPlaySong(song);
    })->disable(songs.size() != 1 || !currentSong.isValid());

    menu->split()->addAction("下一首播放", [=]{
        appendNextSongs(songs);
    })->disable(!songs.size());

    menu->addAction("添加到播放列表", [=]{
        appendOrderSongs(songs);
    })->disable(!songs.size());

    menu->addAction("收藏", [=]{
        if (!favoriteSongs.contains(currentSong))
            addFavorite(songs);
        else
            removeFavorite(songs);
    })->disable(!songs.size())
            ->text(favoriteSongs.contains(currentSong), "从收藏中移除", "添加到收藏");

    menu->split()->addAction("上移", [=]{
        // normalSongs.swapItemsAt(row, row-1); // 5.13之前不支持
        normalSongs.insert(row - 1, normalSongs.takeAt(row));
        saveSongList("music/order", normalSongs);
        setSongModelToView(normalSongs, ui->normalSongsListView);
    })->disable(songs.size() != 1 || row < 1);

    menu->addAction("下移", [=]{
        // normalSongs.swapItemsAt(row, row+1); // 5.13之前不支持
        normalSongs.insert(row, normalSongs.takeAt(row + 1));
        saveSongList("music/order", normalSongs);
        setSongModelToView(normalSongs, ui->normalSongsListView);
    })->disable(songs.size() != 1 || row >= normalSongs.size()-1);

    menu->split()->addAction("移出空闲播放", [=]{
        removeNormal(songs);
    })->disable(!songs.size());

    menu->split()->addAction("导入歌单链接", [=]{
        inputPlayList();
    });

    menu->addAction("批量导入歌曲", [=]{
        openMultiImport();
    });

    if (currentSong.isValid())
        menu->split()->addAction(currentSong.sourceName())->disable();

    menu->exec();
}

void OrderPlayerWindow::on_historySongsListView_customContextMenuRequested(const QPoint &)
{
    auto indexes = ui->historySongsListView->selectionModel()->selectedRows(0);
    SongList songs;
    foreach (auto index, indexes)
        songs.append(historySongs.at(index.row()));
    int row = ui->historySongsListView->currentIndex().row();
    Song currentSong;
    if (row > -1 && row < historySongs.size())
        currentSong = historySongs.at(row);

    FacileMenu* menu = new FacileMenu(this);

    if (!currentSong.addBy.isEmpty())
    {
        currentSong.setAddDesc(currentSong.addBy);
        menu->addAction(currentSong.addBy)->disable();
        menu->split();
    }

    menu->addAction("立即播放", [=]{
        Song song = historySongs.at(row);
        startPlaySong(song);
    })->disable(songs.size() != 1 || !currentSong.isValid());

    menu->split()->addAction("下一首播放", [=]{
        appendNextSongs(songs);
    })->disable(!songs.size());

    menu->addAction("添加到播放列表", [=]{
        appendOrderSongs(songs);
    })->disable(!songs.size());

    menu->addAction("添加空闲播放", [=]{
        addNormal(songs);
    })->disable(!songs.size());

    menu->addAction("收藏", [=]{
        if (!favoriteSongs.contains(currentSong))
            addFavorite(songs);
        else
            removeFavorite(songs);
    })->disable(!songs.size())
            ->text(favoriteSongs.contains(currentSong), "从收藏中移除", "添加到收藏");

    menu->split()->addAction("清理下载文件", [=]{
        foreach (Song song, songs)
        {
            QString path = songPath(song);
            if (QFileInfo(path).exists())
                QFile(path).remove();
            path = coverPath(song);
            if (QFileInfo(path).exists())
                QFile(path).remove();
            path = lyricPath(song);
            if (QFileInfo(path).exists())
                QFile(path).remove();
        }
    })->disable(!songs.size());

    menu->addAction("删除记录", [=]{
        QPoint center = ui->listTabWidget->tabBar()->tabRect(LISTTAB_HISTORY).center();
        foreach (Song song, songs)
        {
            if (historySongs.removeOne(song))
                showTabAnimation(center, "+1");
        }
        saveSongList("music/history", historySongs);
        setSongModelToView(historySongs, ui->historySongsListView);
    })->disable(!songs.size());

    if (currentSong.isValid())
        menu->split()->addAction(currentSong.sourceName())->disable();

    menu->exec();
}

void OrderPlayerWindow::on_orderSongsListView_activated(const QModelIndex &index)
{
    Song song = orderSongs.takeAt(index.row());
    setSongModelToView(orderSongs, ui->orderSongsListView);
    startPlaySong(song);
}

void OrderPlayerWindow::on_favoriteSongsListView_activated(const QModelIndex &index)
{
    Song song = favoriteSongs.at(index.row());
    activeSong(song);
}

void OrderPlayerWindow::on_normalSongsListView_activated(const QModelIndex &index)
{
    Song song = normalSongs.at(index.row());
    activeSong(song);
}

void OrderPlayerWindow::on_historySongsListView_activated(const QModelIndex &index)
{
    Song song = historySongs.at(index.row());
    activeSong(song);
}

void OrderPlayerWindow::on_desktopLyricButton_clicked()
{
    bool showDesktopLyric = !settings.value("music/desktopLyric", false).toBool();
    settings.setValue("music/desktopLyric", showDesktopLyric);
    if (showDesktopLyric)
    {
        desktopLyric->show();
        ui->desktopLyricButton->setIcon(QIcon(":/icons/lyric_show"));
    }
    else
    {
        desktopLyric->hide();
        ui->desktopLyricButton->setIcon(QIcon(":/icons/lyric_hide"));
    }
}

void OrderPlayerWindow::slotExpandPlayingButtonClicked()
{
    if (ui->bodyStackWidget->currentWidget() == ui->lyricsPage) // 隐藏歌词
    {
        QRect rect(ui->bodyStackWidget->mapTo(this, QPoint(5, 0)), ui->bodyStackWidget->size()-QSize(5, 0));
        QPixmap pixmap(rect.size());
        render(&pixmap, QPoint(0, 0), rect);
        QLabel* label = new QLabel(this);
        label->setScaledContents(true);
        label->setGeometry(rect);
        label->setPixmap(pixmap);
        QPropertyAnimation* ani = new QPropertyAnimation(label, "geometry");
        ani->setStartValue(rect);
        ani->setEndValue(QRect(ui->playingCoverLabel->geometry().center(), QSize(1,1)));
        ani->setEasingCurve(QEasingCurve::InOutCubic);
        ani->setDuration(300);
        connect(ani, &QPropertyAnimation::finished, this, [=]{
            ani->deleteLater();
            label->deleteLater();
        });
        label->show();
        ani->start();
        ui->bodyStackWidget->setCurrentWidget(ui->searchResultPage);
        settings.setValue("music/lyricStream", false);
    }
    else // 显示歌词
    {
        ui->bodyStackWidget->setCurrentWidget(ui->lyricsPage);
        QRect rect(ui->bodyStackWidget->mapTo(this, QPoint(5, 0)), ui->bodyStackWidget->size()-QSize(5, 0));
        QPixmap pixmap(rect.size());
        render(&pixmap, QPoint(0, 0), rect);
        QLabel* label = new QLabel(this);
        label->setScaledContents(true);
        label->setGeometry(rect);
        label->setPixmap(pixmap);
        QPropertyAnimation* ani = new QPropertyAnimation(label, "geometry");
        ani->setStartValue(QRect(ui->playingCoverLabel->geometry().center(), QSize(1,1)));
        ani->setEndValue(rect);
        ani->setDuration(300);
        ani->setEasingCurve(QEasingCurve::InOutCubic);
        connect(ani, &QPropertyAnimation::finished, this, [=]{
            ui->bodyStackWidget->setCurrentWidget(ui->lyricsPage);
            ani->deleteLater();
            label->deleteLater();
        });
        label->show();
        ani->start();
        ui->bodyStackWidget->setCurrentWidget(ui->searchResultPage);
        settings.setValue("music/lyricStream", true);
    }
}

void OrderPlayerWindow::slotPlayerPositionChanged()
{
    qint64 position = player->position();
    if (desktopLyric && !desktopLyric->isHidden())
        desktopLyric->setPosition(position);
    if (ui->lyricWidget->setPosition(position))
    {
        // 开始滚动
        QPropertyAnimation* ani = new QPropertyAnimation(this, "lyricScroll");
        ani->setStartValue(ui->scrollArea->verticalScrollBar()->sliderPosition());
        ani->setEndValue(qMax(0, ui->lyricWidget->getCurrentTop() - this->height()/2));
        ani->setDuration(200);
        connect(ani, &QPropertyAnimation::valueChanged, this, [=]{
            ui->scrollArea->verticalScrollBar()->setSliderPosition(lyricScroll);
        });
        connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
        ani->start();
    }
    ui->playProgressSlider->setSliderPosition(static_cast<int>(position));
    update();
}

void OrderPlayerWindow::on_splitter_splitterMoved(int, int)
{
    adjustExpandPlayingButton();
}

void OrderPlayerWindow::on_titleButton_clicked()
{
    QString text = settings.value("music/title").toString();
    bool ok;
    QString rst = QInputDialog::getText(this, "专属标志", "固定在左上角，乃个人定制", QLineEdit::Normal, text, &ok);
    if (!ok)
        return ;
    settings.setValue("music/title", rst);
    ui->titleButton->setText(rst);
}

/**
 * 调整当前歌词的时间
 */
void OrderPlayerWindow::adjustCurrentLyricTime(QString lyric)
{
    if (!playingSong.isValid())
        return ;
    QFile file(lyricPath(playingSong));
    file.open(QIODevice::WriteOnly);
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << lyric;
    file.flush();
    file.close();

    // 调整桌面歌词
    desktopLyric->setLyric(lyric);
    desktopLyric->setPosition(player->position());
}

void OrderPlayerWindow::on_settingsButton_clicked()
{
    FacileMenu* menu = new FacileMenu(this);

    FacileMenu* accountMenu = menu->addMenu("账号");

    accountMenu->addAction("添加账号", [=]{
        LoginDialog* dialog = new LoginDialog(this);
        connect(dialog, &LoginDialog::signalLogined, this, [=](MusicSource source, QString cookies){
            switch (source) {
            case UnknowMusic:
                return ;
            case NeteaseCloudMusic:
                neteaseCookies = cookies;
                settings.setValue("music/neteaseCookies", cookies);
                neteaseCookiesVariant = getCookies(neteaseCookies);
                getNeteaseAccount();
                break;
            case QQMusic:
                qqmusicCookies = cookies;
                settings.setValue("music/qqmusicCookies", cookies);
                qqmusicCookiesVariant = getCookies(qqmusicCookies);
                getQQMusicAccount();
                if (unblockQQMusic)
                    settings.setValue("music/unblockQQMusic", unblockQQMusic = false);
                break;
            case MiguMusic:
            case KugouMusic:
            case LocalMusic:
                return ;
            }
            clearDownloadFiles();
        });
        dialog->exec();
    })->uncheck();

    accountMenu->split();
    accountMenu->addAction(QIcon(":/musics/netease"), "网易云音乐", [=]{
        if (QMessageBox::question(this, "网易云音乐", "是否退出网易云音乐账号？") == QMessageBox::Yes)
        {
            neteaseCookies = "";
            neteaseCookiesVariant.clear();
            settings.setValue("music/neteaseCookies", "");
        }
    })->text(!neteaseNickname.isEmpty(), neteaseNickname)->disable(neteaseCookies.isEmpty());

    accountMenu->addAction(QIcon(":/musics/qq"), "QQ音乐", [=]{
        if (QMessageBox::question(this, "QQ音乐", "是否退出QQ音乐账号？") == QMessageBox::Yes)
        {
            qqmusicCookies = "";
            qqmusicCookiesVariant.clear();
            settings.setValue("music/qqmusicCookies", "");
        }
    })->text(!qqmusicNickname.isEmpty(), qqmusicNickname)->disable(qqmusicCookies.isEmpty());

    FacileMenu* playMenu = menu->addMenu("播放");

    playMenu->addAction("伴奏优先", [=]{
        settings.setValue("music/accompanyMode", accompanyMode = !accompanyMode);
    })->check(accompanyMode)->tooltip("外部点歌，自动搜索伴奏");

    playMenu->addAction("双击播放", [=]{
        settings.setValue("music/doubleClickToPlay", doubleClickToPlay = !doubleClickToPlay);
    })->check(doubleClickToPlay);

    playMenu->addAction("音乐品质", [=]{
        bool ok = false;
        int br = QInputDialog::getInt(this, "设置码率", "请输入音乐码率，越高越清晰，体积也更大\n修改后将清理所有缓存，重新下载歌曲文件", songBr, 128000, 1280000, 10000, &ok);
        if (!ok)
            return ;
        if (songBr != br)
            clearDownloadFiles();
        settings.setValue("music/br", songBr = br);
    })->check(songBr >= 320000);

    playMenu->split()->addAction("自动换源", [=]{
        settings.setValue("music/autoSwitchSource", autoSwitchSource = !autoSwitchSource);
    })->setChecked(autoSwitchSource);

    playMenu->addAction("试听接口", [=]{
        settings.setValue("music/unblockQQMusic", unblockQQMusic = !unblockQQMusic);
        if (unblockQQMusic)
            QMessageBox::information(this, "试听接口", "可在不登录的情况下试听QQ音乐的VIP歌曲1分钟\n若已登录QQ音乐的会员用户，十分建议关掉");
    })->check(unblockQQMusic);

    playMenu->split()->addAction("清理缓存", [=]{
        clearDownloadFiles();
    })->uncheck();

    FacileMenu* stMenu = menu->addMenu("设置");

    bool h = settings.value("music/hideTab", false).toBool();

    stMenu->addAction("模糊背景", [=]{
        settings.setValue("music/blurBg", blurBg = !blurBg);
        if (blurBg)
            setBlurBackground(currentCover);
        update();
    })->setChecked(blurBg);

    QStringList sl{"32", "64", "96", "128"/*, "160", "192", "224", "256"*/};
    auto blurAlphaMenu = stMenu->addMenu("模糊透明");
    stMenu->lastAction()->hide(!blurBg)->uncheck();
    blurAlphaMenu->addOptions(sl, blurAlpha / 32 - 1, [=](int index){
        blurAlpha = (index+1) * 32;
        settings.setValue("music/blurAlpha", blurAlpha);
        setBlurBackground(currentCover);
    });

    stMenu->addAction("主题变色", [=]{
        settings.setValue("music/themeColor", themeColor = !themeColor);
        if (themeColor)
            setThemeColor(currentCover);
        update();
    })->setChecked(themeColor);

    stMenu->split()->addAction("隐藏Tab", [=]{
        settings.setValue("music/hideTab", !h);
        if (h)
            ui->listTabWidget->tabBar()->show();
        else
            ui->listTabWidget->tabBar()->hide();
    })->check(h);

    menu->exec();
}

void OrderPlayerWindow::on_musicSourceButton_clicked()
{
    auto changeSource = [=](int s){
        if (musicSource == s) // 没有改变
            return ;
        musicSource = MusicSource(s);
        settings.setValue("music/source", musicSource);

        setMusicIconBySource();

        QString text = ui->searchEdit->text();
        if (!text.isEmpty())
        {
            searchMusic(text, currentResultOrderBy);
            ui->bodyStackWidget->setCurrentWidget(ui->searchResultPage);
        }
        else
        {
            ui->searchResultTable->clear();
        }
    };

    /* const int musicCount = 4;
    changeSource((int(musicSource) + 1) % musicCount); */

    auto menu = new FacileMenu(this);

    menu->addAction(QIcon(":/musics/netease"), "网易云音乐", [=]{
        changeSource(NeteaseCloudMusic);
    });

    menu->addAction(QIcon(":/musics/qq"), "QQ音乐", [=]{
        changeSource(QQMusic);
    });

    menu->addAction(QIcon(":/musics/migu"), "咪咕音乐", [=]{
        changeSource(MiguMusic);
    });

    menu->addAction(QIcon(":/musics/kugou"), "酷狗音乐", [=]{
        changeSource(KugouMusic);
    });

    if (musicSource >= NeteaseCloudMusic && musicSource <= KugouMusic)
        menu->at(musicSource)->check();

    menu->exec();

}

void OrderPlayerWindow::on_nextSongButton_clicked()
{
    if (orderSongs.size())
        playNext();
    else
        slotSongPlayEnd(); // 顺序停止播放；单曲重新播放
}
