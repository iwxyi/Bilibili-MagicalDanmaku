#ifndef SONGBEANS_H
#define SONGBEANS_H

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QDateTime>
#include <QDebug>
#include "qt_compat.h"

#define JSON_VAL_LONG(json, x) static_cast<qint64>(json.value(#x).toDouble())
#define JVAL_LONG(x) static_cast<qint64>(json.value(#x).toDouble())
#define JVAL_INT(x) json.value(#x).toInt()
#define JVAL_STR(x) json.value(#x).toString()
#define snum(x) QString::number(x)
#define cast(x,y)  static_cast<x>(y)

enum MusicSource
{
    UnknowMusic = -1,
    NeteaseCloudMusic = 0,
    QQMusic = 1,
    MiguMusic = 2,
    KugouMusic = 3,
    LocalMusic = 10
};

struct Artist
{
    qint64 id = 0;
    QString mid;
    QString name;
    QString faceUrl;

    Artist()
    {}

    Artist(QString name)
    {
        this->name = name;
    }

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
        return artist;
    }

    static Artist fromMiguMusicJson(QJsonObject json)
    {
        Artist artist;
        artist.id = JVAL_LONG(id);
        artist.name = JVAL_STR(name);
        return artist;
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        if (id)
            json.insert("id", id);
        if (!mid.isEmpty())
            json.insert("mid", mid);
        json.insert("name", name);
        if (!faceUrl.isEmpty())
            json.insert("faceUrl", faceUrl);
        return json;
    }
};

struct Album
{
    qint64 id = 0;
    QString mid;
    qint64 audio_id = 0;
    QString name;
    int size = 0;
    int mark = 0;
    QString picUrl;

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
        album.id = JVAL_LONG(albumid);
        album.mid = JVAL_STR(albummid);
        album.name = JVAL_STR(albumname);
        return album;
    }

    static Album fromMiguMusicJson(QJsonObject json)
    {
        Album album;
        album.id = JVAL_LONG(id);
        album.mid = JVAL_STR(mid);
        album.name = JVAL_STR(name);
        album.picUrl = JVAL_STR(picUrl);
        return album;
    }

    static Album fromKugouMusicJson(const QJsonObject& json)
    {
        Album album;
        album.id = JVAL_STR(album_id).toLongLong();
        album.audio_id = JVAL_LONG(album_audio_id);
        album.name = JVAL_STR(album_name);
        return album;
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        if (id)
            json.insert("id", id);
        if (!mid.isEmpty())
            json.insert("mid", mid);
        json.insert("name", name);
        if (size)
            json.insert("size", size);
        if (mark)
            json.insert("mark", mark);
        if (!picUrl.isEmpty())
            json.insert("picUrl", picUrl);
        if (audio_id)
            json.insert("audio_id", audio_id);
        return json;
    }
};

struct Song
{
    qint64 id = 0;
    QString mid;
    QString name;
    int duration = 0; // 毫秒
    int mark = 0;
    QList<Artist> artists;
    Album album;
    QString artistNames;
    QString url;
    qint64 addTime;
    QString mediaId;
    QString addBy;
    MusicSource source = NeteaseCloudMusic;
    char m_padding2[4];
    QString filePath;

    static Song fromJson(QJsonObject json)
    {
        Song song;
        song.id = JVAL_LONG(id);
        song.mid = JVAL_STR(mid);
        song.name = JVAL_STR(name);

        QJsonArray array = json.value("artists").toArray();
        if (array.empty())
            array = json.value("ar").toArray();
        QStringList artistNameList;
        foreach (QJsonValue val, array)
        {
            Artist artist = Artist::fromJson(val.toObject());
            song.artists.append(artist);
            artistNameList.append(artist.name);
        }
        song.artistNames = artistNameList.join("/");

        QJsonObject album = json.value("album").toObject();
        if (album.isEmpty())
            album = json.value("al").toObject();
        song.album = Album::fromJson(album);
        song.duration = JVAL_INT(duration);
        if (song.duration == 0)
            song.duration = JVAL_INT(dt);
        song.mark = JVAL_INT(mark);

        if (json.contains("mediaId"))
            song.mediaId = JVAL_STR(mediaId);
        if (json.contains("addTime"))
            song.addTime = JVAL_LONG(addTime);
        if (json.contains("addBy"))
            song.addBy = JVAL_STR(addBy);
        if (json.contains("source"))
            song.source = static_cast<MusicSource>(JVAL_INT(source));
        if (json.contains("filePath"))
            song.filePath = JVAL_STR(filePath);
        return song;
    }

    static Song fromQQMusicJson(QJsonObject json)
    {
        Song song;
        song.id = JVAL_LONG(songid);
        if (song.id == 0)
            song.id = JVAL_LONG(id);
        if (json.contains("songmid"))
            song.mid = JVAL_STR(songmid);
        else if (json.contains("mid"))
            song.mid = JVAL_STR(mid);
        song.name = JVAL_STR(songname);
        if(song.name.isEmpty())
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

        QJsonObject album = json.value("album").toObject();
        if (album.isEmpty())
            song.album = Album::fromQQMusicJson(json);
        else
            song.album = Album::fromJson(album);
        song.duration = JVAL_INT(interval) * 1000; // 秒数，转毫秒
        song.mediaId = JVAL_STR(media_mid);

        if (json.contains("addTime"))
            song.addTime = JVAL_LONG(addTime);
        if (json.contains("addBy"))
            song.addBy = JVAL_STR(addBy);
        song.source = QQMusic;
        return song;
    }

