#ifndef PARTIMAGEWIDGET_H
#define PARTIMAGEWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <functional>

typedef std::function<QPainterPath(QRect canvas)> FuncPaintType;

/**
 * 只显示一部分的控件
 */
class PartImageWidget : public QWidget
{
public:
    PartImageWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
    }

    void setPixmap(const QPixmap& p)
    {
        this->originPixmap = p;
        this->scaledPixmap = p.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }

    /// 根据 x/y/w/h 来选择要绘制的区域
    void setPart(FuncPaintType func)
    {
        this->paintFunc = func;
    }

    void paintEvent(QPaintEvent *) override
    {
        if (originPixmap.isNull() || !paintFunc)
            return ;

        QPainter painter(this);

        // 仅根据控件大小，获取裁剪的区域
        QRect r = this->rect();
        QPainterPath path = paintFunc(geometry());
        painter.setClipPath(path);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // 填满画布
        QPoint pos((scaledPixmap.width() - r.width())/2, (scaledPixmap.height() - r.height()) / 2);
        painter.drawPixmap(r, scaledPixmap, QRect(pos, r.size()));
    }

    void resizeEvent(QResizeEvent *event) override
    {
        scaledPixmap = originPixmap.scaled(this->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        return QWidget::resizeEvent(event);
    }

private:
    QPixmap originPixmap;
    QPixmap scaledPixmap;
    FuncPaintType paintFunc = nullptr;
};

#endif // PARTIMAGEWIDGET_H
