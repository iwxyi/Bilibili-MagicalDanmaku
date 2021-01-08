#ifndef ORDERPLAYERWINDOW_H
#define ORDERPLAYERWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSettings>
#include <QDebug>
#include <QApplication>
#include <QClipboard>
#include <QNetworkCookie>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextCodec>
#include <stdio.h>
#include <iostream>
#include <QtWebSockets/QWebSocket>
#include <QAuthenticator>
#include <QtConcurrent/QtConcurrent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
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
            bfs.v[i] = this->v[i] * prop;
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
    ~OrderPlayerWindow() override;

    enum PlayCircleMode
    {
        OrderList,
        SingleCircle
    };

    bool hasSongInOrder(QString by);

public slots:
    void slotSearchAndAutoAppend(QString key, QString by = "");

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

    void on_listSongsListView_customContextMenuRequested(const QPoint &pos);

    void on_normalSongsListView_customContextMenuRequested(const QPoint &pos);

    void on_orderSongsListView_activated(const QModelIndex &index);

    void on_favoriteSongsListView_activated(const QModelIndex &index);

    void on_normalSongsListView_activated(const QModelIndex &index);

    void on_historySongsListView_activated(const QModelIndex &index);

    void on_desktopLyricButton_clicked();

    void slotExpandPlayingButtonClicked();

    void slotPlayerPositionChanged();

    void on_splitter_splitterMoved(int pos, int index);

    void on_titleButton_clicked();

    void adjustCurrentLyricTime(QString lyric);

    void on_settingsButton_clicked();

private:
    void searchMusic(QString key);
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

    void startPlaySong(Song song);
    void playNext();
    void appendOrderSongs(SongList songs);
    void appendNextSongs(SongList songs);

    void playLocalSong(Song song);
    void addDownloadSong(Song song);
    void downloadNext();
    void downloadSong(Song song);
    void downloadSongLyric(Song song);
    void downloadSongCover(Song song);
    void setCurrentLyric(QString lyric);

    void adjustExpandPlayingButton();
    void connectDesktopLyricSignals();
    void setCurrentCover(const QPixmap& pixmap);
    void setBlurBackground(const QPixmap& bg);
    void setThemeColor(const QPixmap& cover);

    void readMp3Data(const QByteArray& array);

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
    void signalOrderSongSucceed(Song song, qint64 msecond, int waiting);
    void signalOrderSongPlayed(Song song);
    void signalWindowClosed();

private:
    Ui::OrderPlayerWindow *ui;
    QSettings settings;
    QDir musicsFileDir;
    const QString API_DOMAIN = "http://iwxyi.com:3000/";
    SongList searchResultSongs;
    PlayListList searchResultPlayLists;

    QStringList orderBys;

    SongList orderSongs;
    SongList favoriteSongs;
    SongList normalSongs;
    SongList historySongs;
    PlayListList myPlayLists;
    SongList toDownloadSongs;
    Song playAfterDownloaded;
    Song downloadingSong;

    QMediaPlayer* player;
    PlayCircleMode circleMode = OrderList;
    Song playingSong;
    QTimer* playingPositionTimer;
    int lyricScroll;

    bool doubleClickToPlay = false; // 双击是立即播放，还是添加到列表
    qint64 setPlayPositionAfterLoad = 0; // 加载后跳转到时间

    DesktopLyricWidget* desktopLyric;
    InteractiveButtonBase* expandPlayingButton;

    bool blurBg = true;
    int blurAlpha = 32;
    QPixmap currentCover;
    int currentBgAlpha = 255;
    QPixmap currentBlurBg;
    QPixmap prevBlurBg;
    int prevBgAlpha = 0;

    bool themeColor = false;
    BFSColor prevPa;
    BFSColor currentPa;
    double paletteAlpha;
};

class NoFocusDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    NoFocusDelegate(){}
    ~NoFocusDelegate(){}

    void paint(QPainter* painter, const QStyleOptionViewItem & option, const QModelIndex &index) const
    {
        QStyleOptionViewItem itemOption(option);
        if (itemOption.state & QStyle::State_Selected)
        {
            painter->fillRect(option.rect, QApplication::palette().color(QPalette::Highlight));
            /*int radius = option.rect.height() / 2;
            QPainterPath path;
            path.addRoundedRect(option.rect, radius, radius);
            painter->fillPath(path, QColor(100, 149, 237, 128));*/
        }
        QRect rect = option.rect;
        rect.setLeft(rect.left() + 4);
        painter->setPen(QApplication::palette().color(QPalette::Text));
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, index.data(Qt::DisplayRole).toString());
    }
};

#endif // ORDERPLAYERWINDOW_H
