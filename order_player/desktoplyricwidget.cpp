#include "desktoplyricwidget.h"

DesktopLyricWidget::DesktopLyricWidget(QWidget *parent) : QWidget(parent),
    settings("musics.ini", QSettings::Format::IniFormat)
{
    this->setWindowTitle("桌面歌词");
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);      //设置为无边框置顶窗口
    this->setMinimumSize(45, 25);                        //设置最小尺寸
    this->setAttribute(Qt::WA_TranslucentBackground, true); // 设置窗口透明
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
    pointSize = settings.value("music/desktopLyricPointSize", pointSize).toInt();
}

/**
 * 歌词文本设置成歌词流
 * 在这里解析格式
 */
void DesktopLyricWidget::setLyric(QString text)
{
    QStringList sl = text.split("\n", QString::SkipEmptyParts);
    LyricBean prevLyric(false);
    foreach (QString line, sl)
    {
        QRegularExpression re("^\\[(\\d{2}):(\\d{2}).(\\d{2})\\](\\[(\\d{2}):(\\d{2}).(\\d{2})\\])?(.*)$");
        QRegularExpressionMatch match;
        if (line.indexOf(re, 0, &match) == -1)
            continue;
        QStringList caps = match.capturedTexts();
        LyricBean lyric;
        int minute = caps.at(1).toInt();
        int second = caps.at(2).toInt();
        int ms10 = caps.at(3).toInt();
        lyric.start = minute * 60000 + second*1000 + ms10 * 10;
        if (!caps.at(4).isEmpty()) // 有终止时间
        {
            int minute = caps.at(5).toInt();
            int second = caps.at(6).toInt();
            int ms10 = caps.at(7).toInt();
            lyric.end = minute * 60000 + second*1000 + ms10 * 10;
        }
        lyric.text = caps.at(8);
        lyricStream.append(lyric);
    }

    currentRow = 0;
    update();
}

void DesktopLyricWidget::showEvent(QShowEvent *event)
{
    restoreGeometry(settings.value("livedanmakuwindow/geometry").toByteArray());
}

void DesktopLyricWidget::hideEvent(QHideEvent *event)
{
    settings.setValue("livedanmakuwindow/geometry", this->saveGeometry());
}

bool DesktopLyricWidget::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
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
    return false;         //此处返回false，留给其他事件处理器处理
}

void DesktopLyricWidget::enterEvent(QEvent *event)
{
    hovering = true;
    update();
}

void DesktopLyricWidget::leaveEvent(QEvent *event)
{
    hovering = false;
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
    QFont font;
    font.setPointSize(pointSize);
    font.setBold(true);
    painter.setFont(font);

    // 绘制桌面歌词
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

    // 绘制背景
    if (hovering)
    {
        painter.setRenderHint(QPainter::Antialiasing, true);
        QPainterPath path;
        path.addRoundedRect(this->rect(), 5, 5);
        painter.fillPath(path, QColor(64, 64, 64, 32));
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
        QColor c = QColorDialog::getColor(playingColor, this, "选择背景颜色", QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        if (c != playingColor)
        {
            settings.setValue("music/playingColor", playingColor = c);
            update();
        }
    })->fgColor(playingColor);
    menu->addAction("未播放颜色", [=]{
        QColor c = QColorDialog::getColor(waitingColor, this, "选择背景颜色", QColorDialog::ShowAlphaChannel);
        if (!c.isValid())
            return ;
        if (c != waitingColor)
        {
            settings.setValue("music/waitingColor", waitingColor = c);
            update();
        }
    })->fgColor(waitingColor);
    auto fontMenu = menu->addMenu("字体大小");
    QStringList sl;
    for (int i = 12; i < 30; i++)
        sl << QString::number(i);
    fontMenu->addOptions(sl, pointSize-12, [=](int index){
        pointSize = index + 12;
        settings.setValue("music/desktopLyricPointSize", pointSize);
        update();
    });
    /*fontMenu->addNumberedActions("%1", 5, 30, [=](FacileMenuItem*){}, [=](int index){
        pointSize = index + 10;
        settings.setValue("music/desktopLyricPointSize", pointSize);
        update();
    });*/
    menu->split()->addAction("隐藏", [=]{
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
    if (lyric.start <= position && nextLyric.start > position) // 不需要改变
        return ;

    if (lyric.start > position) // 什么情况？从头强制重新开始！
        currentRow = 0;

    while (currentRow+1 < lyricStream.size() && lyricStream.at(currentRow+1).start < position)
    {
        currentRow++;
    }
    update();
}
