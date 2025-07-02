#ifndef DESKTOPLYRICWIDGET_H
#define DESKTOPLYRICWIDGET_H

#include <QWidget>
#ifdef Q_OS_WIN
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
#include "facilemenu.h"
#include "qt_compat_event.h"

struct LyricBean
{
    qint64 start = 0;
    qint64 end = 0;
    QString text;

    LyricBean(){}
    LyricBean(bool):start(-1),end(-1){}
};

typedef QList<LyricBean> LyricStream;

class DesktopLyricWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DesktopLyricWidget(QSettings &settings, QWidget *parent = nullptr);

    enum LineMode
    {
        SuitableLine,
        SingleLine,
        DoubleLine
    };

    enum AlignMode
    {
        AlignMid,
        AlignWidth,
        AlignLeft,
        AlignRight,
    };

    void setLyric(QString text);

    void setColors(QColor p, QColor w);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#else
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void resizeEvent(QResizeEvent *) override;
    void paintEvent(QPaintEvent *) override;

signals:
    void signalhide();
    void signalSwitchTrans();

public slots:
    void showMenu();
    void setPosition(qint64 position);

private:
    QSettings& settings;

    // 几何数值
    int fontHeight;
    int lineSpacing;
    int boundaryWidth = 8;
    int boundaryShowed = 2;
    QPoint pressPos;
    bool hovering = false;

    // 绘制设定
    LineMode lineMode = DoubleLine; // 单行还是双行
    AlignMode alignMode = AlignLeft; // 0左 1中 2右 3分散
    QColor playingColor = QColor(155, 80, 255);
    QColor waitingColor = QColor(255, 155, 155);
    QColor bgColor = QColor(0xf0, 0xf0, 0xf0);

    // 歌词显示
    LyricStream lyricStream;
    int currentRow = -1; // 当前播放或即将播放的歌词row（不一定有下一行）
    int pointSize = 20;
    QString fontFamily;

    bool jiWindow = false;
};

#endif // DESKTOPLYRICWIDGET_H
