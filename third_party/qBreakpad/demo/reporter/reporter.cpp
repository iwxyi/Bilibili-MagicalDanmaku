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

#include "reporter.h"
#include "ui_reporter.h"

#include <QTimer>
#include <QDateTime>
#include "QBreakpadHandler.h"
#include "../program/TestThread.h"

int main (int argc, char *argv[])
{
    QApplication app (argc, argv);

    QCoreApplication::setApplicationName("ReporterExample");
    QCoreApplication::setApplicationVersion("0.0.1");
    QCoreApplication::setOrganizationName("OrgName");
    QCoreApplication::setOrganizationDomain("name.org");

    // Set directory to store dumps and url to upload
    QBreakpadInstance.setDumpPath("crashes");
    // Set server type for uploading
#if defined(SOCORRO)
    QBreakpadInstance.setUploadUrl(QUrl("http://[your.site.com]/submit"));
#elif defined(CALIPER)
    QBreakpadInstance.setUploadUrl(QUrl("http://[your.site.com]/crash_upload"));
#endif

    // Create the dialog and show it
    ReporterExample example;
    example.show();

    // Run the app
    return app.exec();
}

ReporterExample::ReporterExample (QWidget *parent) :
    QDialog (parent),
    ui (new Ui::ReporterExample)
{
    // Create and configure the user interface
    ui->setupUi (this);
    this->setWindowTitle("ReporterExample (qBreakpad v."+QBreakpadHandler::version()+")");
    ui->urlLineEdit->setText(QBreakpadInstance.uploadUrl());

    ui->dumpFilesTextEdit->appendPlainText(QBreakpadInstance.dumpFileList().join("\n"));

    // Force crash app when the close button is clicked
    connect (ui->crashButton, SIGNAL (clicked()),
             this,              SLOT (crash()));

    // upload dumps when the updates button is clicked
    connect (ui->uploadButton, SIGNAL (clicked()),
             this,               SLOT (uploadDumps()));
}

ReporterExample::~ReporterExample()
{
    delete ui;
}

void ReporterExample::crash()
{
    qsrand(QDateTime::currentDateTime().toTime_t());
    TestThread t1(false, qrand());
    TestThread t2(true, qrand());

    t1.start();
    t2.start();

    QTimer::singleShot(3000, qApp, SLOT(quit()));
}

void ReporterExample::uploadDumps()
{
    QBreakpadInstance.sendDumps();
}
