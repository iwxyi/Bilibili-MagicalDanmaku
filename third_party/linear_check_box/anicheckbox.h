#ifndef LINEARCHECKBOX_H
#define LINEARCHECKBOX_H

#include <QCheckBox>
#include <QPropertyAnimation>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>

class AniCheckBox : public QCheckBox
{
    Q_OBJECT
    Q_PROPERTY(double hover_prog READ getHoverProg WRITE setHoverProg)
    Q_PROPERTY(double part_prog READ getPartProg WRITE setPartProg)
    Q_PROPERTY(double check_prog READ getCheckProg WRITE setCheckProg)
public:
    AniCheckBox(QWidget* parent = nullptr);

    void setForeColor(QColor c);

protected:
    void paintEvent(QPaintEvent *) override;
    void enterEvent(QEvent *e) override;
    void leaveEvent(QEvent *e) override;
    bool hitButton(const QPoint &) const override;

    virtual void checkStateChanged(int state);

    virtual void drawBox(QPainter &painter, QRectF rect);

    QPropertyAnimation* startAnimation(const QByteArray &property, double begin, double end, int duration = 500, QEasingCurve curve = QEasingCurve::OutQuad);

protected:
    double getHoverProg() const;
    void setHoverProg(double prog);
    double getPartProg() const;
    void setPartProg(double prog);
    double getCheckProg() const;
    void setCheckProg(double prog);

protected:
    int boxSide = 0; // 选择框边长，0为自适应
    QColor foreColor = QColor("#2753ff"); // 前景颜色

    double hoverProg = 0;   // 鼠标移上去的进度
    double partyProg = 0;   // 部分选中的进度
    double checkProg = 0;   // 选中的进度
};

#endif // LINEARCHECKBOX_H
