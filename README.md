神奇弹幕
===

## 介绍

一个Bilibili直播弹幕机器人。

人性化交互，支持弹幕聊天、观众交互、数据统计、防喷禁言等等。


## 功能

- 实时显示弹幕
- 小窗弹幕聊天
- 保存弹幕历史
- 每日数据统计
- 自动欢迎进入
- 自动感谢送礼
- 自动感谢关注
- 点歌自动复制
- 查看点歌历史
- 新人发言提示
- 房管一键禁言
- 快速百度搜索
- 外语自动翻译
- 智能沙雕回复
- 定时弹幕任务
- 动态编程变量
- 特别关心高亮
- 智能昵称简化
- 个人专属昵称
- 粉丝变化提示
- 开播下播提醒
- 用户弹幕历史
- 定时连接后台
- 喷子自动拉黑
- 新人快速禁言
- 跳转用户主页
- 查看粉丝牌子
- 查看礼物价值
- 用户累计数据
- 一个吃瓜偷塔
- 可编程变量集
- 变量四则运算



![截图1](pictures/screenshot1.png)

![截图2](pictures/screenshot2.png)



## 可编程变量列表

| 变量             | 描述             | 注意事项                                     |
| ---------------- | ---------------- | -------------------------------------------- |
| uname            | 用户名           | 需要用户值，例如定时消息，则只是空字符串     |
| username         | 用户名           |                                              |
| nickname         | 用户名           |                                              |
| ai_name          | 用户智能昵称     | 优先专属昵称，其次简写昵称，无简写则用原昵称 |
| local_name       | 用户专属昵称     |                                              |
| simple_name      | 用户简写昵称     |                                              |
| level            | 用户等级         | 进入直播间没有level                          |
| text             | 当前弹幕消息     | 几乎用不到                                   |
| come_count       | 用户进来次数     |                                              |
| come_time        | 用户上次进来时间 | 第一次进来是0                                |
| gift_gold        | 当前礼物金瓜子   | 非送礼答谢则没有                             |
| gift_silver      | 当前礼物银瓜子   |                                              |
| gift_name        | 当前礼物名字     |                                              |
| gift_num         | 当前礼物数量     |                                              |
| total_gold       | 用户总共金瓜子   | 该用户一直以来赠送的所有金瓜子数量           |
| total_silver     | 用户总共银瓜子   | 同上                                         |
| anchor_roomid    | 粉丝牌房间ID     | 进入、弹幕才有粉丝牌                         |
| medal_name       | 粉丝牌名称       |                                              |
| medal_level      | 粉丝牌等级       |                                              |
| medal_up         | 粉丝牌UP主名称   | 只有弹幕消息有                               |
| new_attention    |                  |                                              |
| today_come       |                  |                                              |
| today_come       |                  |                                              |
| today_newbie_msg |                  |                                              |
| today_danmaku    |                  |                                              |
| today_fans       |                  |                                              |
| fans_count       |                  |                                              |
| today_gold       |                  |                                              |
| today_silver     |                  |                                              |
| today_guard      |                  |                                              |
| time_hour        |                  |                                              |
| time_minute      |                  |                                              |
| time_second      |                  |                                              |
| time_day         |                  |                                              |
| time_month       |                  |                                              |
| time_year        |                  |                                              |
| time_day_week    |                  |                                              |
| time_day_year    |                  |                                              |
| timestamp13      |                  |                                              |
| pking            |                  |                                              |



## 弹幕CMD列表

根据字节流中的`protocol`，等于 `2` 时body部分需要用`zlib.uncompress`解压

```
"ROOM_ADMINS" //房管列表
"room_admin_entrance"
"ONLINE_RANK_TOP3"
"ONLINE_RANK_COUNT"
"ONLINE_RANK_V2"
"TRADING_SCORE" //每日任务
"MATCH_ROOM_CONF" //赛事房间配置
"HOT_ROOM_NOTIFY" //热点房间
"MATCH_TEAM_GIFT_RANK" //赛事人气比拼
"ACTIVITY_MATCH_GIFT" //赛事礼物
"PK_BATTLE_PRE" //人气pk
"PK_BATTLE_START" //人气pk
"PK_BATTLE_PROCESS" //人气pk
"PK_BATTLE_END" //人气pk
"PK_BATTLE_RANK_CHANGE" //人气pk
"PK_BATTLE_SETTLE_USER" //人气pk
"PK_BATTLE_SETTLE_V2" //人气pk
"PK_BATTLE_SETTLE" //人气pk
"SYS_MSG" //系统消息
"ROOM_SKIN_MSG"
"GUARD_ACHIEVEMENT_ROOM"
"ANCHOR_LOT_START" //天选之人开始
"ANCHOR_LOT_CHECKSTATUS"
"ANCHOR_LOT_END" //天选之人结束
"ANCHOR_LOT_AWARD" //天选之人获奖
"COMBO_SEND"
"INTERACT_WORD"
"ACTIVITY_BANNER_UPDATE_V2"
"NOTICE_MSG"
"ROOM_BANNER"
"ONLINERANK"
"WELCOME"
"HOUR_RANK_AWARDS"
"ROOM_RANK"
"ROOM_SHIELD"
"USER_TOAST_MSG" //大航海购买信息
"WIN_ACTIVITY" //活动
"SPECIAL_GIFT" //节奏风暴
"GUARD_BUY" / //大航海购买
"WELCOME_GUARD" //大航海进入
"DANMU_MSG" //弹幕
"ROOM_CHANGE" //房间信息分区改变
"ROOM_SILENT_OFF" //禁言结束
"ROOM_SILENT_ON" //禁言开始
"SEND_GIFT" //礼物
"ROOM_BLOCK_MSG" //封禁
"PREPARING" //下播
"LIVE" //开播
"SUPER_CHAT_ENTRANCE" //SC入口
"SUPER_CHAT_MESSAGE_DELETE" //SC删除
"SUPER_CHAT_MESSAGE" //SC
"SUPER_CHAT_MESSAGE_JPN" //SC
"PANEL" //排行榜
"ENTRY_EFFECT" / //进入特效
"ROOM_REAL_TIME_MESSAGE_UPDATE" / //粉丝数
```



> 参考资料：
>
> - B站API列表：https://github.com/SocialSisterYi/bilibili-API-collect
>
> - 直播WS信息流：https://github.com/SocialSisterYi/bilibili-API-collect/blob/master/live/message_stream.md
>
> - 直播数据包解析：https://segmentfault.com/a/1190000017328813?utm_source=tag-newest
>
> - 部分CMD包分析：https://github.com/czp3009/bilibili-api/tree/master/record/%E7%9B%B4%E6%92%AD%E5%BC%B9%E5%B9%95
>
> - Qt解压zlib：https://blog.csdn.net/doujianyoutiao/article/details/106236207
>
