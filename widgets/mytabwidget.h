#ifndef MYTABWIDGET_H
#define MYTABWIDGET_H

#include <QTabWidget>

class MyTabWidget : public QTabWidget
{
    Q_OBJECT
    Q_PROPERTY(int bgAlpha READ getBgAlpha WRITE setBgAlpha)
    Q_PROPERTY(int prevAlpha READ getPrevAlpha WRITE setPrevAlpha)
public:
    MyTabWidget(QWidget* parent = nullptr);

    void setBg(const QPixmap& pixmap);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setBlurBg(const QPixmap& bg);

    void setBgAlpha(int x);
    int getBgAlpha() const;
    void setPrevAlpha(int x);
    int getPrevAlpha() const;

private:
    QPixmap originPixmap;
    QPixmap bgPixmap;
    QPixmap prevPixmap;

    const int standardAlpha = 32;
    int maxAlpha = 0;
    int bgAlpha = 0;
    int prevAlpha = 0;
};

#endif // MYTABWIDGET_H
