#include "orderplayerwindow.h"
#include "ui_orderplayerwindow.h"

OrderPlayerWindow::OrderPlayerWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::OrderPlayerWindow),
      settings(QApplication::applicationDirPath()+"/musics.ini", QSettings::Format::IniFormat),
      musicsFileDir(QApplication::applicationDirPath()+"/musics"),
      player(new QMediaPlayer(this)),
      desktopLyric(new DesktopLyricWidget(nullptr)),
      expandPlayingButton(new InteractiveButtonBase(this)),
      playingPositionTimer(new QTimer(this))
{
    ui->setupUi(this);

    QHeaderView* header = ui->searchResultTable->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);
    header->setMinimumSectionSize(QFontMetrics(this->font()).horizontalAdvance("哈哈哈哈哈哈"));
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    header->setStyleSheet("QHeaderView { background-color: transparent; }");
    ui->searchResultTable->verticalHeader()->setStyleSheet("QHeaderView { background-color: transparent; }");
    ui->searchResultTable->setItemDelegate(new NoFocusDelegate());
    ui->orderSongsListView->setItemDelegate(new NoFocusDelegate());
    ui->normalSongsListView->setItemDelegate(new NoFocusDelegate());
    ui->favoriteSongsListView->setItemDelegate(new NoFocusDelegate());
    ui->listSongsListView->setItemDelegate(new NoFocusDelegate());
    ui->historySongsListView->setItemDelegate(new NoFocusDelegate());

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
    ui->titleButton->setText(settings.value("music/title", "Lazy点歌姬").toString());

    QPalette pa;
    pa.setColor(QPalette::Highlight, QColor(100, 149, 237, 88));
    QApplication::setPalette(pa);

    connect(ui->searchResultTable->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(sortSearchResult(int)));

    connect(player, &QMediaPlayer::positionChanged, this, [=](qint64 position){
        ui->playingCurrentTimeLabel->setText(msecondToString(position));
        slotPlayerPositionChanged();
    });
    connect(player, &QMediaPlayer::durationChanged, this, [=](qint64 duration){
        ui->playProgressSlider->setMaximum(static_cast<int>(duration));
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
            qDebug() << "无效媒体：" << playingSong.simpleString();
            playNext();
        }
    });
    connect(player, &QMediaPlayer::stateChanged, this, [=](QMediaPlayer::State state){
        if (state == QMediaPlayer::PlayingState)
        {
            playingPositionTimer->start();
            ui->playButton->setIcon(QIcon(":/icons/pause"));
        }
        else
        {
            playingPositionTimer->stop();
            ui->playButton->setIcon(QIcon(":/icons/play"));
        }
    });

    connect(ui->lyricWidget, SIGNAL(signalAdjustLyricTime(QString)), this, SLOT(adjustCurrentLyricTime(QString)));

    musicsFileDir.mkpath(musicsFileDir.absolutePath());
    QTime time;
    time= QTime::currentTime();
    qsrand(time.msec()+time.second()*1000);

    bool lyricStack = settings.value("music/lyricStream", false).toBool();
    if (lyricStack)
        ui->bodyStackWidget->setCurrentWidget(ui->lyricsPage);

    blurBg = settings.value("music/blurBg", blurBg).toBool();
    blurAlpha = settings.value("music/blurAlpha", blurAlpha).toInt();
    themeColor = settings.value("music/themeColor", themeColor).toBool();

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
    settings.setValue("music/playPosition", 0);

    connect(expandPlayingButton, SIGNAL(clicked()), this, SLOT(slotExpandPlayingButtonClicked()));
    expandPlayingButton->show();

    // searchMusic("司夏"); // 测试用的

    prevBlurBg = QPixmap(32, 32);
    prevBlurBg.fill(QColor(245, 245, 247));
    playingPositionTimer->setInterval(100);
    connect(playingPositionTimer, &QTimer::timeout, this, [=]{
        if (player->state() == QMediaPlayer::PlayingState)
            slotPlayerPositionChanged();
    });
}

OrderPlayerWindow::~OrderPlayerWindow()
{
    delete ui;
    desktopLyric->deleteLater();
}

bool OrderPlayerWindow::hasSongInOrder(QString by)
{
    foreach (Song song, orderSongs)
    {
        if (song.addBy == by)
            return true;
    }
    return false;
}

