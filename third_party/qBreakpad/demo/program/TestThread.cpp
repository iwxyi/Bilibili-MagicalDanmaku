/*
 *  Copyright (C) 2009 Aleksey Palazhchenko
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

#include "TestThread.h"

#include <QTimer>

TestThread::TestThread(bool buggy, uint seed)
	: m_buggy(buggy)
{
	qsrand(seed);
}

TestThread::~TestThread()
{
}

void TestThread::crash()
{
	reinterpret_cast<QString*>(1)->toInt();
	throw 1;
}

void TestThread::run()
{
	if(m_buggy) {
		QTimer::singleShot(qrand() % 2000 + 100, this, SLOT(crash()));
	} else {
		QTimer::singleShot(qrand() % 2000 + 100, this, SLOT(quit()));
	}
	exec();
}
