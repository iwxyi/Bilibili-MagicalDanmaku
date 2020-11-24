#ifndef SONGBEANS_H
#define SONGBEANS_H

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QDateTime>

#define JSON_VAL_LONG(json, x) static_cast<qint64>(json.value(#x).toDouble())
#define JVAL_LONG(x) static_cast<qint64>(json.value(#x).toDouble())
#define JVAL_INT(x) json.value(#x).toInt()
#define JVAL_STR(x) json.value(#x).toString()
#define snum(x) QString::number(x)

struct Artist
{
    qint64 id = 0;
    QString name;
    QString faceUrl;

    static Artist fromJson(QJsonObject json)
    {
        Artist artist;
        artist.id = JVAL_LONG(id);
        artist.name = JVAL_STR(name);
        artist.faceUrl = JVAL_STR(img1v1Url);
        return artist;
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json.insert("id", id);
        json.insert("name", name);
        json.insert("faceUrl", faceUrl);
        return json;
    }
};

struct Album
{
    qint64 id = 0;
    QString name;
    int size = 0;
    int mark = 0;

    static Album fromJson(QJsonObject json)
    {
        Album album;
        album.id = JVAL_LONG(id);
        album.name = JVAL_STR(name);
        album.size = JVAL_INT(size);
        album.mark = JVAL_INT(mark);
        return album;
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json.insert("id", id);
        json.insert("name", name);
        json.insert("size", size);
        json.insert("mark", mark);
        return json;
    }
};

struct Song
{
    qint64 id = 0;
    QString name;
    int duration = 0;
    int mark = 0;
    QList<Artist> artists;
    Album album;
    QString artistNames;
    qint64 addTime;
    QString addBy;

    static Song fromJson(QJsonObject json)
    {
        Song song;
        song.id = JVAL_LONG(id);
        song.name = JVAL_STR(name);
        QJsonArray array = json.value("artists").toArray();
        QStringList artistNameList;
        foreach (QJsonValue val, array)
        {
            Artist artist = Artist::fromJson(val.toObject());
            song.artists.append(artist);
            artistNameList.append(artist.name);
        }
        song.artistNames = artistNameList.join("/");
        song.album = Album::fromJson(json.value("album").toObject());
        song.duration = JVAL_INT(duration);
        song.mark = JVAL_INT(mark);
        if (json.contains("addTime"))
            song.addTime = JVAL_LONG(addTime);
        if (json.contains("addBy"))
            song.addBy = JVAL_STR(addBy);
        return song;
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json.insert("id", id);
        json.insert("name", name);
        json.insert("duration", duration);
        json.insert("mark", mark);
        QJsonArray array;
        foreach (Artist artist, artists)
            array.append(artist.toJson());
        json.insert("artists", array);
        json.insert("album", album.toJson());
        if (addTime)
            json.insert("addTime", addTime);
        if (!addBy.isEmpty())
            json.insert("addBy", addBy);
        return json;
    }

    bool isValid() const
    {
        return id;
    }

    bool operator==(const Song& song)
    {
        return this->id == song.id;
    }

    QString simpleString() const
    {
        return name + " - " + artistNames;
    }

    void setAddDesc(QString by)
    {
        this->addBy = by;
        this->addTime = QDateTime::currentSecsSinceEpoch();
    }
};

struct PlayListCreator
{
    QString nickname;
    QString signature;
    QString avatarUrl;
};

struct PlayList
{
    QString name;
    qint64 id;
    QString description;
    QStringList tags;
    int playCount;
    PlayListCreator creator;
};

typedef QList<Song> SongList;
typedef QList<PlayList> PlayListList;

#endif // SONGBEANS_H
