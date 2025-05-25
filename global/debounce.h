#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <QObject>
#include <QTimer>
#include <QDebug>
#include "string_hash.h"

#define DEBOUNCE_INTERVAL 200  // 防抖动的时间
#define DEBOUNCE_DEB if (0) qDebug()

typedef std::function<void()> DebounceFunction;

/**
 * 一个防抖动的类
 */
class Debounce : public QObject
{
public:
    /**
     * @brief call 防抖动的入口
     * @param key  确保全局key是唯一，否则会被覆盖掉
     * @param func 要执行的代码，相同key后来的会覆盖原先的。尽量不包含指针，否则让一出问题
     */
    void call(hash_t key, DebounceFunction func)
    {
        call(key, this, func);
    }

    void call(hash_t key, QObject* obj, DebounceFunction func)
    {
        call(key, DEBOUNCE_INTERVAL, obj, func);
    }

    void call(hash_t key, int ms, QObject* obj, DebounceFunction func)
    {
        int index = keys.indexOf(key);
        if (index > -1)
        {
            DEBOUNCE_DEB << "[debounce] cache:" << key;
            funcs[index] = func;
            timers[index]->stop();
            timers[index]->start(ms);
        }
        else
        {
            DEBOUNCE_DEB << "[debounce] append:" << key;
            keys.append(key);
            funcs.append(func);
            QTimer* timer = new QTimer(this);
            timers.append(timer);
            timer->setSingleShot(true);
            timer->setInterval(ms);
            connect(timer, &QTimer::timeout, obj, [=](){
                DEBOUNCE_DEB << "[debounce] execute:" << key;
                int index = keys.indexOf(key);
                (funcs[index])();
                keys.removeAt(index);
                funcs.removeAt(index);
                timers.removeAt(index);
                // timer->deleteLater();
            });
            timer->start();
        }
    }

private:
    QList<hash_t> keys;
    QList<QTimer*> timers;
    QList<DebounceFunction> funcs;
};

extern Debounce* debounce;

#endif // DEBOUNCE_H
