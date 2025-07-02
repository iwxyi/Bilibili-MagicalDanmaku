#include <QFontDatabase>
#include <QComboBox>
#include "desktoplyricwidget.h"
#include "qt_compat.h"

DesktopLyricWidget::DesktopLyricWidget(QSettings& settings, QWidget *parent) : QWidget(parent),
    settings(settings)
{
    this->setWindowTitle("桌面歌词");
    this->setMinimumSize(45, 25);                        //设置最小尺寸
    this->setMaximumSize(10000, 10000);
    if ((jiWindow = settings.value("music/desktopLyricTrans", false).toBool()))
    {
        this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);      //设置为无边框置顶窗口
    }
    else
    {
        this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);      //设置为无边框置顶窗口
        this->setAttribute(Qt::WA_TranslucentBackground, true); // 设置窗口透明
    }
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setMouseTracking(true);

    QFontMetrics fm(this->font());
    fontHeight = fm.height();
    lineSpacing = fm.lineSpacing();

    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showMenu()));

    lineMode = static_cast<LineMode>(settings.value("music/lineMode", lineMode).toInt());
    alignMode = static_cast<AlignMode>(settings.value("music/alignMode", alignMode).toInt());
    playingColor = qvariant_cast<QColor>(settings.value("music/playingColor", playingColor));
    waitingColor = qvariant_cast<QColor>(settings.value("music/waitingColor", waitingColor));
    bgColor = qvariant_cast<QColor>(settings.value("music/bgColor", bgColor));
    pointSize = settings.value("music/desktopLyricPointSize", pointSize).toInt();
    fontFamily = settings.value("music/desktopLyricFontFamily", fontFamily).toString();
}

/**
 * 歌词文本设置成歌词流
 * 在这里解析格式
 */
void DesktopLyricWidget::setLyric(QString text)
{
    // 检测是不是全是毫秒还是10毫秒的
    int ms10x = 10;
    QRegularExpression re10("\\[\\d{2}:\\d{2}(?:\\.([1-9]\\d{2}))?\\]");
    QRegularExpressionMatch match10;
    if (text.lastIndexOf(re10, -1, &match10) != -1)
    {
        int val = match10.captured(1).toInt();
        if (val > 0) // 存在不为0的三位数
        {
            ms10x = 1;
        }
    }

    // 遍历每一行
    QStringList sl = text.split("\n", SKIP_EMPTY_PARTS);
    LyricBean prevLyric(false);
    qint64 currentTime = 0;
    lyricStream.clear();
    foreach (QString line, sl)
    {
        QRegularExpression re("^\\[(\\d{2}):(\\d{2})(?:\\.(\\d{2,3}))?\\](\\[(\\d{2}):(\\d{2})\\.(\\d{2,3})\\])?(.*)$");
        QRegularExpressionMatch match;
        if (line.indexOf(re, 0, &match) == -1) // 不是时间格式的，可能是其他标签
        {
            if (line.indexOf(QRegularExpression("^\\s*\\[.+:.*\\]\\s*$")) > -1) // 是标签，无视掉
                continue;
            else if (line.indexOf(QRegularExpression("\\[00:00:00\\](.+)"), 0, &match) > -1)
            {
                LyricBean lyric;
                lyric.start = 0;
                lyric.end = 0;
                lyric.text = match.captured(1);
                lyricStream.append(lyric);
                continue;
            }

            LyricBean lyric;
            lyric.start = currentTime;
            lyric.end = 0;
            lyric.text = line;
            lyricStream.append(lyric);
            continue;
        }
        QStringList caps = match.capturedTexts();
        LyricBean lyric;
        int minute = caps.at(1).toInt();
        int second = caps.at(2).toInt();
        int ms10 = caps.at(3).toInt();
        lyric.start = minute * 60000 + second*1000 + ms10 * ms10x;
        if (!caps.at(4).isEmpty()) // 有终止时间
        {
            int minute = caps.at(5).toInt();
            int second = caps.at(6).toInt();
            int ms10 = caps.at(7).toInt();
            lyric.end = minute * 60000 + second*1000 + ms10 * ms10x;
        }
        lyric.text = caps.at(8);
        lyricStream.append(lyric);
        currentTime = lyric.start;
    }

    currentRow = 0;
    update();
}

void DesktopLyricWidget::setColors(QColor p, QColor w)
{
    playingColor = p;
    waitingColor = w;
    update();
}

void DesktopLyricWidget::showEvent(QShowEvent *event)
{
    restoreGeometry(settings.value("music/desktopLyricGeometry").toByteArray());
}

