#ifndef LIVEDANMAKUWINDOW_H
#define LIVEDANMAKUWINDOW_H

#include <QWidget>
#ifdef Q_OS_WIN32
#include <windows.h>
#include <windowsx.h>
#endif
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>
#include <QFontMetrics>
#include <QTimer>
#include <QListWidget>
#include <QVBoxLayout>
#include <QSettings>
#include <QMenu>
#include <QAction>
#include <QColorDialog>
#include <QLabel>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QLineEdit>
#include <QCryptographicHash>
#include <QScrollBar>
#include <QPropertyAnimation>
#include <QInputDialog>
#include <QDir>
#include <QPushButton>
#include <QStringListModel>
#include <QGraphicsDropShadowEffect>
#include <QNetworkCookie>
#include <QFontDialog>
#include "livedanmaku.h"
#include "netutil.h"
#include "freecopyedit.h"
#ifndef Q_OS_ANDROID
#include "qxtglobalshortcut.h"
#endif
#include "portraitlabel.h"
#include "commonvalues.h"
#include "eternalblockdialog.h"

#define DANMAKU_ANIMATION_ENABLED true

#define DANMAKU_JSON_ROLE Qt::UserRole
#define DANMAKU_STRING_ROLE Qt::UserRole+1
#define DANMAKU_TRANS_ROLE Qt::UserRole+3
#define DANMAKU_REPLY_ROLE Qt::UserRole+4
#define DANMAKU_HIGHLIGHT_ROLE Qt::UserRole+5

#define PORTRAIT_SIDE 24
#define DANMAKU_WIDGET_PORTRAIT 0
#define DANMAKU_WIDGET_LABEL 1

class LiveDanmakuWindow : public QWidget, public CommonValues
{
    Q_OBJECT
    Q_PROPERTY(int bgAlpha READ getBgAlpha WRITE setBgAlpha)
    Q_PROPERTY(int prevAlpha READ getPrevAlpha WRITE setPrevAlpha)
    friend class MainWindow;
public:
    LiveDanmakuWindow(QSettings &st, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void resizeEvent(QResizeEvent *) override;
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *event) override;

signals:
    void signalSendMsg(QString msg);
    void signalSendMsgToPk(QString msg);
    void signalAddBlockUser(qint64 uid, int hour);
    void signalDelBlockUser(qint64 uid);
    void signalEternalBlockUser(qint64 uid, QString uname);
    void signalCancelEternalBlockUser(qint64 uid);
    void signalChangeWindowMode();
    void signalAIReplyed(QString msg);
    void signalShowPkVideo();

public slots:
    void slotNewLiveDanmaku(LiveDanmaku danmaku);
    void slotOldLiveDanmakuRemoved(LiveDanmaku danmaku);
    void setItemWidgetText(QListWidgetItem* item);
    void highlightItemText(QListWidgetItem* item, bool recover = false);
    void resetItemsTextColor();
    void resetItemsText();
    void resetItemsFont();
    void mergeGift(LiveDanmaku danmaku);

    void showMenu();
    void showEditMenu();
    void showPkMenu();

    void setAutoTranslate(bool trans);
    void startTranslate(QListWidgetItem* item);
    void setAIReply(bool reply);
    void startReply(QListWidgetItem* item);
    void setEnableBlock(bool enable);
    void setListWidgetItemSpacing(int x);
    void setNewbieTip(bool tip);
    void setIds(qint64 uid, qint64 roomId);
    void markRobot(qint64 uid);

    void showFastBlock(qint64 uid, QString msg);

    void setPkStatus(int status, qint64 roomId, qint64 uid, QString pkUname);
    void showStatusText();
    void setStatusText(QString text);
    void setStatusTooltip(QString tooltip);
    void hideStatusText();

    void showFollowCountInAction(qint64 uid, QAction* action, QAction* action2 = nullptr);
    void showViewCountInAction(qint64 uid, QAction* action, QAction* action2 = nullptr, QAction* action3 = nullptr);
    void showGuardInAction(qint64 roomId, qint64 uid, QAction* action);
    void showPkLevelInAction(qint64 roomId, QAction* actionUser, QAction* actionRank);

    void releaseLiveData();

private:
    bool isItemExist(QListWidgetItem *item);
    PortraitLabel* getItemWidgetPortrait(QListWidgetItem *item);
    QLabel* getItemWidgetLabel(QListWidgetItem *item);
    void adjustItemTextDynamic(QListWidgetItem* item);
    void getUserInfo(qint64 uid, QListWidgetItem *item);
    void getUserHeadPortrait(qint64 uid, QString url, QListWidgetItem *item);
    void showUserMsgHistory(qint64 uid, QString title);
    QString getPinyin(QString text);
    QVariant getCookies();
    void selectBgPicture();
    QPixmap getBlurPixmap(QPixmap& bg);
    void drawPixmapCenter(QPainter& painter, const QPixmap& bgPixmap);

private:
    void setBgAlpha(int x);
    int getBgAlpha() const;
    void setPrevAlpha(int x);
    int getPrevAlpha() const;

private:
    QSettings& settings;
    QListWidget* listWidget;
    TransparentEdit* lineEdit;
#ifndef Q_OS_ANDROID
    QxtGlobalShortcut* editShortcut;
#endif
#ifdef Q_OS_WIN32
    HWND prevWindow = nullptr;
#endif
    QString myPrevSendMsg; // 上次发送的内容，没有发送成功的话自动填充
    qint64 roomId = 0;
    qint64 upUid = 0; // 当前主播的UID，用来显示主播标志

    QColor nameColor;
    QColor msgColor;
    QColor bgColor;
    QColor hlColor;
    QFont danmakuFont;
    bool autoTrans = true;
    bool aiReply = false;
    bool enableBlock = false;
    bool simpleMode = false; // 简约模式：不显示头像
    bool chatMode = false; // 聊天模式：只显示弹幕，并且不使用彩色
    bool newbieTip = true;
    QHash<qint64, QPixmap> headPortraits;
    QList<QString> ignoreDanmakuColors;

    int fontHeight;
    int lineSpacing;
    int boundaryWidth = 8;
    int boundaryShowed = 2;
    QPoint pressPos;
    int listItemSpacing = 6;

    int pkStatus = 0;
    QLabel* statusLabel = nullptr;
    qint64 pkRoomId = 0;
    qint64 pkUid = 0;
    QString pkUname;

    QPixmap originPixmap;
    QPixmap bgPixmap;
    QString pictureFilePath;
    QString pictureDirPath;
    QTimer* switchBgTimer = nullptr;
    int pictureAlpha;
    bool aspectRatio = false;
    int pictureBlur = 0;

    QPixmap prevPixmap;
    int bgAlpha = 0;
    int prevAlpha = 0;
};

#endif // LIVEDANMAKUWINDOW_H
