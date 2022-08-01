/*
 *  Copyright (C) 2009 Aleksey Palazhchenko
 *  Copyright (C) 2014 Sergey Shambir
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
 
#ifndef QBREAKPAD_HTTP_SENDER_H
#define QBREAKPAD_HTTP_SENDER_H

#include <QObject>
#include <QPointer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class QString;
class QUrl;
class QFile;

class QBreakpadHttpUploader : public QObject
{
	Q_OBJECT
public:
    QBreakpadHttpUploader(QObject *parent=0);
    QBreakpadHttpUploader(const QUrl& url, QObject *parent=0);
    ~QBreakpadHttpUploader();

    //TODO: proxy, ssl
    QString remoteUrl() const;
    void setUrl(const QUrl& url);

signals:
    void finished(QString answer);

public slots:
    void uploadDump(const QString& abs_file_path);

private slots:
    void onUploadProgress(qint64 sent, qint64 total);
    void onError(QNetworkReply::NetworkError err);
    void onUploadFinished();

private:
    QNetworkAccessManager m_manager;
    QNetworkRequest m_request;
    QPointer<QNetworkReply> m_reply;
    QFile* m_file;
};

#endif	// QBREAKPAD_HTTP_SENDER_H
