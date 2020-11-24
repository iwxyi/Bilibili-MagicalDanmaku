#ifndef LYRICSTREAMWIDGET_H
#define LYRICSTREAMWIDGET_H

#include <QWidget>
#include <QPainter>
#include "desktoplyricwidget.h"

class LyricStreamWidget : public QWidget
{
    Q_OBJECT
public:
    LyricStreamWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showMenu()));
    }

    void setLyric(QString text)
    {
        // 检测是不是全是毫秒还是10毫秒的
        int ms10x = 10;
        QRegularExpression re10("^\\[(\\d{2}):(\\d{2}).(\\d{2,3})\\]");
        QRegularExpressionMatch match10;
        if (text.lastIndexOf(re10, -1, &match10) != 0)
        {
            int val = match10.captured(3).toInt();
            if (val > 0) // 存在不为0的三位数
            {
                ms10x = 1;
            }
        }

        // 遍历每一行
        QStringList sl = text.split("\n", QString::SkipEmptyParts);
        LyricBean prevLyric(false);
        qint64 currentTime = 0;
        lyricStream.clear();
        foreach (QString line, sl)
        {
            QRegularExpression re("^\\[(\\d{2}):(\\d{2})\\.(\\d{2,3})\\](\\[(\\d{2}):(\\d{2})\\.(\\d{2,3})\\])?(.*)$");
            QRegularExpressionMatch match;
            if (line.indexOf(re, 0, &match) == -1)
            {
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

        QFont font = this->font();
        QFontMetrics fm(font);
        this->lineSpacing = fm.height() + (fm.lineSpacing() - fm.height())*2; // 双倍行间距
        setFixedHeight((lyricStream.size()+verticalMargin*2) * lineSpacing);
        update();
    }

    void setPosition(qint64 position)
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

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        int rowOffset = verticalMargin;
        for (int i = 0; i < lyricStream.size(); i++)
        {
            int row = i + rowOffset;
            int top = row * lineSpacing;
            QRect lineRect(0, top, width(), lineSpacing);
            painter.drawText(lineRect, Qt::AlignCenter, lyricStream.at(i).text);
        }

    }

private slots:
    void showMenu()
    {
        FacileMenu* menu = new FacileMenu(this);


        menu->exec();
    }

private:
    LyricStream lyricStream;
    int currentRow = -1;
    int lineSpacing = 30;
    int verticalMargin = 5; // 上下间距5行
};

#endif // LYRICSTREAMWIDGET_H

