#ifndef LYRICSTREAMWIDGET_H
#define LYRICSTREAMWIDGET_H

#include <QWidget>
#include <QPainter>
#include "desktoplyricwidget.h"

class LyricStreamWidget : public QWidget
{
    Q_OBJECT
public:
    LyricStreamWidget(QWidget* parent = nullptr)
        : QWidget(parent), settings("musics.ini", QSettings::Format::IniFormat),
          updateTimer(new QTimer(this))
    {
        connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showMenu()));
        playingColor = qvariant_cast<QColor>(settings.value("lyric/playingColor", playingColor));
        waitingColor = qvariant_cast<QColor>(settings.value("lyric/waitingColor", waitingColor));
        pointSize = settings.value("lyric/pointSize", pointSize).toInt();

        connect(updateTimer, SIGNAL(timeout()), this, SLOT(update()));
        updateTimer->setInterval(16);
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
        font.setPointSize(pointSize);
        QFontMetrics fm(font);
        this->lineSpacing = fm.height() + (fm.lineSpacing() - fm.height())*2; // 双倍行间距
        setFixedHeight((lyricStream.size()+verticalMargin*2) * lineSpacing);
        update();
    }

    /**
     * @return 当前行有没有改变
     */
    bool setPosition(qint64 position)
    {
        if (!lyricStream.size())
            return false;
        if (currentRow < 0 || currentRow >= lyricStream.size())
            currentRow = 0;

        LyricBean lyric = lyricStream.at(currentRow);
        if (currentRow == lyricStream.size()-1 && lyric.start <= position) // 已经到末尾了
            return false;

        LyricBean nextLyric = currentRow == lyricStream.size()-1 ? LyricBean(false) : lyricStream.at(currentRow + 1);
        if (lyric.start <= position && nextLyric.start > position) // 不需要改变
            return false;

        if (lyric.start > position) // 什么情况？从头强制重新开始！
            currentRow = 0;

        while (currentRow+1 < lyricStream.size() && lyricStream.at(currentRow+1).start < position)
        {
            currentRow++;
        }
        switchRowTimestamp = QDateTime::currentMSecsSinceEpoch();
        updateTimer->start();
        update();
        return true;
    }

    int getCurrentTop() const
    {
        return (currentRow + verticalMargin + 0.5) * lineSpacing;
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        QFont font(painter.font());
        font.setPointSize(pointSize);
        painter.setFont(font);
        painter.setPen(waitingColor);
        QFontMetrics fm(font);
        int lineSpacing = fm.height() + (fm.lineSpacing() - fm.height()) * 2;
        double rowOffset = verticalMargin;
        qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();
        qint64 aniDelta = currentTimestamp - switchRowTimestamp;
        bool animating = aniDelta < switchRowDuration;
        const double scale = 0.5;
        for (int i = 0; i < lyricStream.size(); i++)
        {
            double row = i + rowOffset;
            int top = row * lineSpacing;
            if (i == currentRow - 1 && animating)
            {
                painter.save();
                double prop = scale - scale * aniDelta / switchRowDuration;
                QFont font(painter.font());
                font.setPointSize(pointSize * (1+prop));
                painter.setFont(font);
                QRect lineRect(0, top - lineSpacing*prop/2, width(), lineSpacing * (1 + prop));
                painter.drawText(lineRect, Qt::AlignCenter, lyricStream.at(i).text);
                painter.restore();
                rowOffset += prop;
            }
            else if (i == currentRow)
            {
                painter.save();
                if (animating)
                {
                    double prop = scale * aniDelta / switchRowDuration;
                    QRect lineRect(0, top - lineSpacing*prop/2, width(), lineSpacing * (1 + 2 * prop));
                    QFont font(painter.font());
                    font.setPointSize(pointSize * (1+prop));
                    painter.setPen(playingColor);
                    painter.setFont(font);
                    painter.drawText(lineRect, Qt::AlignCenter, lyricStream.at(i).text);
                    rowOffset += prop;
                }
                else
                {
                    QRect lineRect(0, top - lineSpacing*0.25, width(), lineSpacing * (1+scale*2));
                    QFont font(painter.font());
                    font.setPointSize(pointSize * (1+scale));
                    painter.setPen(playingColor);
                    painter.setFont(font);
                    painter.drawText(lineRect, Qt::AlignCenter, lyricStream.at(i).text);
                    rowOffset += scale;

                    if (updateTimer->isActive())
                        updateTimer->stop();
                }
                painter.restore();
            }
            else
            {
                QRect lineRect(0, top, width(), lineSpacing);
                painter.drawText(lineRect, Qt::AlignCenter, lyricStream.at(i).text);
            }
        }

    }

private slots:
    void showMenu()
    {
        FacileMenu* menu = new FacileMenu(this);
        auto lineMenu = menu->addMenu("行数");
        menu->addAction("已播放颜色", [=]{
            QColor c = QColorDialog::getColor(playingColor, this, "选择已播放颜色", QColorDialog::ShowAlphaChannel);
            if (!c.isValid())
                return ;
            if (c != playingColor)
            {
                settings.setValue("lyric/playingColor", playingColor = c);
                update();
            }
        })->fgColor(playingColor);
        menu->addAction("未播放颜色", [=]{
            QColor c = QColorDialog::getColor(waitingColor, this, "选择未播放颜色", QColorDialog::ShowAlphaChannel);
            if (!c.isValid())
                return ;
            if (c != waitingColor)
            {
                settings.setValue("lyric/waitingColor", waitingColor = c);
                update();
            }
        })->fgColor(waitingColor);
        auto fontMenu = menu->addMenu("字体大小");
        QStringList sl;
        for (int i = 12; i < 30; i++)
            sl << QString::number(i);
        fontMenu->addOptions(sl, pointSize-12, [=](int index){
            pointSize = index + 12;
            settings.setValue("lyric/pointSize", pointSize);
            update();
        });
        menu->exec(QCursor::pos());
    }

private:
    QSettings settings;
    LyricStream lyricStream;
    int currentRow = -1;
    qint64 switchRowTimestamp = 0;
    const int switchRowDuration = 200;
    QTimer* updateTimer;
    int lineSpacing = 30;
    int verticalMargin = 5; // 上下间距5行

    int pointSize = 14;
    QColor playingColor = QColor(155, 80, 255);
    QColor waitingColor = Qt::black;
};

#endif // LYRICSTREAMWIDGET_H

