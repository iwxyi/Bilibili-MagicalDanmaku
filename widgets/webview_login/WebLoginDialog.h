#pragma once
#include <QDialog>
#include <QString>
#include <QWebEngineView>

class QWebEngineView;
class QPushButton;
class QLabel;

class MyWebEngineView : public QWebEngineView {
    Q_OBJECT
public:
    using QWebEngineView::QWebEngineView;
protected:
    QWebEngineView *createWindow(QWebEnginePage::WebWindowType type) override {
        // 让所有新窗口都在当前view中打开
        Q_UNUSED(type);
        return this;
    }
};

class WebLoginDialog : public QDialog {
    Q_OBJECT
public:
    WebLoginDialog(const QString &platformUrl, const QString &defaultCookie = QString(), QWidget *parent = nullptr);
    QString getCookie() const;
private slots:
    void onLoginClicked();
private:
    QLabel* tipLabel;
    MyWebEngineView *webView;
    QPushButton *loginButton;
    QStringList loadedCookies;
    QString cookie;
    void fetchCookie();
}; 
