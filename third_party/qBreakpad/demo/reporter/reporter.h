/*
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

#ifndef QAPPTESTFIELD_H
#define QAPPTESTFIELD_H

#include <QDialog>

namespace Ui
{
class ReporterExample;
}

class ReporterExample : public QDialog
{
    Q_OBJECT

public:
    explicit ReporterExample (QWidget *parent = 0);
    ~ReporterExample();

public slots:
    void crash();
    void uploadDumps();

private:
    Ui::ReporterExample *ui;
};

#endif  // QAPPTESTFIELD_H
