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
#include "livedanmaku.h"

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
    void showMenu();

private:
    QSettings settings;
    QListWidget* listWidget;

    QColor fgColor;
    QColor bgColor;

    int fontHeight;
    int lineSpacing;
    int boundaryWidth = 8;
    int boundaryShowed = 2;
    QPoint pressPos;
};

#endif // LIVEDANMAKUWINDOW_H