void DesktopLyricWidget::hideEvent(QHideEvent *event)
{
    settings.setValue("music/desktopLyricGeometry", this->saveGeometry());
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
bool DesktopLyricWidget::nativeEvent(const QByteArray &eventType, void *message, long *result)
#else
bool DesktopLyricWidget::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
#endif
{
#if defined(Q_OS_WIN)
    Q_UNUSED(eventType)
    MSG* msg = static_cast<MSG*>(message);
    switch(msg->message)
    {
    case WM_NCHITTEST:
        const auto ratio = devicePixelRatioF(); // 解决4K下的问题
        int xPos = static_cast<int>(GET_X_LPARAM(msg->lParam) / ratio - this->frameGeometry().x());
        int yPos = static_cast<int>(GET_Y_LPARAM(msg->lParam) / ratio - this->frameGeometry().y());
        if(xPos < boundaryWidth &&yPos<boundaryWidth)                    //左上角
            *result = HTTOPLEFT;
        else if(xPos>=width()-boundaryWidth&&yPos<boundaryWidth)          //右上角
            *result = HTTOPRIGHT;
        else if(xPos<boundaryWidth&&yPos>=height()-boundaryWidth)         //左下角
            *result = HTBOTTOMLEFT;
        else if(xPos>=width()-boundaryWidth&&yPos>=height()-boundaryWidth)//右下角
            *result = HTBOTTOMRIGHT;
        else if(xPos < boundaryWidth)                                     //左边
            *result =  HTLEFT;
        else if(xPos>=width()-boundaryWidth)                              //右边
            *result = HTRIGHT;
        /*else if(yPos<boundaryWidth)                                       //上边
            *result = HTTOP;*/
        else if(yPos>=height()-boundaryWidth)                             //下边
            *result = HTBOTTOM;
        else              //其他部分不做处理，返回false，留给其他事件处理器处理
           return false;
        return true;
    }
#endif
    return false;         //此处返回false，留给其他事件处理器处理
}

void DesktopLyricWidget::enterEvent(QEnterEvent *event)
{
    hovering = true;
    update();
}

void DesktopLyricWidget::leaveEvent(QEvent *event)
{
    if (!this->geometry().contains(QCursor::pos()))
    {
        hovering = false;
    }
    else
    {
        QTimer::singleShot(300, [=]{
            if (!this->geometry().contains(QCursor::pos()))
            {
                hovering = false;
                update();
            }
        });
    }
    update();
}

void DesktopLyricWidget::mousePressEvent(QMouseEvent *e)
{
    if(e->button()==Qt::LeftButton)
    {
        pressPos = e->pos();
    }
    QWidget::mousePressEvent(e);
}

void DesktopLyricWidget::mouseMoveEvent(QMouseEvent *e)
{
    hovering = true;
    if(e->buttons()&Qt::LeftButton)
    {
        move(QCursor::pos() - pressPos);
    }
    QWidget::mouseMoveEvent(e);
}

void DesktopLyricWidget::resizeEvent(QResizeEvent *)
{

}

void DesktopLyricWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    // 绘制背景
    if (jiWindow)
    {
        painter.fillRect(this->rect(), bgColor);
    }
    if (hovering)
    {
        painter.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;
        path.addRoundedRect(this->rect(), 5, 5);
        painter.fillPath(path, QColor(64, 64, 64, 32));
    }

    // 绘制桌面歌词
    QFont font;
    font.setPointSize(pointSize);
    font.setBold(true);
    if (!fontFamily.isEmpty())
        font.setFamily(fontFamily);
    painter.setFont(font);

    // 文字区域
    QRect rect = this->rect();
    rect.setLeft(rect.left() + boundaryWidth);
    rect.setTop(rect.top() + boundaryWidth);
    rect.setRight(rect.right() - boundaryWidth);
    rect.setBottom(rect.bottom() - boundaryWidth);

    if (currentRow > -1 && currentRow < lyricStream.size())
    {
        bool cross = lineMode != SingleLine && currentRow % 2;

        // 绘制当前句
        if (currentRow + (cross ? 1 : 0) < lyricStream.size())
        {
            painter.setPen(cross ? waitingColor : playingColor);
            QFlags<Qt::AlignmentFlag> align;
            if (lineMode == SuitableLine || lineMode == DoubleLine)
                align = Qt::AlignTop;
            else if (lineMode == SingleLine)
                align = Qt::AlignVCenter;
            if (alignMode == AlignMid)
                align |= Qt::AlignHCenter;
            else if (alignMode == AlignRight)
                align |= Qt::AlignRight;
            else
                align |= Qt::AlignLeft;
            painter.drawText(rect, align, lyricStream.at(currentRow + (cross ? 1 : 0)).text);
        }

        // 绘制下一句
        if (currentRow - (cross ? 1 : 0) < lyricStream.size()-1 && (lineMode == SuitableLine || lineMode == DoubleLine))
        {
            painter.setPen(cross ? playingColor : waitingColor);
            QFlags<Qt::AlignmentFlag> align = Qt::AlignBottom;
            if (alignMode == AlignMid)
                align |= Qt::AlignHCenter;
            else if (alignMode == AlignLeft)
                align |= Qt::AlignLeft;
            else
                align |= Qt::AlignRight;

            painter.drawText(rect, align, lyricStream.at(currentRow + (cross ? 0 : 1)).text);
        }
    }
}

