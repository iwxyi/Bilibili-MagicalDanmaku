神奇弹幕
===

## 介绍

一个Bilibili直播弹幕姬+答谢姬+点歌姬+录播姬+回复姬。

人性化交互，支持弹幕聊天、观众互动、自动点歌、数据统计、防喷禁言、原画录播等等。


## 神奇

神奇在何处？哦，也就下面这些：

- 根据观众自身数据（上次进入、等级、粉丝牌、弹幕等），按条件智能欢迎
- 根据礼物价值，使用不同语气程度发送答谢，支持发送多条
- 按等级、粉丝牌等多个条件，并筛选关键词自动禁言
- 外语自动翻译、AI闲聊回复、生僻字读音等自动化功能
- 大乱斗查看双方串门的粉丝、同步视频PK对面的消息
- 粉丝变化、大乱斗状况、新人发言等种种数据
- 全自动点歌姬，优雅的界面，以及播放时间预计
- 单个粉丝设置特别关心、强提醒、专属昵称等
- 小窗闲聊，上班摸鱼挂后台听直播必备
- 查看每名观众进来次数、礼物总计、弹幕记录、判断机器人等
- 大量可编程运算，超高自由度的弹幕机器人
- 更多神奇的隐藏（不可公开）的小功能


## 弹幕

### 答谢姬

- 自动欢迎进入
- 自动感谢送礼
- 实时感谢关注
- 冷却避免刷屏
- 合并礼物连击
- 设置礼物别名
- 智能昵称简化
- 目标强制欢迎
- 舰长单独欢迎
- 自动语音播报
- 个人专属昵称
- 开播下播提醒
- 可编程变量集
- 动态条件运算
- 欢迎的黑名单
- 支持远程控制
- 支持语音朗读


### 弹幕姬

- 实时显示弹幕
- 小窗弹幕聊天
- 定时弹幕任务
- 简约聊天模式
- 点歌自动复制
- 查看点歌历史
- 保存弹幕历史
- 每日数据统计
- 外语自动翻译
- 智能沙雕回复
- 特别关心高亮
- 查看粉丝牌子
- 查看礼物价值
- 查看昵称拼音
- 粉丝变化提示
- 房管一键禁言
- 新人发言提示
- 弹幕自由复制
- 定时连接后台
- 喷子自动拉黑
- 新人快速禁言
- 跳转用户主页
- 用户累计数据
- 用户弹幕历史
- 吃瓜自动偷塔
- 大乱斗反偷塔
- PK 历史次数
- PK 偷塔概率
- 快速百度搜索
- 大乱斗的串门
- 对面消息同步
- 机器小号判断
- 弹幕语音朗读
- 弹幕全屏滚动
- 固定图片背景
- 多张图片轮播
- 自动切换勋章


### 点歌姬

- 网易云的音源
- 按关键词搜索
- 点歌自动播放
- 自动点歌队列
- 点歌防止刷屏
- 预计时长反馈
- 背景封面模糊
- 提取主题色调
- 查看桌面歌词
- 查看滚动歌词
- 收藏夹与历史


### 录播姬

- 自动开始录播
- 分割文件大小
- 断联自动重录


### 回复姬

- 自定义关键词
- 筛选用户条件
- 多条随机回复
- 支持编程变量
- 数量不做限制


### 视频流

- 查看直播视频
- 预览PK对面


### 其他

- 投稿歌词字幕
- 自动切换勋章
- 每日自动签到
- 自动参与天选
- 直播状态查询


## 截图


![房间](pictures/screenshot1.png)

![弹幕](pictures/screenshot2.png)

![自动欢迎](pictures/screenshot3.png)

![答谢](pictures/screenshot4.png)

![点歌](pictures/screenshot5.png)

![开播、录播](pictures/screenshot6.png)

![定时任务](pictures/screenshot7.png)

![全屏滚动、语音朗读](pictures/screenshot8.png)

![喷子禁言](pictures/screenshot9.png)

![实时弹幕](pictures/danmaku1.png)

![自动偷塔、反偷塔](pictures/danmaku2.png)

![音乐](pictures/music1.png)

![音乐](pictures/music2.png)

![视频字幕制作](pictures/subtitle1.png)



## 使用技巧

绿色版，开箱即用，输入房间号自动连接。

### 实时弹幕

实时弹幕支持如同聊天软件的水群小窗，`Shift+Alt+D`快速弹出（后台有效），ESC返回之前的程序