    static Song fromMiguMusicJson(QJsonObject json)
    {
        Song song;
        song.id = JVAL_STR(id).toLongLong();
        if (json.contains("cid"))
            song.mid = JVAL_STR(cid);
        song.name = JVAL_STR(name);
        song.url = JVAL_STR(url);

        QJsonArray array = json.value("artists").toArray();
        QStringList artistNameList;
        foreach (QJsonValue val, array)
        {
            Artist artist = Artist::fromMiguMusicJson(val.toObject());
            song.artists.append(artist);
            artistNameList.append(artist.name);
        }
        song.artistNames = artistNameList.join("/");

        song.album = Album::fromMiguMusicJson(json.value("album").toObject());

        if (json.contains("addTime"))
            song.addTime = JVAL_LONG(addTime);
        if (json.contains("addBy"))
            song.addBy = JVAL_STR(addBy);
        song.source = MiguMusic;
        return song;
    }

    static Song fromKugouMusicJson(QJsonObject json)
    {
        Song song;

        song.id = qMax(0, json.value("trans_param").toObject().value("cid").toInt());
        song.mid = JVAL_STR(hash);
        song.name = JVAL_STR(songname).replace("<em>", "").replace("</em>", "");
        song.artistNames = JVAL_STR(singername); // 没有歌手详细信息，直接设置了
        song.artists.append(Artist(song.artistNames));
        song.duration = JVAL_INT(duration) * 1000; // 秒数，转毫秒
        song.album = Album::fromKugouMusicJson(json);

        if (json.contains("addTime"))
            song.addTime = JVAL_LONG(addTime);
        if (json.contains("addBy"))
            song.addBy = JVAL_STR(addBy);
        song.source = KugouMusic;
        return song;
    }

    static Song fromNeteaseShareJson(QJsonObject json)
    {
        Song song;
        song.id = JVAL_LONG(id);
        song.name = JVAL_STR(name);
        QJsonArray array = json.value("ar").toArray();
        QStringList artistNameList;
        foreach (QJsonValue val, array)
        {
            Artist artist = Artist::fromJson(val.toObject());
            song.artists.append(artist);
            artistNameList.append(artist.name);
        }
        song.artistNames = artistNameList.join("/");
        song.album = Album::fromJson(json.value("al").toObject());
        song.duration = JVAL_INT(dt); // 毫秒
        song.mark = JVAL_INT(mark);
        song.source = NeteaseCloudMusic;
        return song;
    }

    static Song fromKugouShareJson(QJsonObject json)
    {
        Song song;

        song.id = qMax(0, json.value("trans_param").toObject().value("cid").toInt());
        song.mid = JVAL_STR(hash);
        QString fileName = JVAL_STR(filename); // 歌手2、歌手2 - 歌名
        if (fileName.isEmpty())
            fileName = JVAL_STR(name);
        int pos = fileName.indexOf("-");
        if (pos > -1)
        {
            song.name = fileName.right(fileName.length() - pos - 1).trimmed();
            QStringList ars = fileName.left(pos).trimmed().split("、", SKIP_EMPTY_PARTS);
            foreach (auto a, ars)
                song.artists.append(Artist(a));
            song.artistNames = ars.join("/");
        }
        song.duration = JVAL_INT(duration) * 1000; // 秒数，转毫秒
        if (song.duration == 0)
            song.duration = JVAL_INT(timelen);
        song.album = Album::fromKugouMusicJson(json);
        song.album.name = JVAL_STR(remark);

        if (json.contains("addTime"))
            song.addTime = JVAL_LONG(addTime);
        if (json.contains("addBy"))
            song.addBy = JVAL_STR(addBy);
        song.source = KugouMusic;
        return song;
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json.insert("id", id);
        json.insert("mid", mid);
        json.insert("name", name);
        if (!url.isEmpty())
        json.insert("url", url);
        json.insert("duration", duration);
        if (mark)
            json.insert("mark", mark);
        QJsonArray array;
        foreach (Artist artist, artists)
            array.append(artist.toJson());
        json.insert("artists", array);
        json.insert("album", album.toJson());
        if (!mediaId.isEmpty())
            json.insert("mediaId", mediaId);
        if (addTime)
            json.insert("addTime", addTime);
        if (!addBy.isEmpty())
            json.insert("addBy", addBy);
        json.insert("source", static_cast<int>(source));
        if (!filePath.isEmpty())
            json.insert("filePath", filePath);
        return json;
    }

    bool isValid() const
    {
        return id || !mid.isEmpty();
    }

    bool operator==(const Song& song) const
    {
        return this->id == song.id && this->mid == song.mid;
    }

    bool operator!=(const Song& song) const
    {
        return this->id != song.id || this->mid != song.mid;
    }

    bool isSame(const Song& song, bool art = false) const
    {
        if (art)
            if (!this->artistNames.contains(song.artistNames)
                    && !song.artistNames.contains(this->artistNames))
                return false;
        return this->name.contains(song.name)
                || song.name.contains(this->name);
    }

    QString simpleString() const
    {
        if (artistNames.isEmpty())
            return name;
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
