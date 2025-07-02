#include "checkbox1.h"
#include <QtMath>

CheckBox1::CheckBox1(QWidget *parent) : AniCheckBox(parent)
{

}

void CheckBox1::setUncheckedColor(QColor c)
{
    this->uncheckedColor = c;
    update();
}

void CheckBox1::enterEvent(QEnterEvent *e)
{
    QCheckBox::enterEvent(e);
    startAnimation("hover_prog", getHoverProg(), 1, 300, QEasingCurve::OutBack);
}

void CheckBox1::leaveEvent(QEvent *e)
{
    QCheckBox::leaveEvent(e);
    startAnimation("hover_prog", getHoverProg(), 0, 300, QEasingCurve::OutBack);
}

void CheckBox1::checkStateChanged(int state)
{
    const double threshold = 0.5;
    if (state == Qt::Unchecked)
    {
        startAnimation("check_prog", getCheckProg(), 0, 600, QEasingCurve::OutQuad);
    }
    else if (state == Qt::PartiallyChecked)
    {
        startAnimation("check_prog", getCheckProg(), threshold, 400, QEasingCurve::OutQuad);
    }
    else if (state == Qt::Checked)
    {
        startAnimation("check_prog", getCheckProg(), 1, getCheckProg() >= threshold ? 300 : 600, QEasingCurve::OutQuad);
    }
}

void CheckBox1::drawBox(QPainter &painter, QRectF rect)
{
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(uncheckedColor, qMax(rect.width() / 16, 1.0), Qt::SolidLine, Qt::RoundCap));
    if (!isEnabled())
        painter.setOpacity(0.5);

    /// 绘制边缘方框，和悬浮状态有关
    QPainterPath borderPath;
    double radius = rect.width() / 8;
    double l = rect.left(), t = rect.top(), r = rect.right(), b = rect.bottom(),
            w = rect.width(), h = rect.height();

    // arcTo:从0度开始，逆时针扫，参数为角度制
    double quadDishu = 8;
    borderPath.moveTo(l, t + radius);
    borderPath.arcTo(QRectF(l, t, radius * 2, radius * 2), 180, -90);
    borderPath.quadTo(QPointF(l + w / 2, t + h / quadDishu * hoverProg), QPointF(r - radius, t));
    // borderPath.lineTo(r - radius, t);
    borderPath.arcTo(QRectF(r - radius * 2, t, radius * 2, radius * 2), 90, -90);
    borderPath.quadTo(QPointF(r - w / quadDishu * hoverProg, t + h / 2), QPointF(r, b - radius));
    borderPath.arcTo(QRectF(r - radius * 2, b - radius * 2, radius * 2, radius * 2), 0, -90);
    borderPath.quadTo(QPointF(r - w / 2, b - h / quadDishu * hoverProg), QPointF(l + radius, b));
    borderPath.arcTo(QRectF(l, b - radius * 2, radius * 2, radius * 2), -90, -90);
    borderPath.quadTo(QPointF(l + w / quadDishu * hoverProg, b - h / 2), QPointF(l, t + radius));
     painter.drawPath(borderPath);

    /// 画选中
    painter.setPen(QPen(foreColor, qMax(rect.width() / 20, 1.0), Qt::SolidLine, Qt::RoundCap));
    QPainterPath circlePath;
    circlePath.moveTo(l, t);
    const double outRad = sqrt(w * w + h * h) / 2; // 外接圆半径
    const double threshold = 0.5; // 到什么时刻转完一圈

    // 边框转圈
    double borderVal = checkProg;
    if (borderVal > threshold)
        borderVal = threshold;
    borderVal = borderVal / threshold; // 边缘的完成度的比例
    if (borderVal >= 1)
        borderVal = 1;
    circlePath.arcTo(QRectF(l + w / 2 - outRad, t + h / 2 - outRad, outRad * 2, outRad * 2), 135, 360 * borderVal);
    painter.save();
    painter.setClipPath(circlePath);
    painter.drawPath(borderPath);
    painter.restore();

    // 勾的出现
    if (checkProg > threshold)
    {
        double gou = checkProg - threshold;
        const double threshold = 0.18;
        const double restThreshold = 0.32;
        // QPointF point0(l, t + radius); // 从边缘连接到√
        QPointF point1(l + w * 0.30, t + h * 0.45);
        QPointF point2(l + w * 0.45, t + h * 0.70);
        QPointF point3(l + w * 0.70, t + h * 0.32);

        // 第一段
        double prop = gou;
        if (prop >= threshold)
            prop = threshold;
        prop = prop / threshold; // 在当前段的比例(偏中心）
        QPainterPath path;
        path.moveTo(point1);
        path.lineTo(point2);
        painter.drawLine(QLineF(point1, path.pointAtPercent(prop)));

        // 第二段
        if (gou > threshold)
        {
            gou -= threshold;
            double prop = gou / restThreshold; // 在当前段的比例
            QPainterPath path2;
            path2.moveTo(point2);
            path2.lineTo(point3);
            painter.drawLine(QLineF(point2, path2.pointAtPercent(prop)));
        }
    }
}
