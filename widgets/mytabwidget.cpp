#include <QPropertyAnimation>
#include <QPainter>
#include <QDebug>
#include "mytabwidget.h"

QT_BEGIN_NAMESPACE
    extern Q_WIDGETS_EXPORT void qt_blurImage( QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0 );
QT_END_NAMESPACE

MyTabWidget::MyTabWidget(QWidget *parent) : QTabWidget(parent)
{

}

void MyTabWidget::setBg(const QPixmap &pixmap)
{
    this->originPixmap = pixmap;
    this->prevPixmap = this->bgPixmap;
    prevAlpha = maxAlpha;
    setBlurBg(pixmap); // 重新设置maxAlpha
    bgAlpha = 0;

    QPropertyAnimation* ani = new QPropertyAnimation(this, "bgAlpha");
    ani->setStartValue(0);
    ani->setEndValue(maxAlpha);
    ani->setDuration(800);
    ani->setEasingCurve(QEasingCurve::InOutSine);
    connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
    ani->start();

    ani = new QPropertyAnimation(this, "prevAlpha");
    ani->setStartValue(prevAlpha);
    ani->setEndValue(0);
    ani->setDuration(800);
    ani->setEasingCurve(QEasingCurve::InOutSine);
    connect(ani, SIGNAL(finished()), ani, SLOT(deleteLater()));
    ani->start();
}

void MyTabWidget::paintEvent(QPaintEvent *event)
{
    QTabWidget::paintEvent(event);

    QPainter painter(this);
    // 画模糊背景
    if (prevAlpha && !prevPixmap.isNull())
    {
        painter.setOpacity((double)prevAlpha / 255);
        int x = 0, y = 0;
        if (prevPixmap.width() > width())
            x = (prevPixmap.width() - width()) / 2;
        if (prevPixmap.height() > height())
            y = (prevPixmap.height() - height()) / 2;
        painter.drawPixmap(rect(), prevPixmap, QRect(x, y, width(), height()));
    }

    if (bgAlpha && !bgPixmap.isNull())
    {
        painter.setOpacity((double)bgAlpha / 255);
        int x = 0, y = 0;
        if (bgPixmap.width() > width())
            x = (bgPixmap.width() - width()) / 2;
        if (bgPixmap.height() > height())
            y = (bgPixmap.height() - height()) / 2;
        painter.drawPixmap(rect(), bgPixmap, QRect(x, y, width(), height()));
    }
}

void MyTabWidget::resizeEvent(QResizeEvent *event)
{
    QTabWidget::resizeEvent(event);

    if (!originPixmap.isNull())
        setBlurBg(originPixmap);
}

void MyTabWidget::setBlurBg(const QPixmap &bg)
{
    const int radius = qMax(20, qMin(width(), height())/5);
    QPixmap pixmap = bg;
    pixmap = pixmap.scaled(this->width()+radius*2, this->height() + radius*2, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QImage img = pixmap.toImage(); // img -blur-> painter(pixmap)
    QPainter painter( &pixmap );
    qt_blurImage( &painter, img, radius, true, false );

    // 裁剪掉边缘（模糊后会有黑边）
    int c = qMin(bg.width(), bg.height());
    c = qMin(c/2, radius);
    QPixmap clip = pixmap.copy(c, c, pixmap.width()-c*2, pixmap.height()-c*2);
    this->bgPixmap = clip;

    // 抽样获取背景，设置之后的透明度
    qint64 rgbSum = 0;
    QImage image = clip.toImage();
    int w = image.width(), h = image.height();
    const int m = 16;
    for (int y = 0; y < m; y++)
    {
        for (int x = 0; x < m; x++)
        {
            QColor c = image.pixelColor(w*x/m, h*x/m);
            rgbSum += c.red() + c.green() + c.blue();
        }
    }
    int addin = rgbSum * standardAlpha / (255*3*m*m);
    maxAlpha = standardAlpha + addin;
}

void MyTabWidget::setBgAlpha(int x)
{
    this->bgAlpha = x;
    update();
}

int MyTabWidget::getBgAlpha() const
{
    return bgAlpha;
}

void MyTabWidget::setPrevAlpha(int x)
{
    this->prevAlpha = x;
    update();
}

int MyTabWidget::getPrevAlpha() const
{
    return prevAlpha;
}
