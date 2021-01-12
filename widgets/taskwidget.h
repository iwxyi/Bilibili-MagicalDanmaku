#ifndef TASKWIDGET_H
#define TASKWIDGET_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextDocument>
#include <QDebug>

class TaskWidget : public QWidget
{
    Q_OBJECT
public:
    TaskWidget(QWidget *parent = nullptr);

signals:
    void signalSendMsgs(QString msgs, bool manual);
    void spinChanged(int val);
    void signalResized();

public slots:
    void slotSpinChanged(int val);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

public:
    QTimer* timer;
    QCheckBox* check;
    QSpinBox* spin;
    QPushButton* btn;
    QPlainTextEdit* edit;
};

#endif // TASKWIDGET_H
