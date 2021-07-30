#ifndef ORDERPLAYERWINDOW_H
#define ORDERPLAYERWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSettings>
#include <QDebug>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextCodec>
#include <stdio.h>
#include <iostream>
#include <QtWebSockets/QWebSocket>
#include <QAuthenticator>
#include <QtConcurrent/QtConcurrent>
#include <QStyledItemDelegate>
#include <QListView>
#include <QStringListModel>
#include <QScrollBar>
#include <QMediaPlayer>
#include "facilemenu.h"
#include "songbeans.h"
#include "desktoplyricwidget.h"
#include "numberanimation.h"
#include "imageutil.h"
#include "logindialog.h"
#include "netinterface.h"

QT_BEGIN_NAMESPACE
namespace Ui { class OrderPlayerWindow; }
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE
    extern Q_WIDGETS_EXPORT void qt_blurImage( QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0 );
QT_END_NAMESPACE

#define LISTTAB_ORDER 0
#define LISTTAB_NORMAL 1
#define LISTTAB_FAVORITE 2
#define LISTTAB_PLAYLIST 3
#define LISTTAB_HISTORY 3

#define MUSIC_DEB if (1) qDebug()

enum MusicQuality
{
    NormalQuality, // MP3普通品质
    HightQuality, // MP3高品质
    ApeQuality, // 高品无损
    FlacQuality // 无损
};

struct BFSColor
{
    int v[12] = {0};

    BFSColor()
    {}

    BFSColor(QList<QColor> cs)
    {
        Q_ASSERT(cs.size() == 4);
        for (int i = 0; i < 4; i++)
        {
            QColor c = cs[i];
            v[i*3+0] = c.red();
            v[i*3+1] = c.green();
            v[i*3+2] = c.blue();
        }
    }

    BFSColor operator-(const BFSColor& ano)
    {
        BFSColor bfs;
        for (int i = 0; i < 12; i++)
            bfs.v[i] = this->v[i] - ano.v[i];
        return bfs;
    }

    BFSColor operator+(const BFSColor& ano)
    {
        BFSColor bfs;
        for (int i = 0; i < 12; i++)
            bfs.v[i] = this->v[i] + ano.v[i];
        return bfs;
    }

    BFSColor operator*(const double& prop)
    {
        BFSColor bfs;
        for (int i = 0; i < 12; i++)
            bfs.v[i] = int(this->v[i] * prop);
        return bfs;
    }

    static BFSColor fromPalette(QPalette pa)
    {
        QColor bg = pa.color(QPalette::Window);
        QColor fg = pa.color(QPalette::Text);
        QColor sbg = pa.color(QPalette::Highlight);
        QColor sfg = pa.color(QPalette::HighlightedText);
        return BFSColor(QList<QColor>{bg, fg, sbg, sfg});
    }

    void toColors(QColor* bg, QColor* fg, QColor* sbg, QColor* sfg)
    {
        *bg = QColor(v[0], v[1], v[2]);
        *fg = QColor(v[3], v[4], v[5]);
        *sbg = QColor(v[6], v[7], v[8]);
        *sfg = QColor(v[9], v[10], v[11]);
    }
};

class OrderPlayerWindow : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(int lyricScroll READ getLyricScroll WRITE setLyricScroll)
    Q_PROPERTY(int disappearBgProg READ getDisappearBgProg WRITE setDisappearBgProg)
    Q_PROPERTY(int appearBgProg READ getAppearBgProg WRITE setAppearBgProg)
    Q_PROPERTY(double paletteProg READ getPaletteBgProg WRITE setPaletteBgProg)
public:
    OrderPlayerWindow(QWidget *parent = nullptr);
    OrderPlayerWindow(QString dataPath, QWidget* parent = nullptr);
    ~OrderPlayerWindow() override;

    enum PlayCircleMode
    {
        OrderList,
        SingleCircle
    };

    const Song& getPlayingSong() const;
    const SongList& getOrderSongs() const;
    const QStringList getSongLyrics(int rowCount) const;
    int userOrderCount(QString by);
    const QPixmap getCurrentSongCover() const;

public slots:
    void slotSearchAndAutoAppend(QString key, QString by = "");
    void improveUserSongByOrder(QString username, int promote);
    void cutSongIfUser(QString username);
    void cutSong();

