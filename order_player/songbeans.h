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
#define cast(x,y)  static_cast<x>(y)

enum MusicSource
{
    NeteaseCloudMusic,
    QQMusic
};

struct Artist
{
    qint64 id = 0;
    QString mid;
    QString name;
    QString faceUrl;

    static Artist fromJson(QJsonObject json)
    {
        Artist artist;
        artist.id = JVAL_LONG(id);
        if (json.contains("mid"))
            artist.mid = JVAL_STR(mid);
        artist.name = JVAL_STR(name);
        artist.faceUrl = JVAL_STR(img1v1Url);
        return artist;
    }

    static Artist fromQQMusicJson(QJsonObject json)
    {
        Artist artist;
        artist.id = JVAL_LONG(id);
        artist.mid = JVAL_STR(mid);
        artist.name = JVAL_STR(name);
//        artist.faceUrl = JVAL_STR(img1v1Url);
        return artist;
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json.insert("id", id);
        json.insert("mid", mid);
        json.insert("name", name);
        json.insert("faceUrl", faceUrl);
        return json;
    }
};

struct Album
{
    qint64 id = 0;
    QString mid;
    QString name;
    int size = 0;
    int mark = 0;

    static Album fromJson(QJsonObject json)
    {
        Album album;
        album.id = JVAL_LONG(id);
        if (json.contains("mid"))
            album.mid = JVAL_STR(mid);
        album.name = JVAL_STR(name);
        album.size = JVAL_INT(size);
        album.mark = JVAL_INT(mark);
        return album;
    }

    static Album fromQQMusicJson(QJsonObject json)
    {
        Album album;
        album.id = JVAL_LONG(id);
        album.mid = JVAL_STR(mid);
        album.name = JVAL_STR(name);
//        album.size = JVAL_INT(size);
//        album.mark = JVAL_INT(mark);
        return album;
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json.insert("id", id);
        json.insert("mid", mid);
        json.insert("name", name);
        json.insert("size", size);
        json.insert("mark", mark);
        return json;
    }
};

struct Song
{
    qint64 id = 0;
    QString mid;
    QString name;
    int duration = 0;
    int mark = 0;
    QList<Artist> artists;
    Album album;
    QString artistNames;
    qint64 addTime;
    QString addBy;
    MusicSource source = NeteaseCloudMusic;

    static Song fromJson(QJsonObject json)
    {
        Song song;
        song.id = JVAL_LONG(id);
        song.mid = JVAL_STR(mid);
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
        song.duration = JVAL_INT(duration); // 毫秒
        song.mark = JVAL_INT(mark);
        if (json.contains("addTime"))
            song.addTime = JVAL_LONG(addTime);
        if (json.contains("addBy"))
            song.addBy = JVAL_STR(addBy);
        if (json.contains("source"))
            song.source = static_cast<MusicSource>(JVAL_INT(source));
        return song;
    }

    static Song fromQQMusicJson(QJsonObject json)
    {
        Song song;
        song.id = JVAL_LONG(id);
        if (json.contains("mid"))
            song.mid = JVAL_STR(mid);
        song.name = JVAL_STR(name);
        QJsonArray array = json.value("singer").toArray();
        QStringList artistNameList;
        foreach (QJsonValue val, array)
        {
            Artist artist = Artist::fromQQMusicJson(val.toObject());
            song.artists.append(artist);
            artistNameList.append(artist.name);
        }
        song.artistNames = artistNameList.join("/");
        song.album = Album::fromQQMusicJson(json.value("album").toObject());
        song.duration = JVAL_INT(interval) * 1000; // 秒数，转毫秒
        song.mark = JVAL_INT(mark);
        if (json.contains("addTime"))
            song.addTime = JVAL_LONG(addTime);
        if (json.contains("addBy"))
            song.addBy = JVAL_STR(addBy);
        song.source = QQMusic;
        return song;
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json.insert("id", id);
        json.insert("mid", mid);
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
        json.insert("source", static_cast<int>(source));
        return json;
    }

    bool isValid() const
    {
        return id;
    }

    bool operator==(const Song& song) const
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

    bool is(MusicSource source) const
    {
        return this->source == source;
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
    MusicSource source = NeteaseCloudMusic;
};

typedef QList<Song> SongList;
typedef QList<PlayList> PlayListList;

#endif // SONGBEANS_H
