/*
 * Copyright (C) 2016 Alexander Makarov
 *
 * Source:
 * https://wiki.qt.io/Qt_thread-safe_singleton
 *
 * This file is a part of Breakpad-qt library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

#ifndef SINGLETON
#define SINGLETON

#include <QtGlobal>
#include <QScopedPointer>
#include "call_once.h"

template <class T>
class Singleton
{
public:
    static T& instance()
    {
        qCallOnce(init, flag);
        return *tptr;
    }

    static void init()
    {
        tptr.reset(new T);
    }

private:
    Singleton() {}
    ~Singleton() {}
    Q_DISABLE_COPY(Singleton)

    static QScopedPointer<T> tptr;
    static QBasicAtomicInt flag;
};

template<class T> QScopedPointer<T> Singleton<T>::tptr(0);
template<class T> QBasicAtomicInt Singleton<T>::flag = Q_BASIC_ATOMIC_INITIALIZER(CallOnce::CO_Request);

#endif // SINGLETON

