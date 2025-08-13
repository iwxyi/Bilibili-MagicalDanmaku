#ifndef NOTICEBEAN_H
#define NOTICEBEAN_H

#include "myjson.h"
#include <QRegularExpression>

enum NoticeType
{
    NoticeType_Comment = 31, // 评论
    NoticeType_Follow = 33, // 关注
    NoticeType_Digg = 41, // 点赞
    NoticeType_At = 45, // 被@
    NoticeType_Recommend = 9009, // 推荐
};

struct UserInfo
{
    QString uid;
    QString nickname;
    QString unique_id;
    QString avatar;
    int follow_status;
    bool is_block;

    UserInfo(){}
    UserInfo(const MyJson &json)
    {
        uid = json.s("uid");
        unique_id = json.s("unique_id");
        nickname = json.s("nickname");
        follow_status = json.i("follow_status");
        is_block = json.b("is_block");
        if (json.contains("avatar_thumb"))
            avatar = json.o("avatar_thumb").a("url_list").first().toString();
    }
};

struct AwemeInfo
{
    QString aweme_id;
    QString desc;
    qint64 create_time;
    UserInfo author;
    QString cover;

    AwemeInfo(){}
    AwemeInfo(const MyJson &json)
    {
        aweme_id = json.s("aweme_id");
        desc = json.s("desc");
        create_time = json.l("create_time");
        if (json.contains("author"))
            author = UserInfo(json.o("author"));
        if (json.contains("video"))
            cover = json.o("video").o("cover").a("url_list").first().toString();
        if (json.contains("images")) // 图片作品会有多个下载的链接
        {}
    }
};

struct CommentInfo
{
    QString cid;
    QString text;
    qint64 create_time;
    UserInfo user;

    CommentInfo(){}
    CommentInfo(const MyJson &json)
    {
        cid = json.s("cid");
        text = json.s("text");
        create_time = json.l("create_time");
        if (json.contains("user"))
            user = UserInfo(json.o("user"));
        if (json.contains("sticker")) // 表情包评论
        {}
    }
};

struct NoticeInfo
{
    int type;
    qint64 nid;
    qint64 create_time;
    AwemeInfo aweme;
    UserInfo user;
    QString content;
    CommentInfo comment;

    QString _text; // 描述
    MyJson _originJson; // 保留原始JSON数据

    NoticeInfo(){}
    NoticeInfo(const MyJson &json)
    {
        _originJson = json;
        type = json.i("type");
        nid = json.l("nid");
        create_time = json.l("create_time");

        if (json.contains("digg"))
        {
            MyJson digg = json.o("digg");
            aweme = AwemeInfo(digg.o("aweme"));
            user = UserInfo(digg.a("from_user").first().toObject());
        }

        if (json.contains("comment"))
        {
            MyJson comment_ = json.o("comment");
            comment = CommentInfo(comment_.o("comment"));
            user = comment.user;
            aweme = AwemeInfo(comment_.o("aweme"));
        }

        if (json.contains("follow"))
        {
            MyJson follow = json.o("follow");
            user = UserInfo(follow.o("from_user"));
            content = follow.s("content"); // “关注了你”
        }

        if (json.contains("at"))
        {
            MyJson at = json.o("at");
            user = UserInfo(at.o("user_info"));
            content = at.s("content"); // “@A @B @C”
            MyJson reply_comment = at.o("reply_comment"); // 可能是空的
            aweme = AwemeInfo(at.o("aweme"));
        }

        if (json.contains("interactive_notice"))
        {
            MyJson interactive_notice = json.o("interactive_notice");
            content = interactive_notice.s("content"); // “推荐了你的视频”
            user = UserInfo(interactive_notice.a("from_user").first().toObject());
            aweme = AwemeInfo(interactive_notice.o("aweme"));
        }

        // 生成描述
        switch (type)
        {
        case NoticeType_Comment:
            _text = QString("%1 评论：%2").arg(user.nickname).arg(comment.text);
            break;
        case NoticeType_Digg:
            _text = QString("%1 点赞了你的视频").arg(user.nickname);
            break;
        case NoticeType_Follow:
            _text = QString("%1 关注了你").arg(user.nickname);
            break;
        case NoticeType_At:
            _text = QString("%1@了你").arg(user.nickname);
            break;
        case NoticeType_Recommend:
            _text = QString("%1 推荐了你的视频").arg(user.nickname);
            break;
        default:
            _text = QString("未知类型：%1").arg(type);
        }
    }

    QString getTypeString() const
    {
        switch (type)
        {
        case NoticeType_Comment:
            return "评论";
        case NoticeType_Digg:
            return "点赞";
        case NoticeType_Follow:
            return "关注";
        case NoticeType_At:
            return "被@";
        case NoticeType_Recommend:
            return "推荐";
        default:
            return "？";
        }
    }

    QString toString() const
    {
        return _text;
    }

    bool contains(const QRegularExpression& key) const
    {
        return _text.contains(key)
            || aweme.desc.contains(key)
            || aweme.aweme_id.contains(key)
            || aweme.author.uid.contains(key)
            || comment.text.contains(key)
            || user.nickname.contains(key)
            || user.unique_id.contains(key);
    }
};

#endif // NOTICEBEAN_H