private slots:
    void on_searchEdit_returnPressed();

    void on_searchButton_clicked();

    void on_searchResultTable_cellActivated(int row, int);

    void on_searchResultTable_customContextMenuRequested(const QPoint &);

    void on_listTabWidget_currentChanged(int index);

    void sortSearchResult(int col);

    void on_playProgressSlider_sliderReleased();

    void on_playProgressSlider_sliderMoved(int position);

    void on_volumeSlider_sliderMoved(int position);

    void on_playButton_clicked();

    void on_volumeButton_clicked();

    void on_circleModeButton_clicked();

    void slotSongPlayEnd();

    void on_orderSongsListView_customContextMenuRequested(const QPoint &);

    void on_favoriteSongsListView_customContextMenuRequested(const QPoint &);

    void on_historySongsListView_customContextMenuRequested(const QPoint &);

    void on_listSongsListView_customContextMenuRequested(const QPoint &);

    void on_normalSongsListView_customContextMenuRequested(const QPoint &);

    void on_orderSongsListView_activated(const QModelIndex &index);

    void on_favoriteSongsListView_activated(const QModelIndex &index);

    void on_normalSongsListView_activated(const QModelIndex &index);

    void on_historySongsListView_activated(const QModelIndex &index);

    void on_desktopLyricButton_clicked();

    void slotExpandPlayingButtonClicked();

    void slotPlayerPositionChanged();

    void on_splitter_splitterMoved(int, int);

    void on_titleButton_clicked();

    void adjustCurrentLyricTime(QString lyric);

    void on_settingsButton_clicked();

    void on_musicSourceButton_clicked();

    void on_nextSongButton_clicked();

private:
    void searchMusic(QString key, QString addBy = QString(), bool notify = false);
    void searchMusicBySource(QString key, MusicSource source, QString addBy = QString());
    void setSearchResultTable(SongList songs);
    void setSearchResultTable(PlayListList playLists);
    void addFavorite(SongList songs);
    void removeFavorite(SongList songs);
    void addNormal(SongList songs);
    void removeNormal(SongList songs);
    void removeOrder(SongList songs);
    void saveSongList(QString key, const SongList &songs);
    void restoreSongList(QString key, SongList& songs);
    void setSongModelToView(const SongList& songs, QListView* listView);
    QString songPath(const Song &song) const;
    QString lyricPath(const Song &song) const;
    QString coverPath(const Song &song) const;
    bool isSongDownloaded(Song song);
    QString msecondToString(qint64 msecond);
    void activeSong(Song song);
    bool isNotPlaying() const;
    Song getSuiableSong(QString key) const;

    void startPlaySong(Song song);
    void playNext();
    void appendOrderSongs(SongList songs);
    void appendNextSongs(SongList songs);

    void playLocalSong(Song song);
    void addDownloadSong(Song song);
    void downloadNext();
    void downloadSong(Song song);
    void downloadSongFailed(Song song);
    void downloadSongMp3(Song song, QString url);
    void downloadSongLyric(Song song);
    void downloadSongCover(Song song);
    void downloadSongCoverJpg(Song song, QString url);
    void setCurrentLyric(QString lyric);
    void inputPlayList();
    void openPlayList(QString shareUrl);
    void openMultiImport();
    void clearDownloadFiles();
    void clearHoaryFiles();

    void playNextRandomSong();
    void generalRandomSongList();

    void adjustExpandPlayingButton();
    void connectDesktopLyricSignals();
    void setCurrentCover(const QPixmap& pixmap);
    void setBlurBackground(const QPixmap& bg);
    void startBgAnimation(int duration = 2000);
    void setThemeColor(const QPixmap& cover);

    void readMp3Data(const QByteArray& array);

    void setMusicIconBySource();
    bool switchSource(Song song, bool play = false);

    void fetch(QString url, NetStringFunc func, MusicSource cookie = UnknowMusic);
    void fetch(QString url, NetJsonFunc func, MusicSource cookie = UnknowMusic);
    void fetch(QString url, NetReplyFunc func, MusicSource cookie = UnknowMusic);
    void fetch(QString url, QStringList params, NetJsonFunc func, MusicSource cookie = UnknowMusic);
    QVariant getCookies(QString cookieString);

    void getNeteaseAccount();
    void getQQMusicAccount();

    void importSongs(const QStringList& lines);
    void importNextSongByName();

