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
#include "livedanmaku.h"
#include "netutil.h"
#include "freecopyedit.h"

#define DANMAKU_JSON_ROLE Qt::UserRole
#define DANMAKU_STRING_ROLE Qt::UserRole+3

class LiveDanmakuWindow : public QWidget
{
    Q_OBJECT
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

signals:

public slots:
    void slotNewLiveDanmaku(LiveDanmaku danmaku);
    void slotOldLiveDanmakuRemoved(LiveDanmaku danmaku);
    void appendItemText(QListWidgetItem* item, QString text);
    void resetItemTextColor();
    void showMenu();
    void setAutoTranslate(bool trans);
    void startTranslate(QListWidgetItem* item);

private:
    QSettings settings;
    QListWidget* listWidget;

    QColor fgColor;
    QColor bgColor;
    bool autoTrans = true;

    int fontHeight;
    int lineSpacing;
    int boundaryWidth = 8;
    int boundaryShowed = 2;
    QPoint pressPos;
};

#endif // LIVEDANMAKUWINDOW_H
