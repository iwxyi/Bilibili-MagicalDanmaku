#ifndef LIVEDANMAKUWINDOW_H
#define LIVEDANMAKUWINDOW_H

#include <QWidget>
#include <windows.h>
#include <windowsx.h>
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
#include "livedanmaku.h"
#include "netutil.h"
#include "freecopyedit.h"
#include "qxtglobalshortcut.h"
#include "portraitlabel.h"
#include "commonvalues.h"

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

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
    friend class MainWindow;
public:
    LiveDanmakuWindow(QWidget *parent = nullptr);

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
    void signalAddBlockUser(qint64 uid, int hour);
    void signalDelBlockUser(qint64 uid);
    void signalChangeWindowMode();

public slots:
    void slotNewLiveDanmaku(LiveDanmaku danmaku);
    void slotOldLiveDanmakuRemoved(LiveDanmaku danmaku);
    void setItemWidgetText(QListWidgetItem* item);
    void highlightItemText(QListWidgetItem* item, bool recover = false);
    void resetItemsTextColor();
    void resetItemsText();

    void showMenu();

    void setAutoTranslate(bool trans);
    void startTranslate(QListWidgetItem* item);
    void setAIReply(bool reply);
    void startReply(QListWidgetItem* item);
    void setEnableBlock(bool enable);
    void setListWidgetItemSpacing(int x);
    void setNewbieTip(bool tip);

    void showFastBlock(qint64 uid, QString msg);

    void showStatusText();
    void setStatusText(QString text);
    void hideStatusText();

private:
    bool isItemExist(QListWidgetItem *item);
    PortraitLabel* getItemWidgetPortrait(QListWidgetItem *item);
    QLabel* getItemWidgetLabel(QListWidgetItem *item);
    void adjustItemTextDynamic(QListWidgetItem* item);
    void getUserInfo(qint64 uid, QListWidgetItem *item);
    void getUserHeadPortrait(qint64 uid, QString url, QListWidgetItem *item);
    void showUserMsgHistory(qint64 uid, QString title);
    QString getPinyin(QString text);

private:
    QSettings settings;
    QListWidget* listWidget;
    TransparentEdit* lineEdit;
    QxtGlobalShortcut* editShortcut;
#ifdef Q_OS_WIN32
    HWND prevWindow = nullptr;
#endif
    QString myPrevSendMsg; // 上次发送的内容，没有发送成功的话自动填充

    QColor nameColor;
    QColor msgColor;
    QColor bgColor;
    QColor hlColor;
    bool autoTrans = true;
    bool aiReply = false;
    bool enableBlock = false;
    bool newbieTip = true;
    QHash<qint64, QPixmap> headPortraits;

    int fontHeight;
    int lineSpacing;
    int boundaryWidth = 8;
    int boundaryShowed = 2;
    QPoint pressPos;
    int listItemSpacing = 6;

    QLabel* statusLabel = nullptr;
};

#endif // LIVEDANMAKUWINDOW_H