void OrderPlayerWindow::on_searchEdit_returnPressed()
{
    QString text = ui->searchEdit->text();
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
    ui->searchEdit->setText(key);
    orderBys.append(by);
    searchMusic(key);
}

/**
 * 搜索音乐
 */
void OrderPlayerWindow::searchMusic(QString key)
{
    if (key.trimmed().isEmpty())
        return ;
    QString url = "http://iwxyi.com:3000/search?keywords=" + key.toUtf8().toPercentEncoding();
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
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        if (json.value("code").toInt() != 200)
        {
            qDebug() << ("返回结果不为200：") << json.value("message").toString();
            return ;
        }

        QJsonArray songs = json.value("result").toObject().value("songs").toArray();
        searchResultSongs.clear();
        foreach (QJsonValue val, songs)
        {
            searchResultSongs << Song::fromJson(val.toObject());
        }

        setSearchResultTable(searchResultSongs);

        // 从点歌的槽进来的
        if (orderBys.size())
        {
            QString by = orderBys.takeFirst();
            Song song = searchResultSongs.first();
            song.setAddDesc(by);

            // 添加到点歌列表
            if (searchResultSongs.size())
                appendOrderSongs(SongList{song});

            // 发送点歌成功的信号
            qint64 sumLatency = isNotPlaying() ? 0 : player->duration() - player->position();
            for (int i = 0; i < orderSongs.size()-1; i++)
            {
                if (orderSongs.at(i).id == song.id) // 同一首歌，如果全都不同，那就是下一首
                    break;
                sumLatency += orderSongs.at(i).duration;
            }
            emit signalOrderSongSucceed(song, sumLatency);
        }
    });
    manager->get(*request);
}

/**
 * 搜索结果数据到Table
 */
void OrderPlayerWindow::setSearchResultTable(SongList songs)
{
    QTableWidget* table = ui->searchResultTable;
    table->clear();
    searchResultPlayLists.clear();

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
}

void OrderPlayerWindow::setSearchResultTable(PlayListList playLists)
{
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
            qDebug() << "歌曲已收藏：" << song.simpleString();
            continue;
        }
        favoriteSongs.append(song);
        showTabAnimation(center, "+1");
        qDebug() << "添加收藏：" << song.simpleString();
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
            qDebug() << "取消收藏：" << song.simpleString();
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
    }
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
}

/**
 * 音乐文件的路径
 */
QString OrderPlayerWindow::songPath(const Song &song) const
{
    return musicsFileDir.absoluteFilePath(snum(song.id) + ".mp3");
}

QString OrderPlayerWindow::lyricPath(const Song &song) const
{
    return musicsFileDir.absoluteFilePath(snum(song.id) + ".lrc");
}

