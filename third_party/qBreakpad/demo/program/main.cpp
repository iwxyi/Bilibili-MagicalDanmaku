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

#include <QCoreApplication>
#include <QTimer>
#include <QDateTime>

#include "QBreakpadHandler.h"
#include "TestThread.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("AppName");
    QCoreApplication::setApplicationVersion("1.0");
    QCoreApplication::setOrganizationName("OrgName");
    QCoreApplication::setOrganizationDomain("name.org");

    QBreakpadInstance.setDumpPath("crashes");

    qsrand(QDateTime::currentDateTime().toTime_t());
    TestThread t1(false, qrand());
    TestThread t2(true, qrand());

    t1.start();
    t2.start();

    QTimer::singleShot(3000, qApp, SLOT(quit()));
    return app.exec();
}
