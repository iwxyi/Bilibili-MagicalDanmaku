#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
#include <QFont>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