QString OrderPlayerWindow::coverPath(const Song &song) const
{
    return musicsFileDir.absoluteFilePath(snum(song.id) + ".jpg");
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

void OrderPlayerWindow::showEvent(QShowEvent *)
{
    restoreGeometry(settings.value("orderplayerwindow/geometry").toByteArray());
    restoreState(settings.value("orderplayerwindow/state").toByteArray());
    ui->splitter->restoreState(settings.value("orderplayerwindow/splitterState").toByteArray());
    auto sizes = ui->splitter->sizes();
    if (sizes.at(0)+1 >= sizes.at(1))
    {
        int sum = sizes.at(0) + sizes.at(1);
        int w = sum / 3;
        ui->splitter->setSizes(QList<int>{w, sum - w});
    }

    adjustExpandPlayingButton();

    if (settings.value("music/desktopLyric", false).toBool())
        desktopLyric->show();
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
            painter.setOpacity((double)currentBgAlpha / 255);
            painter.drawPixmap(rect(), currentBlurBg);
        }
        if (!prevBlurBg.isNull() && prevBgAlpha)
        {
            painter.setOpacity((double)prevBgAlpha / 255);
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
}

int OrderPlayerWindow::getAppearBgProg() const
{
    return this->currentBgAlpha;
}

void OrderPlayerWindow::setDisappearBgProg(int x)
{
    this->prevBgAlpha = x;
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
        menu->addAction("立即播放", [=]{
            startPlaySong(currentSong);
        })->disable(songs.size() != 1);

        menu->addAction("下一首播放", [=]{
            appendNextSongs(songs);
        })->disable(!currentSong.isValid());

        menu->addAction("添加到播放列表", [=]{
            appendOrderSongs(songs);
        })->disable(!currentSong.isValid());

        menu->addAction("添加常时播放", [=]{
            addNormal(songs);
        })->disable(!currentSong.isValid());

        menu->split()->addAction("收藏", [=]{
            if (!favoriteSongs.contains(currentSong))
                addFavorite(songs);
            else
                removeFavorite(songs);
        })->disable(!currentSong.isValid())
                ->text(favoriteSongs.contains(currentSong), "从收藏中移除", "添加到收藏");

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
    else
    {
        playAfterDownloaded = song;
        downloadSong(song);
    }
}

void OrderPlayerWindow::playNext()
{
    if (!orderSongs.size()) // 播放列表全部结束
    {
        // 查看常时列表
        if (!normalSongs.size())
            return ;

        int r = qrand() % normalSongs.size();
        startPlaySong(normalSongs.at(r));
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
    foreach (Song song, songs)
    {
        if (orderSongs.contains(song))
            continue;
        orderSongs.append(song);
        addDownloadSong(song);
        showTabAnimation(center, "+1");
    }
    saveSongList("music/order", orderSongs);
    setSongModelToView(orderSongs, ui->orderSongsListView);

    if (isNotPlaying() && orderSongs.size())
    {
        qDebug() << "当前未播放，开始播放列表";
        startPlaySong(orderSongs.takeFirst());
        setSongModelToView(orderSongs, ui->orderSongsListView);
    }

    downloadNext();
}

/**
 * 下一首播放（点歌队列置顶）
 */
void OrderPlayerWindow::appendNextSongs(SongList songs)
{
    QPoint center = ui->listTabWidget->tabBar()->tabRect(LISTTAB_ORDER).center();
    foreach (Song song, songs)
    {
        if (orderSongs.contains(song))
            orderSongs.removeOne(song);
        orderSongs.insert(0, song);
        addDownloadSong(song);
        showTabAnimation(center, "+1");
    }
    saveSongList("music/order", orderSongs);
    setSongModelToView(orderSongs, ui->orderSongsListView);

    if (isNotPlaying() && songs.size())
    {
        qDebug() << "当前未播放，开始播放本首歌";
        startPlaySong(songs.first());
    }

    downloadNext();
}

/**
 * 立刻开始播放音乐
 */
void OrderPlayerWindow::playLocalSong(Song song)
{
    qDebug() << "开始播放：" << song.simpleString();
    if (!isSongDownloaded(song))
    {
        qDebug() << "error: 未下载歌曲" << song.simpleString() << "开始下载";
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
    ui->playingAllTimeLabel->setText(msecondToString(song.duration));
    // 设置封面
    if (QFileInfo(coverPath(song)).exists())
    {
        QPixmap pixmap(coverPath(song), "1"); // 这里读取要加个参数，原因未知……
        if (pixmap.isNull())
            qDebug() << "warning: 本地封面是空的" << song.simpleString() << coverPath(song);
        pixmap = pixmap.scaledToHeight(ui->playingCoverLabel->height());
        ui->playingCoverLabel->setPixmap(pixmap);
        setCurrentCover(pixmap);
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
    QString url = API_DOMAIN + "/song/url?id=" + snum(song.id);
    qDebug() << "获取歌曲信息：" << song.simpleString();
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray baData = reply->readAll();
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(baData, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        if (json.value("code").toInt() != 200)
        {
            qDebug() << ("返回结果不为200：") << json.value("message").toString();
            return ;
        }

        QJsonArray array = json.value("data").toArray();
        if (!array.size())
        {
            qDebug() << "未找到歌曲：" << song.simpleString();
            downloadingSong = Song();
            downloadNext();
            return ;
        }

        json = array.first().toObject();
        QString url = JVAL_STR(url);
        int br = JVAL_INT(br); // 比率320000
        int size = JVAL_INT(size);
        QString type = JVAL_STR(type); // mp3
        QString encodeType = JVAL_STR(encodeType); // mp3
        qDebug() << "    信息：" << br << size << type << url;
        if (size == 0)
        {
            qDebug() << "无法下载，可能没有版权" << song.simpleString();
            if (playAfterDownloaded == song)
            {
                if (orderSongs.contains(song))
                {
                    orderSongs.removeOne(song);
                }
                playNext();
            }

            downloadingSong = Song();
            downloadNext();
            return ;
        }

        // 开始下载歌曲本身
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply1 = manager.get(QNetworkRequest(QUrl(url)));
        //请求结束并下载完成后，退出子事件循环
        connect(reply1, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        //开启子事件循环
        loop.exec();
        QByteArray baData1 = reply1->readAll();

        QFile file(songPath(song));
        file.open(QIODevice::WriteOnly);
        file.write(baData1);
        file.flush();
        file.close();

        emit signalSongDownloadFinished(song);

        if (playAfterDownloaded == song)
            playLocalSong(song);

        downloadingSong = Song();
        downloadNext();
    });
    manager->get(*request);

    downloadSongLyric(song);
    downloadSongCover(song);
}

void OrderPlayerWindow::downloadSongLyric(Song song)
{
    if (QFileInfo(lyricPath(song)).exists())
        return ;

    downloadingSong = song;
    QString url = API_DOMAIN + "/lyric?id=" + snum(song.id);
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray baData = reply->readAll();
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(baData, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        if (json.value("code").toInt() != 200)
        {
            qDebug() << ("返回结果不为200：") << json.value("message").toString();
            return ;
        }

        QString lrc = json.value("lrc").toObject().value("lyric").toString();
        if (!lrc.isEmpty())
        {
            QFile file(lyricPath(song));
            file.open(QIODevice::WriteOnly);
            QTextStream stream(&file);
            stream << lrc;
            file.flush();
            file.close();

            qDebug() << "下载歌词完成：" << song.simpleString();
            if (playAfterDownloaded == song || playingSong == song)
            {
                setCurrentLyric(lrc);
            }

            emit signalLyricDownloadFinished(song);
        }
        else
        {
            qDebug() << "warning: 下载的歌词是空的" << song.simpleString();
        }
    });
    manager->get(*request);
}

void OrderPlayerWindow::downloadSongCover(Song song)
{
    if (QFileInfo(coverPath(song)).exists())
        return ;
    downloadingSong = song;
    QString url = API_DOMAIN + "/song/detail?ids=" + snum(song.id);
    QNetworkAccessManager* manager = new QNetworkAccessManager;
    QNetworkRequest* request = new QNetworkRequest(url);
    request->setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded; charset=UTF-8");
    request->setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/86.0.4240.111 Safari/537.36");
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        QByteArray baData = reply->readAll();
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(baData, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qDebug() << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        if (json.value("code").toInt() != 200)
        {
            qDebug() << ("返回结果不为200：") << json.value("message").toString();
            return ;
        }

        QJsonArray array = json.value("songs").toArray();
        if (!array.size())
        {
            qDebug() << "未找到歌曲：" << song.simpleString();
            downloadingSong = Song();
            downloadNext();
            return ;
        }

        json = array.first().toObject();
        QString url = json.value("al").toObject().value("picUrl").toString();
        qDebug() << "封面地址：" << url;

        // 开始下载歌曲本身
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply1 = manager.get(QNetworkRequest(QUrl(url)));
        //请求结束并下载完成后，退出子事件循环
        connect(reply1, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        //开启子事件循环
        loop.exec();
        QByteArray baData1 = reply1->readAll();
        QPixmap pixmap;
        pixmap.loadFromData(baData1);
        if (!pixmap.isNull())
        {
            QFile file(coverPath(song));
            file.open(QIODevice::WriteOnly);
            file.write(baData1);
            file.flush();
            file.close();

            emit signalCoverDownloadFinished(song);

            // 正是当前要播放的歌曲
            if (playAfterDownloaded == song || playingSong == song)
            {
                pixmap = pixmap.scaledToHeight(ui->playingCoverLabel->height());
                ui->playingCoverLabel->setPixmap(pixmap);
                setCurrentCover(pixmap);
            }
        }
        else
        {
            qDebug() << "warning: 下载的封面是空的" << song.simpleString();
        }
    });
    manager->get(*request);
}

/**
 * 设置当前歌曲的歌词到歌词控件
 */
void OrderPlayerWindow::setCurrentLyric(QString lyric)
{
    desktopLyric->setLyric(lyric);
    ui->lyricWidget->setLyric(lyric);
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

        desktopLyric = new DesktopLyricWidget(nullptr);
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
    int addin = rgbSum * blurAlpha / (255*3*m*m);

    // 半透明
    currentBlurBg = clip;
    currentBgAlpha = qMin(255, blurAlpha + addin);

    // 出现动画
    QPropertyAnimation* ani1 = new QPropertyAnimation(this, "appearBgProg");
    ani1->setStartValue(0);
    ani1->setEndValue(currentBgAlpha);
    ani1->setDuration(1000);
    ani1->setEasingCurve(QEasingCurve::OutCubic);
    connect(ani1, &QPropertyAnimation::valueChanged, this, [=](const QVariant& val){
        update();
    });
    connect(ani1, &QPropertyAnimation::finished, this, [=]{
        ani1->deleteLater();
    });
    currentBgAlpha = 0;
    ani1->start();

    // 消失动画
    QPropertyAnimation* ani2 = new QPropertyAnimation(this, "disappearBgProg");
    ani2->setStartValue(prevBgAlpha);
    ani2->setEndValue(0);
    ani2->setDuration(1000);
    ani2->setEasingCurve(QEasingCurve::OutCubic);
    connect(ani2, &QPropertyAnimation::valueChanged, this, [=](const QVariant& val){
        prevBgAlpha = val.toInt();
        update();
    });
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
        pa.setColor(QPalette::HighlightedText, fg);

        pa.setColor(QPalette::Highlight, sbg);

        QApplication::setPalette(pa);
        setPalette(pa);

        ui->lyricWidget->setColors(sfg, fg);
        desktopLyric->setColors(sfg, fg);
        ui->playingNameLabel->setPalette(pa);
        ui->titleButton->setPalette(pa);
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

    qDebug() << "当前颜色：" << bg << fg << sfg;
}

/**
 * 列表项改变
 */
void OrderPlayerWindow::on_listTabWidget_currentChanged(int index)
{
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
        playingSong = Song();
        settings.setValue("music/currentSong", "");
        ui->playingNameLabel->clear();
        ui->playingArtistLabel->clear();
        ui->playingCoverLabel->clear();

        // 下一首歌，没有就不放
        playNext();
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
    if (row > -1)
        currentSong = orderSongs.at(row);

    FacileMenu* menu = new FacileMenu(this);

    menu->addAction("立即播放", [=]{
        Song song = orderSongs.takeAt(row);
        startPlaySong(song);
        orderSongs.removeOne(song);
        saveSongList("music/order", orderSongs);
        setSongModelToView(orderSongs, ui->orderSongsListView);
    })->disable(songs.size() != 1 || !currentSong.isValid());

    menu->addAction("下一首播放", [=]{
        foreach (Song song, songs)
        {
            orderSongs.removeOne(song);
        }
        for (int i = songs.size()-1; i >= 0; i--)
        {
            orderSongs.insert(0, songs.at(i));
        }
        saveSongList("music/order", orderSongs);
        setSongModelToView(orderSongs, ui->orderSongsListView);
    })->disable(!songs.size());

    menu->split()->addAction("添加常时播放", [=]{
        foreach (Song song, songs)
        {
            normalSongs.removeOne(song);
        }
        for (int i = songs.size()-1; i >= 0; i--)
        {
            normalSongs.insert(0, songs.at(i));
        }
        saveSongList("music/normal", normalSongs);
        setSongModelToView(normalSongs, ui->normalSongsListView);
    })->disable(!currentSong.isValid());

    menu->addAction("收藏", [=]{
        if (!favoriteSongs.contains(currentSong))
            addFavorite(songs);
        else
            removeFavorite(songs);
    })->disable(!currentSong.isValid())
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
    if (row > -1)
        currentSong = favoriteSongs.at(row);

    FacileMenu* menu = new FacileMenu(this);

    menu->addAction("立即播放", [=]{
        Song song = favoriteSongs.takeAt(row);
        startPlaySong(song);
    })->disable(songs.size() != 1 || !currentSong.isValid());

    menu->split()->addAction("下一首播放", [=]{
        appendNextSongs(songs);
    })->disable(!songs.size());

    menu->addAction("添加到播放列表", [=]{
        appendOrderSongs(songs);
    })->disable(!songs.size());

    menu->addAction("添加常时播放", [=]{
        addNormal(songs);
    })->disable(!currentSong.isValid());

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

    menu->exec();
}

void OrderPlayerWindow::on_listSongsListView_customContextMenuRequested(const QPoint &pos)
{
    auto indexes = ui->favoriteSongsListView->selectionModel()->selectedRows(0);
    int row = ui->listSongsListView->currentIndex().row();
    PlayList pl;
    if (row > -1)
        ;

    FacileMenu* menu = new FacileMenu(this);

    menu->addAction("加载歌单", [=]{

    });

    menu->addAction("播放里面歌曲", [=]{

    });

    menu->split()->addAction("删除歌单", [=]{

    });

    menu->exec();
}

void OrderPlayerWindow::on_normalSongsListView_customContextMenuRequested(const QPoint &pos)
{
    auto indexes = ui->normalSongsListView->selectionModel()->selectedRows(0);
    SongList songs;
    foreach (auto index, indexes)
        songs.append(normalSongs.at(index.row()));
    int row = ui->normalSongsListView->currentIndex().row();
    Song currentSong;
    if (row > -1)
        currentSong = normalSongs.at(row);

    FacileMenu* menu = new FacileMenu(this);

    menu->addAction("立即播放", [=]{
        Song song = normalSongs.takeAt(row);
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
    })->disable(!currentSong.isValid())
            ->text(favoriteSongs.contains(currentSong), "从收藏中移除", "添加到收藏");

    menu->split()->addAction("上移", [=]{
        normalSongs.swapItemsAt(row, row-1);
        saveSongList("music/order", normalSongs);
        setSongModelToView(normalSongs, ui->normalSongsListView);
    })->disable(songs.size() != 1 || row < 1);

    menu->addAction("下移", [=]{
        normalSongs.swapItemsAt(row, row+1);
        saveSongList("music/order", normalSongs);
        setSongModelToView(normalSongs, ui->normalSongsListView);
    })->disable(songs.size() != 1 || row >= normalSongs.size()-1);

    menu->split()->addAction("移出常时播放", [=]{
        removeNormal(songs);
    })->disable(!songs.size());

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
    if (row > -1)
        currentSong = historySongs.at(row);

    FacileMenu* menu = new FacileMenu(this);

    menu->addAction("立即播放", [=]{
        Song song = historySongs.takeAt(row);
        startPlaySong(song);
    })->disable(songs.size() != 1 || !currentSong.isValid());

    menu->split()->addAction("下一首播放", [=]{
        appendNextSongs(songs);
    })->disable(!songs.size());

    menu->addAction("添加到播放列表", [=]{
        appendOrderSongs(songs);
    })->disable(!songs.size());

    menu->addAction("添加常时播放", [=]{
        addNormal(songs);
    })->disable(!currentSong.isValid());

    menu->addAction("收藏", [=]{
        if (!favoriteSongs.contains(currentSong))
            addFavorite(songs);
        else
            removeFavorite(songs);
    })->disable(!currentSong.isValid())
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
    })->disable(!currentSong.isValid());

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

void OrderPlayerWindow::on_splitter_splitterMoved(int pos, int index)
{
    adjustExpandPlayingButton();
}

void OrderPlayerWindow::on_titleButton_clicked()
{
    QString text = settings.value("music/title").toString();
    bool ok;
    QString rst = QInputDialog::getText(this, "请输入名称", "显示在左上角，不影响其他任何效果", QLineEdit::Normal, text, &ok);
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
    menu->addAction("模糊背景", [=]{
        settings.setValue("music/blurBg", blurBg = !blurBg);
        if (blurBg)
            setBlurBackground(currentCover);
        update();
    })->setChecked(blurBg);
    QStringList sl{"32", "64", "96", "128"/*, "160", "192", "224", "256"*/};
    auto blurAlphaMenu = menu->addMenu("模糊透明度");
    menu->lastAction()->hide(!blurBg);
    blurAlphaMenu->addOptions(sl, blurAlpha / 32 - 1, [=](int index){
        blurAlpha = (index+1) * 32;
        settings.setValue("music/blurAlpha", blurAlpha);
        setBlurBackground(currentCover);
    });
    menu->split()->addAction("主题变色", [=]{
        settings.setValue("music/themeColor", themeColor = !themeColor);
        if (themeColor)
            setThemeColor(currentCover);
        update();
    })->setChecked(themeColor);

    menu->exec();
}
