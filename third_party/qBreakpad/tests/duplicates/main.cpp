/*
 *  Copyright (C) 2009 Aleksey Palazhchenko
 *  Copyright (C) 2016 Alexander Makarov
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
 
#include <QBreakpadHandler.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QFile>

#include <cstdio>

void buggyFunc()
{
    delete reinterpret_cast<QString*>(0xFEE1DEAD);
}

void f3(int);

void f1(int i)
{
    if(i) {
        f3(i-1);
    } else {
        buggyFunc();
    }
}

void f2(int i)
{
    f1( i>0 ? i-1 : i);
}

void f3(int i)
{
    f2( i>0 ? i-1 : i );
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    const QStringList args = app.arguments();
    const int method  = args.value(1).toInt();
    if(args.count() != 2 || method < 1 || method > 2 ) {
        std::printf("Usage: %s <crash method>\n", qPrintable(app.applicationFilePath()));
        std::printf("Where <crash method> is '1' or '2'\n");
        return 0;
    }

    app.setApplicationName("test");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("no-such-org");
    app.setOrganizationDomain("no-such.org");
    QBreakpadInstance.setDumpPath("crashes");

    if(method == 1) {
        // first method - simple crash
        f3(0);
    } else {
        // second method - more crazy call stack
        f3(10);
    }

    return app.exec();
}
