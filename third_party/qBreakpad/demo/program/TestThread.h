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

#ifndef TESTTHREAD_H
#define TESTTHREAD_H

#include <QThread>

class TestThread : public QThread
{
	Q_OBJECT

public:
	TestThread(bool buggy, uint seed);
	virtual ~TestThread();

protected:
	virtual void run();

private slots:
	void crash();

private:
	bool m_buggy;
};

#endif // TESTTHREAD_H