若是上班摸鱼，可以设置闲聊模式（仅显示弹幕）+简约模式（去头像、彩色），背景全透明，文字半透明灰色，不注意绝对很难看出来。



### 设置账号Cookie

懒，所以没有做登录功能，如果要发送弹幕，直接使用浏览器Cookie进行登录。

**步骤**：

1. 浏览器上按`F12`打开【开发者工具】，Network-XHR
2. 进入或刷新B站主页（没登录的需登录），Name多了一排，点其中任意一个（若找不到Cookie就换个）
3. 复制如下图的cookie至菜单“账号”中的“设置Cookie”，即可进行发送。

![1606054818666](pictures/cookie.png)

使用Cookie也是为了保证账号安全，当程序借给别人时，自己可以远程退出Bilibili账号，之后则需要重新cookie和data。

> 如果输入房间号后一直显示“获取房间信息...”，需先安装VC_redist：https://aka.ms/vs/15/release/vc_redist.x64.exe



### 欢迎弹幕

以下多项功能支持自定义弹幕、大量可编程变量，具体说明请参考下面的“[可编程变量与运算](#programing)”。

- 普通的欢迎：`%ai_name%，%greet%%tone/punc%`
  将发送：某某某，早上好啊~

- **未戴牌子**不欢迎，除非是舰长或强提醒：`[%guard% = 0, %anchor_roomid%!=%room_id%, !%strong_notify%]**`
  两个`*`表示优先级，覆盖没有`*`或1个`*`的弹幕，空弹幕表示不发送
- 欢迎**舰长**：`[%guard%]*欢迎舰长 %ai_name% 回家~`
- 一周至一个月没来：`[%come_time%>%timestamp%-3600*24*30, %come_time%<%timestamp%-3600*24*7, 0]*%ai_name%，太久没来了哦~`
- **大乱斗对面**来人：`[%pk_opposite%]***欢迎%ai_name%串门哦~`

### 送礼答谢

- 普通的答谢：`谢谢 %ai_name% 的%gift_name%~`
- **小礼物**不理：`[%gift_gold%=0, %gift_silver%<1000, %gift_num% < 10]**`
  也可以写作：`[%coin_type%=0, %gift_coin%<1000, %gift_num% < 10]**`
  1000以下的银瓜子礼物且数量小于10，则忽略。例如24个小心心，则感谢
- **超过80元**的礼物：`[%gift_gold%>=80000]*哇噢！感谢 %ai_name% 的%gift_name%！\n老板大气！！！！！！`
  `\n`表示换行，将分作两条弹幕发送（可以配合`<h1>`放大字体，仅部分弹幕插件有效）

### 定时任务

示例：判断**今日是否有大航海**

- `[%today_guard%=0]今天XX等到新的舰长了吗？\n老板们救救可怜的XX吧~`

- `[%today_guard%>0]今天XX等到了新的舰长，还有老板想上船嘛`

<div id='programing'/>
## 可编程变量与运算

> 这一块比较专业，所以单独拎出来写教程。

弹幕的候选列表中，支持一系列的可编程变量、简单逻辑运算、简单算术运算。

使用两个 `%` 包括的英文，则为变量。数据变量例如 `%uname%`，与当前的一系列数据相关，例如礼物价值、今日人数等等；招呼变量例如 `%greet%`，自动替换为当前时间段对应的招呼语，例如候选项表达式 `%ai_name%，%greet%%tone/punc%`，在早上可能是“早上好啊，某某某”，在下午可能是“某某某，下午好~”，在晚上又会是“晚饭吃了吗，某某某~”。

示例：

- 简单的欢迎：`欢迎 %ai_name% 光临~`
- 动态语气词：`%ai_name%，%greet%%tone/punc%`

看下去，有更多例子。

### 数据变量

| 变量             | 描述                    | 注意事项                                     |
| ---------------- | ----------------------- | -------------------------------------------- |
| uid              | 用户ID                  | 是一串数字，确定唯一用户                     |
| uname            | 用户名                  | 需要用户值，例如定时消息，则只是空字符串     |
| username         | 用户名                  | 和上面一模一样                               |
| nickname         | 用户名                  | 同上                                         |
| ai_name          | 用户智能昵称            | 优先专属昵称，其次简写昵称，无简写则用原昵称 |
| local_name       | 用户专属昵称            | 实时弹幕中右键-设置专属昵称                  |
| simple_name      | 用户简写昵称            | 去除前缀后缀各种字符                         |
| level            | 用户等级                | 进入直播间没有level                          |
| text             | 当前弹幕消息            | 几乎用不到                                   |
| come_count       | 用户进来次数            |                                              |
| come_time        | 用户上次进来时间        | 第一次进来是0                                |
| gift_gold        | 当前礼物金瓜子          | 非送礼答谢则没有                             |
| gift_silver      | 当前礼物银瓜子          |                                              |
| gift_coin        | 当前礼物价值(不分金银)  |                                              |
| coin_gold        | 是否是金瓜子礼物        | 金瓜子是1，银瓜子是0                         |
| gift_name        | 当前礼物名字            | 如果设置了别名，则使用别名                   |
| origin_gift_name | 原始礼物名字(非别名)    |                                              |
| gift_num         | 当前礼物数量            |                                              |
| total_gold       | 用户总共金瓜子          | 该用户一直以来赠送的所有金瓜子数量           |
| total_silver     | 用户总共银瓜子          | 同上                                         |
| anchor_roomid    | 粉丝牌房间ID            | 进入、弹幕才有粉丝牌                         |
| medal_name       | 粉丝牌名称              |                                              |
| medal_level      | 粉丝牌等级              |                                              |
| medal_up         | 粉丝牌UP主名称          | 只有弹幕消息有                               |
| nickname_len     | 昵称长度                |                                              |
| giftname_len     | 礼物名字长度            |                                              |
| name_sum_len     | 昵称长度+礼物名字长度   |                                              |
| ainame_sum_len   | 短昵称长度+礼物名字长度 |                                              |
| new_attention    | 是否是新关注            | 最近50个关注内                               |
| pk_opposite      | 大乱斗对面进来          | 需开启串门提示                               |
| today_come       | 今日进来人次            | 每个人可能重复进入                           |
| today_newbie_msg | 今日新人人数            |                                              |
| today_danmaku    | 今日弹幕总数            |                                              |
| today_fans       | 今日新增粉丝数          |                                              |
| fans_count       | 当前总粉丝数            |                                              |
| today_gold       | 今日收到金瓜子总数      |                                              |
| today_silver     | 今日收到银瓜子总数      |                                              |
| today_guard      | 今日上船人次            | 续多个月算多次                               |
| admin            | 是否是房管              |                                              |
| guard            | 是否是舰长              |                                              |
| admin_or_guard   | 是否是房管或舰长        |                                              |
| time_hour        | 当前小时                |                                              |
| time_minute      | 当前分钟                |                                              |
| time_second      | 当前秒                  |                                              |
| time_day         | 当前日期                |                                              |
| time_month       | 当前月份                |                                              |
| time_year        | 当前年份                |                                              |
| time_day_week    | 当前星期                | 1~7                                          |
| time_day_year    | 当前一年中第几天        |                                              |
| timestamp        | 当前10位时间戳          | 可用于比较进入时间、多久没来等               |
| timestamp13      | 当前13位时间戳          |                                              |
| pking            | 当前是否在大乱斗        | 是：1，否：0                                 |
| living           | 当前是否已开播          | 是：1，否：0                                 |
| room_id          | 直播间ID                |                                              |
| room_name        | 直播间标题              |                                              |
| up_uid           | 主播UID                 |                                              |
| up_name          | 主播名字                |                                              |
| my_uid           | 机器人UID               |                                              |
| my_uname         | 机器人名字              |                                              |
| care             | 特别关系                | 是：1，否：0                                 |
| strong_notify    | 强提醒                  | 是：1，否：0                                 |
| not_welcome      | 不自动欢迎              | 不欢迎：1，欢迎：0                           |
| blocked          | 被禁言中                | 禁言：1，未禁言0                             |

### 招呼变量列表

| 变量      | 描述           | 示例                                              |
| --------- | -------------- | ------------------------------------------------- |
| hour      | 时辰           | 早上/中午/下午/晚上                               |
| greet     | 招呼           | 您好、早上好、晚上好                              |
| all_greet | 带语气词的招呼 | 你好、早上好啊、午饭吃了吗、晚上好呀、怎么还没睡~ |
| tone      | 语气词         | “啊”或“呀”                                        |
| lela      | 语气词         | “了”或“啦”                                        |
| punc      | 标点           | “~”或“！”                                         |
| tone/punc | 语气词或标点   | 上面两项，适用于和%greet%结合                     |

### 四则运算

数字与数字、字符串与字符串之间可进行比较。其中运算符支持 `加+`、`减-`、`乘*`、`除/`、`取模%`，比较支持 `大于>`、`小于<`、`等于=`、`不等于!=`、`大于等于>=`、`小于等于<=`。

tips：

-  开发人员友好，`=` 可写作 `==`
-  不等于 `!=` 也可以是 `<>`
- 字符串两端可不用加双引号`"`

比较的两边，当都是数字或算数表达式时，自动进行简单的计算。

### 逻辑运算

与编程语言相似的算法，每一行使用 `[]` 开头，则方括号中的内容会识别为`条件表达式`，使用 `,` 或 `&&` 来执行“与”逻辑，使用 `;` 或 `||` 来执行“或”逻辑；“或”的优先级更高。`[]` 中使用 `%val%` 作为变量值，例如 `[%level%>10]弹幕内容`，则只有当用户等级超过10级时才会被发送`弹幕内容`。

示例：

- 用户等级为0级：`[%level%=0]` 或 `[%level% == 0]`
- 粉丝牌等级介于10级到19级之间：`[%medal_level%>=10, %medal_level%<20]`
- 名字为某某某：`[%uname%=某某某]` 或 `["%nickname%"=="某某某"]`
- 现在是黑夜：`[%time_hour% > 17 || %time_hour% <= 6]`
- 带粉丝牌的0级号，或非0级号：`[%level%]`
- 付费两万（2千万金瓜子）的老板：`[%total_gold% >= 20000000]`
- 一小时内重新进入直播间：`[%come_time% > %timestamp%-3600]`
- 几周没来：`[%come_time%>%timestamp%-3600*24*30 &&  %come_time%<%timestamp%-3600*24*7]`

### 优先级覆盖

文本框中内容支持多行，一行为一条候选项，随机发送一条。其中每一条都可以用星号 `*` 开头（若有条件表达式 `[exp]`，则`*`位于表达式后面），星号数量越多，则优先级越高。高优先级候选应当带有条件，当满足条件时，发送该条弹幕，并无视掉所有更低优先级的候选。

带有优先级的候选项，会被更高优先级（更多星号）的候选项所覆盖。

例如：

```
感谢 %ai_name% 的%gift_name%，么么哒~
[%gift_gold%>=10000]感谢 %ai_name% 的%gift_name%，老板大气！
[%gift_gold%>=50000]*哇塞！感谢 %ai_name% 的%gift_name%！\n老板大气！
[%gift_gold%>=150000]**哇噢！感谢 %ai_name% 的%gift_name%！\n老板大气！！！！！！
```

注意星号，1、2无优先级，即使满足第2项的条件，也是随机发送前两项之一。3优先级超过1、2，因此当满足3的条件并且不满足4时，会发送第3项。而当4的条件满足时，其余更低优先级的候选项都被无视，只发送4。

- 当本次礼物的金瓜子总价少于1万时，发送第一项；
- 当本次礼物的金瓜子总价小于5万时，随机发送第一项、第二项；
- 当本次礼物金瓜子总价介于5万到15万时，发送第三项；
- 当本次礼物金瓜子总价超过15万时，发送第四项。

### 多条弹幕

使用 `\n` 来分割过长弹幕，则会分多条弹幕发送。

> 这是两个普通字符，并不是换行符。对于编程人员来说，相当于代码中两个反斜杠加字母n。

受于B站后台的限制，多条弹幕将调整为每隔1.5秒发送一次，数量无上限。



## 远程命令

主播或者机器人账号在直播间内发送如`关闭欢迎`、`开启录播`等命令，可远程控制在服务器上的机器人（本地没必要使用远程命令）。

- 关闭/开启功能：开关自动欢迎、送礼答谢、关注答谢、自动禁言
- 关闭/开启欢迎
- 关闭/开启关注答谢
- 关闭/开启送礼答谢
- 关闭/开启禁言：新人关键词自动禁言
- 关闭/开启偷塔
- 关闭/开启点歌
- 关闭/开启点歌回复
- 关闭/开启定时任务
- 关闭/开启自动连接
- 关闭机器人：全局关闭（将无法远程唤醒）



## 附：弹幕CMD列表

根据字节流中的`protocol`，等于 `2` 时body部分需要用`zlib.uncompress`解压，常见CMD如下：

```C++
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