void DesktopLyricWidget::showMenu()
{
    FacileMenu* menu = new FacileMenu(this);
    auto lineMenu = menu->addMenu("行数");
    lineMenu->addOptions(QStringList{"自动", "单行", "双行"}, lineMode, [=](int index){
        lineMode = static_cast<LineMode>(index);
        settings.setValue("music/lineMode", lineMode);
        update();
    });
    auto alignMenu = menu->addMenu("对齐");
    alignMenu->addOptions(QStringList{"居中", "左右分离", "左对齐", "右对齐"}, alignMode, [=](int index){
        alignMode = static_cast<AlignMode>(index);
        settings.setValue("music/alignMode", alignMode);
        update();
    });
    menu->addAction("已播放颜色", [=]{
        QColor c = QColorDialog::getColor(playingColor, this, "选择已播放颜色", QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        if (c != playingColor)
        {
            settings.setValue("music/playingColor", playingColor = c);
            update();
        }
    })->fgColor(playingColor);
    menu->addAction("未播放颜色", [=]{
        QColor c = QColorDialog::getColor(waitingColor, this, "选择未播放颜色", QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        if (c != waitingColor)
        {
            settings.setValue("music/waitingColor", waitingColor = c);
            update();
        }
    })->fgColor(waitingColor);
    menu->addAction("背景颜色", [=]{
        QColor c = QColorDialog::getColor(bgColor, this, "选择背景颜色", QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        if (c != bgColor)
        {
            settings.setValue("music/bgColor", bgColor = c);
            update();
        }
    })->bgColor(bgColor)->hide(!jiWindow);
    auto fontMenu = menu->addMenu("字体大小");
    QStringList sl;
    for (int i = 12; i < 30; i++)
        sl << QString::number(i);
    fontMenu->addOptions(sl, pointSize-12, [=](int index){
        pointSize = index + 12;
        settings.setValue("music/desktopLyricPointSize", pointSize);
        update();
    });
    QFontDatabase fdb;
    QStringList families = fdb.families();
    /* auto familyMenu = menu->addMenu("选择字体");
    familyMenu->addOptions(families, families.indexOf(fontFamily), [=](int index) {
        this->fontFamily = families.at(index);
        settings.setValue("music/desktopLyricFontFamily", this->fontFamily);
        update();
    }); */
    auto combo = new QComboBox(menu);
    combo->addItems(families);
    combo->setCurrentIndex(families.indexOf(fontFamily.isEmpty() ? font().family() : fontFamily));
    connect(combo, &QComboBox::currentTextChanged, this, [=](const QString& string) {
        this->fontFamily = string;
        settings.setValue("music/desktopLyricFontFamily", this->fontFamily);
        update();
    });
    combo->setStyleSheet("QComboBox{ background: transparent; }");
    menu->addWidget(combo);
    menu->split()->addAction("透明模式", [=]{
        bool trans = settings.value("music/desktopLyricTrans", false).toBool();
        settings.setValue("music/desktopLyricTrans", !trans);
        emit signalSwitchTrans();
    })->ifer(!jiWindow)->check();
    menu->addAction("隐藏", [=]{
        this->hide();
        emit signalhide();
    });
    menu->exec(QCursor::pos());
}

void DesktopLyricWidget::setPosition(qint64 position)
{
    if (!lyricStream.size())
        return ;
    if (currentRow < 0 || currentRow >= lyricStream.size())
        currentRow = 0;

    LyricBean lyric = lyricStream.at(currentRow);
    if (currentRow == lyricStream.size()-1 && lyric.start <= position) // 已经到末尾了
        return ;

    LyricBean nextLyric = currentRow == lyricStream.size()-1 ? LyricBean(false) : lyricStream.at(currentRow + 1);
    if (lyric.start <= position && nextLyric.start >= position) // 不需要改变
        return ;

    if (lyric.start > position) // 什么情况？从头强制重新开始！
        currentRow = 0;

    while (currentRow+1 < lyricStream.size() && lyricStream.at(currentRow+1).start < position)
    {
        currentRow++;
    }
    update();
}
