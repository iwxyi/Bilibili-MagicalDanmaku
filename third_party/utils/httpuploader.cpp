#include <QFile>
#include <QUuid>
#include <QFileInfo>
#include <QNetworkCookie>
#include "httpuploader.h"

HttpUploader::HttpUploader(const QString& url)
    : url(url)
    , post_content(QByteArray())
{
    multi_part = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    net_manager = new QNetworkAccessManager(this);
    connect(net_manager, SIGNAL(finished(QNetworkReply*)), this, SIGNAL(finished(QNetworkReply*)));
}

HttpUploader::~HttpUploader()
{
    if (multi_part != nullptr)
    {
        delete multi_part;
        multi_part = nullptr;
    }

    if (net_manager != nullptr)
    {
        delete net_manager;
        net_manager = nullptr;
    }
}

void HttpUploader::setCookies(QString c)
{
    this->cookies = toVariantCookies(c);
}

void HttpUploader::setCookies(QVariant c)
{
    this->cookies = c;
}

bool HttpUploader::addTextField(const QString &key, const QByteArray &value)
{
    QHttpPart text_part;
    text_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"" + key + "\""));
    text_part.setBody(value);

    multi_part->append(text_part);
    return true;
}

bool HttpUploader::addFileField(const QString &key, const QString& file_path)
{
    QFileInfo upload_file_info(file_path);
    return addFileField(key, upload_file_info.fileName(), file_path);
}

bool HttpUploader::addFileField(const QString &key, const QString &file_name, const QString &file_path)
{
    return addFileField(key, file_name, file_path, "application/octet-stream");
}

bool HttpUploader::addFileField(const QString &key, const QString &file_name, const QString &file_path, const QString &type)
{
    QHttpPart file_part;
    file_part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(type));
    file_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"" + key + "\"; filename=\"" + file_name + "\""));

    QFile *file = new QFile(file_path);
    file->open(QIODevice::ReadOnly);
    file->setParent(multi_part);
    file_part.setBodyDevice(file);

    multi_part->append(file_part);

    return true;
}

bool HttpUploader::post()
{
    QNetworkRequest request = QNetworkRequest(QUrl(url));
    if (!cookies.isNull())
        request.setHeader(QNetworkRequest::CookieHeader, cookies);

    net_reply = net_manager->post(request, multi_part);
    multi_part->setParent(net_reply); // delete the multiPart with the reply
    connect(net_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SIGNAL(errored(QNetworkReply::NetworkError)));
    connect(net_reply, SIGNAL(uploadProgress(qint64, qint64)), this, SIGNAL(progress(qint64, qint64)));

    return true;
}

QVariant HttpUploader::toVariantCookies(QString c) const
{
    QList<QNetworkCookie> cookies;


    // 设置cookie
    QString cookieText = c;
    QStringList sl = cookieText.split(";");
    foreach (auto s, sl)
    {
        s = s.trimmed();
        int pos = s.indexOf("=");
        QString key = s.left(pos);
        QString val = s.right(s.length() - pos - 1);
        cookies.push_back(QNetworkCookie(key.toUtf8(), val.toUtf8()));
    }

    // 请求头里面加入cookies
    QVariant var;
    var.setValue(cookies);
    return var;
}