protected:
    void showEvent(QShowEvent*e) override;
    void closeEvent(QCloseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void paintEvent(QPaintEvent* e) override;

private:
    void setLyricScroll(int x);
    int getLyricScroll() const;
    void setAppearBgProg(int x);
    int getAppearBgProg() const;
    void setDisappearBgProg(int x);
    int getDisappearBgProg() const;
    void showTabAnimation(QPoint center, QString text);
    void setPaletteBgProg(double x);
    double getPaletteBgProg() const;

signals:
    void signalSongDownloadFinished(Song song);
    void signalLyricDownloadFinished(Song song);
    void signalCoverDownloadFinished(Song song);
    void signalSongPlayStarted(Song song);
    void signalSongPlayFinished(Song song);

    // 因为目标是作为插件，这些信号是给外面程序连接的
    void signalOrderSongSucceed(Song song, qint64 msecond, int waiting);
    void signalCurrentSongChanged(Song song);
    void signalOrderSongPlayed(Song song); // 仅点歌
    void signalOrderSongModified(const SongList& songs);
    void signalLyricChanged();
    void signalOrderSongNoCopyright(Song song);
    void signalOrderSongImproved(Song song, int previous, int current);
    void signalOrderSongCutted(Song song);
    void signalOrderSongStarted();
    void signalOrderSongEnded();

    void signalWindowClosed();

private:
    Ui::OrderPlayerWindow *ui;
    bool starting = true;

    // 配置
    QSettings settings;
    QDir musicsFileDir;
    QDir localMusicsFileDir;
    MusicSource musicSource = NeteaseCloudMusic;
    SongList searchResultSongs;
    PlayListList searchResultPlayLists;

    // 列表
    SongList orderSongs;
    SongList favoriteSongs;
    SongList normalSongs;
    SongList historySongs;
    PlayListList myPlayLists;
    SongList toDownloadSongs;
    Song playAfterDownloaded;
    Song downloadingSong;

    // 播放
    QMediaPlayer* player;
    PlayCircleMode circleMode = OrderList;
    Song playingSong;
    int lyricScroll;

    // 开关设置
    bool doubleClickToPlay = false; // 双击是立即播放，还是添加到列表
    qint64 setPlayPositionAfterLoad = 0; // 加载后跳转到时间
    bool accompanyMode = false; // 伴奏模式
    bool unblockQQMusic = false; // 试听QQ音乐

    // 控件
    DesktopLyricWidget* desktopLyric;
    InteractiveButtonBase* expandPlayingButton;

    // 封面
    Song coveringSong;
    bool blurBg = true;
    int blurAlpha = 32;
    QPixmap currentCover;
    int currentBgAlpha = 255;
    QPixmap currentBlurBg;
    QPixmap prevBlurBg;
    int prevBgAlpha = 0;

    // 主题
    bool themeColor = false;
    BFSColor prevPa;
    BFSColor currentPa;
    double paletteAlpha;

    // 点歌
    QString currentResultOrderBy; // 当前搜索结果是谁点的歌，用作替换
    Song prevOrderSong;
    bool autoSwitchSource = true; // 自动切换音源
    bool insertOrderOnce = false; // 插入到前面

    // 音乐账号
    int songBr = 320000; // 码率，单位bps
    QString neteaseCookies;
    QVariant neteaseCookiesVariant;
    QString qqmusicCookies;
    QVariant qqmusicCookiesVariant;
    QString kugouCookies;
    QVariant kugouCookiesVariant;
    QString neteaseNickname;
    QString qqmusicNickname;
    QString kugouMid = "1627614178416";

    // 算法
    QList<Song> randomSongList; // 洗牌算法的随机音乐

    // 导入
    int importFormat = 0;
    SongList importingSongNames; // 正在导入的歌曲队列（此时只有名字和歌手）
};

class NoFocusDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    NoFocusDelegate(){}
    NoFocusDelegate(QAbstractItemView* view, int columnCount = 1) : view(view), columnLast(columnCount-1)
    {
        if (view)
        {
            view->setItemDelegate(this);
            // 下面代码开启选中项圆角，奈何太丑了，关掉了
            // connect(view, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChanged()));
        }
    }

    ~NoFocusDelegate(){}

    void paint(QPainter* painter, const QStyleOptionViewItem & option, const QModelIndex &index) const
    {
        QStyleOptionViewItem itemOption(option);
        if (itemOption.state & QStyle::State_Selected)
        {
            if (selectTop == -1) // 没有选中，或者判断失误？
            {
                painter->fillRect(option.rect, option.palette.color(QPalette::Highlight));
            }
            else // 绘制圆角矩形
            {
                QRect rect = option.rect;
                const int radius = qMin(5, qMin(rect.width(), rect.height())/3);
                const int row = index.row();

                QPainterPath path;
                path.setFillRule(Qt::FillRule::WindingFill);
                if (columnLast == 0)
                {
                    if (row == selectTop)
                        path.addRoundedRect(rect.left(), rect.top(), rect.width()-1, radius*2, radius, radius);
                    else
                        path.addRect(rect.left(), rect.top(), rect.width()-1, radius*2);
                    if (row == selectBottom)
                        path.addRoundedRect(rect.left(), rect.bottom() - radius*2, rect.width()-1, radius*2, radius, radius);
                    else
                        path.addRect(rect.left(), rect.bottom() - radius*2, rect.width()-1, radius*2+1);
                    path.addRect(rect.left(), rect.top()+radius, rect.width()-1, rect.height()-radius*2);
                }
                else
                {
                    path.addRect(rect);

                    if (row == selectTop)
                    {
                        QPainterPath cornerPath;
                        if (index.column() == 0)
                        {
                            QPainterPath path1, path2;
                            path1.addRect(rect.left(), rect.top(), radius, radius);
                            path2.addEllipse(rect.left(), rect.top(), radius*2+2, radius*2+2);
                            path -= path1;
                            path += path2;
                        }
                        if (index.column() == columnLast)
                        {
                            QPainterPath path1, path2;
                            path1.addRect(rect.right()-radius, rect.top(), radius+1, radius);
                            path2.addEllipse(rect.right()-radius*2, rect.top(), radius*2, radius*2);
                            path -= path1;
                            path += path2;
                        }
                    }
                    if (row == selectBottom)
                    {
                        if (index.column() == 0)
                        {
                            QPainterPath path1, path2;
                            path1.addRect(rect.left(), rect.bottom()-radius, radius, radius+1);
                            path2.addEllipse(rect.left(), rect.bottom()-radius*2-2, radius*2+2, radius*2+2);
                            path -= path1;
                            path += path2;
                        }
                        if (index.column() == columnLast)
                        {
                            QPainterPath path1, path2;
                            path1.addRect(rect.right()-radius, rect.bottom()-radius, radius+1, radius+1);
                            path2.addEllipse(rect.right()-radius*2, rect.bottom()-radius*2, radius*2, radius*2);
                            path -= path1;
                            path += path2;
                        }
                    }
                }
                painter->setRenderHint(QPainter::Antialiasing);
                painter->fillPath(path, option.palette.color(QPalette::Highlight));
            }
        }

        QRect rect = option.rect;
        rect.setLeft(rect.left() + 4);
        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, index.data(Qt::DisplayRole).toString());
    }

private slots:
    void selectionChanged()
    {
        QList<QModelIndex> indexes = view->selectionModel()->selectedRows(0);
        if (!indexes.size())
        {
            selectTop = selectBottom = -1;
            return ;
        }
        else if (indexes.size() == 1)
        {
            selectTop = selectBottom = indexes.first().row();
            return ;
        }

        selectTop = view->model()->rowCount();
        selectBottom = -1;
        foreach (const QModelIndex index, indexes)
        {
            int row = index.row();
            if (row < selectTop)
                selectTop = row;
            if (row > selectBottom)
                selectBottom = row;
        }
    }

private:
    QAbstractItemView* view = nullptr;
    int columnLast = 1;
    int selectTop = -1;
    int selectBottom = -1;
};

#endif // ORDERPLAYERWINDOW_H
