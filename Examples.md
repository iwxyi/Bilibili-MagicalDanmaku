[toc]

# 说明

这里是一些可用的示例，按需要复制至对应位置。



# 欢迎进入

## 示例

### 招呼欢迎

```
%ai_name%，%greet%%tone/punc%
```
将随机发送：

- 某某某，早上好啊~
- 某某某，晚饭吃了吗？
- 某某某，通宵了吗~



### 欢迎舰长

```
[%guard%]*欢迎舰长 %ai_name% 回家~
```



### 欢迎串门

```
[%pk_opposite%]**欢迎%ai_name%串门哦~
```

中间的`*`表示优先级，数量多的优先级更高，会覆盖数量少的

但是当超过“单条弹幕最大字数”时，超出字数的都将被忽略，哪怕优先级更高



### 欢迎串门回来

```
[%pk_view_return%]**欢迎%ai_name%串门回来~
```



## 高级示例

### 只欢迎戴自己勋章

这里采用的是反向操作：不自动欢迎没带直播间勋章的用户

```
[%guard% = 0, %anchor_roomid%!=%room_id%, !%strong_notify%]*
```

> v3.5.6及之前有个bug：取反符号`!`前面不能有空格，旧版需去掉空格



### 一天没来

```
[%come_time%>%timestamp%-3600*24*2, %come_time%<%timestamp%-3600*24, 0]*%ai_name%，昨天没来哦~
```

> 上次进入时间最大保留七天





# 送礼答谢

## 示例

### 普通的答谢

```
谢谢 %ai_name% 的%gift_name%~
```



### 小礼物不答谢

1000以下的银瓜子礼物且数量小于10，则忽略。例如忽略6个小心心，但24个小心心则感谢

```
[%gift_gold%=0, %gift_silver%<1000, %gift_num% < 10]**
```



### 大礼物特殊答谢

**超过80元**的礼物

```
[%gift_gold%>=80000]*哇噢！感谢 %ai_name% 的%gift_name%！\n老板大气！！！！！！
```

`\n`表示换行，将分作两条弹幕发送（可以配合`<h1>`放大字体，仅部分弹幕插件有效）



### 跳过机器人送的吃瓜

```
[%uid%=%my_uid%,%origin_gift_name%=吃瓜]**
```



### 指定礼物专属答谢

```
[%gift_name%=小电视飞船]哇！！！谢谢%ai_name%带我上太空~
```



## 高级示例

### 显示礼物价值

```
谢谢%ai_name%赠送了价值%[%gift_gold%/1000]%元的%gift_name%！
```



### 上船自动私信

**答谢 — 感谢送礼**中添加：

```
[%guard_buy%,%guard_first%=1]感谢%ai_name%开通%gift_name%！\n>sendPrivateMsg(%uid%, 感谢开通大航海，可加入粉丝群：xxx)
```

`%guard_buy%` 判断是否是购买舰长的通知；`%guard_first%` 判断是否第一次上船，第一次=1，续船=0，掉船后重新上传=2。

也可用 `%guard_count%` 来读取上船的次数作为条件，`%guard_count%=0`表示第一次上船。

> `%guard_count%` 只计算本程序时运行时购买舰长的用户，每次舰长+1，提督+10，总督+100。



### 送礼优先点歌

**答谢-感谢送礼**中添加：

```
[%gift_name%=喵娘]>improveSongOrder(%username%,5)
```

赠送一个喵娘则提前5首歌播放。可将`5`改为`999`表示无限大，或者用 `%gift_gold% / 1000` 表示每1000金瓜子礼物可提前一首歌，如下：

```
[%gift_name%=喵娘]>improveSongOrder(%username%,%[%gift_gold%/1000]%)
```





# 定时任务

### 示例：今日是否有大航海

```
[%today_guard%=0]今天XX等到新的舰长了吗？\n老板们救救可怜的XX吧~
[%today_guard%>0]今天XX等到了新的舰长，还有老板想上船嘛
```

> 今天的相关数据都需要在程序连接房间时才有效



### 示例：定时联网

```
>connectNet(要发送数据的网址)
```



### 示例：自动打卡

添加定时任务，设置时间为 `86400` （一天秒数）

添加发送的文本：

```
>sendRoomMsg([直播间房号], 打卡)
```

注意：需要**关闭“仅直播时发送”**，或加上条件`[%living%+1]`（未开播也执行）





# 自动回复

### 示例：关键词自动禁言

> 有专门的禁言选项卡，此示例仅仅是展示使用方式

添加需要禁言关键词`xxx`，支持正则表达式。设置回复：

```
>block(%uid%)
```



### 示例：房管远程弹幕禁言

默认仅主播与机器人账号支持弹幕禁言，此方法可提升至房管（或其他特定条件用户）：

添加回复：`^((禁言|解禁|解除禁言|取消禁言) .+|撤销禁言)$`，添加动作：

```
[%admin%]>execRemoteCommand(%text%)
```



### 示例：不要欢迎我

添加表达式：`别这么热情`，添加回复：

```
>ignoreWelcome(%uid%)\n已关闭您的自动欢迎
```

只要弹幕中包含“别这么热情”，则以后都不会自动欢迎



### 示例：弹幕切歌

房管可以切所有歌，普通观众只能切自己点的歌

添加**表达式**：`^切歌$`，设置动作：

```
[%admin%]*>cutOrderSong()
>cutOrderSong(%username%)
```



### 示例：远程开关AI弹幕回复

默认仅机器人和主播可远程控制，而其他人不行。通过此方法设置特定用户（例如舰长、房管）开关。

添加关键词表达式：`^开启AI回复$`

添加回复：

```
>execRemoteCommand(开启弹幕回复,0)\n>setValue(reply, 1)\n已开启AI弹幕回复，发送“关闭”结束
```

添加关键词表达式：`^关闭(AI回复)?$`（回复“关闭”两字即可，可不用全打）

添加回复：

```
[%guard%, %{reply}%]>execRemoteCommand(关闭弹幕回复)\n>setValue(reply, 0)
```

逐一说明：

- `>execRemoteCommand`：运行仅机器人和主播才能用的远程指令，参数2的`0`表示不自动回复（因为有自定义的另一个回复，覆盖掉默认的）
- `\n`：多个指令或多条弹幕，用 `\n` 标记隔开
- `setValue(reply, 1)`：设置变量`reply`的值为1，用来判断是否需要关闭
- `[%guard%, %{reply}%]`：舰长且`reply`=1，若是则执行后面的，否则不做操作







## 高级示例

### 示例：设置自己专属昵称

添加自动回复表达式：`^请?叫我(.*)$`，添加回复：

```
>setNickname(%uid%, %$1%)\n设置您的专属昵称为：%$1%
```

当用户发送弹幕：“叫我小明”或“请叫我小华”，程序自动设置其专属昵称为“小明”。



### 示例：修改指定用户昵称

添加自动回复表达式：`叫(\S+)\s+(.*)`，添加回复：

```
>setNickname(%(%$1%)%, %$2%)\n修改专属昵称成功
```





# 事件动作

## 弹幕事件

### 示例：QQ群推送开播消息

以酷推为例：[https://cp.xuthus.cc](https://cp.xuthus.cc)，按其说明配置

添加事件：`LIVE`，动作：

```
>connectNet(https://push.xuthus.cc/group/[skey]?c=[开播消息])
```

其中`[skey]`为您的酷推Skey，`[开播消息]`按服务格式自定义

一切配置妥当，开播时将会自动发送消息至QQ群



示例：远程禁言回复

`禁言 xxx`通过倒找弹幕发送人昵称的方法，通过弹幕禁言用户。其中所有属性同`DANMU_MSG`，例如`%uid%`、`%uname%`等。如果禁言对象是房管，那么将会禁言失败。

添加事件`REMOTE_BLOCK`，添加动作：

```
[%uid%=%my_uid%]**>因为太帅无法被禁言
[%uid%=%up_uid%]**>已禁言主播（狗头保命）
[%admin%]*>无法禁言房管
>已禁言：%uname%
```

**注意：添加本事件，将会屏蔽系统自带的禁言回复**



### 示例：下播自动关机

下播30秒后电脑自动关机

添加下播事件：`PREPARE`，动作：

```
>timerShot(30000, >runCommandLine(shutdown -s -t 30))
```

添加开播事件：`LIVE`，动作：

```
>runCommandLine(shutdown -a)
```

多了延时30秒和响应开播事件，是排除主播意外掉线的情况。



### 示例：感谢分享直播间

添加分享事件：`SHARE`

添加弹幕动作：

```
感谢%ai_name%分享%upname%的直播间！
```

其中`%upname%`可以使用菜单中的“自定义变量”来统一设置，也可以固定写死。



### 示例：上船声音提示

添加事件：`GUARD_BUY`，动作：

```
>playSound(%app_path%/audios/guard.mp3)
```

有人上船则自动播放`安装目录/audios/guard.mp3`，本程序**不自带**，需要自己找音频文件放上去。也可以是安装目录之外的绝对路径。



## 点歌姬事件

### 示例：点歌提示未带勋章

添加事件：`ORDER_SONG_NO_MEDAL`，动作：

```
(cd35:600)请戴粉丝牌点歌
```

加了**冷却通道**，最多十分钟提醒一次。



## 大乱斗事件

### 示例：大乱斗结束前提醒

大乱斗结束前30秒提醒：一次大乱斗为5分钟，开始后4分半发送弹幕，270秒=270000毫秒。

添加事件：`PK_BATTLE_START`，动作：

```
>timerShot(270000, 离大乱斗结束还有30秒)
```



### 示例：大乱斗蹭积分卡

添加事件：`PK_BATTLE_PRE`，动作：

```
>setValue(pk_ceng, 0)
```

添加事件：`SEND_GIFT`，动作：

```
[%pking%, %{pk_ceng}%=0, %gift_gold% >= 100000, %gift_num%=1]>setValue(pk_ceng, 1)\n>sendGift(20004, 1)
```

添加事件：`PK_BATTLE_END`，动作：

```
[%{pk_ceng}%=1]>setValue(pk_ceng, 1)\n>postData(https://api.live.bilibili.com/xlive/lottery-interface/v2/pk/join, id=%pk_id%&roomid=%room_id%&type=pk&csrf_token=%csrf%&csrf=%csrf%&visit_id=)
```



### 示例：大乱斗最佳助攻

添加事件：`PK_BEST_UNAME`，动作：

```
[%level%>0, %gift_coin% >= 100]感谢本场最佳助攻：%uname%
```

仅当赢了，并且本次累计送礼有超过100积分（10000金瓜子）才感谢



### 示例：大乱斗尊严票

对面超过100积分（1万金瓜子）而自己还是0积分的时候，送一个吃瓜保尊严 ╮(╯﹏╰）╭。

添加大乱斗即将结束事件：`PK_FINAL`

此事件的时间为大乱斗结束前几秒，具体值由偷塔提前量决定。

添加动作：

```
[%pk_my_votes%=0, %pk_match_votes%>100]>sendGift(20004, 1)
```

`20004` 为吃瓜的礼物ID，1为数量



### 示例：自动开启大乱斗

每次结束后5秒，自动重新开始大乱斗的匹配。

事件：`PK_BATTLE_SETTLE`，动作：

```
>timerShot(5000, >joinBattle(1))
```

其中`1`为普通大乱斗，`2`为视频大乱斗。

第一次大乱斗需手动点“发送”开启。



# 混合

### 示例：禁言小游戏

扣1禁言、关注主播或赠送小心心解除禁言（需要房管或主播）

**回复**中添加一栏，**关键词**为 `^1$`，**回复**：

```
已自动禁言，赠送小心心或关注主播解禁\n>block(%uid%, 1)\n>addGameUser(%uid%)
```

这里执行了三个操作：

1. 回复
2. 禁言
3. 添加到游戏用户，等待解除禁言

**答谢 — 感谢送礼**中添加：

```
[%in_game_users%,%origin_gift_name%=小心心]***已解除禁言\n>unblock(%uid%)\n>removeGameUser(%uid%)
```

> 其中 `[...]***` 中的星号为优先级，保证数量超过其余弹幕即可。
>
> `%in_game_users%` 确保是扣 `1` 被禁言（因为禁言的时候添加到了游戏用户）的，而非被手动禁言的用户

**答谢 — 感谢关注**中添加：

```
[%in_game_users%]*已解除禁言\n>unblock(%uid%)\n>removeGameUser(%uid%)
```

> 实测已关注的，先取关再马上重新关注，收不到通知，需要等会儿再关注

在两个答谢中，执行了三个操作：

1. 回复
2. 解除禁言
3. 从游戏用户中移除，后续小心心不再触发该游戏，而是普通的答谢



# 服务端开发

### 示例：远程计数

例如送指定礼物做10个蹲起，这时候需要显示蹲起的数量。

添加新连接事件：`WEBSOCKET_CMDS`，动作：

```
[%text%=SQUAT]>sendToSockets(SQUAT, {"cmd":"SQUAT", "data":%{squat}%})
```

添加`初始化蹲起数量`的动作（事件可空，手动发送）：

```
>setValue(squat, 0)\n>sendToSockets(SQUAT, {"cmd":"SQUAT", "data":0})
```

添加`蹲起数量+1`的动作（事情可空，手动发送）：

```
>setValue(squat, %[%{squat}%+1]%)\n>sendToSockets(SQUAT, {"cmd":"SQUAT", "data":%[%{squat}%+1]%})
```

创建`www/squat.html`文件，写入：

```html
<head>
    <title>神奇弹幕蹲起</title>
    <script src="js/jquery-2.1.0.js"></script>

    <style type="text/css">
        .card {
            background-color: #fff;
            box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.2);
            transition: 0.3s;
            /* width: 40%; */
            border-radius: 5px;
            padding-left: 20px;
            padding-right: 20px;
        }

        .card:hover {
            box-shadow: 0 8px 16px 0 rgba(0, 0, 0, 0.2);
        }
    </style>
</head>

<body>
    <div class="card" style="display:inline-block">
        <h1 id='count'>蹲起个数：0</h1>
    </div>

    <script type="text/javascript">
        $(document).ready(function () {
            var ws = new WebSocket("ws://__DOMAIN__:__WS_PORT__");
            ws.onopen = function () {
                ws.send('{"cmd":"cmds", "data":["SQUAT"]}');
            };
            ws.onmessage = function (e) {
                var json = JSON.parse(e.data);
                var cmd = json['cmd'];
                switch (cmd) {
                    case 'SQUAT':
                        console.log(json);
                        var count = parseInt(json['data']);
                        $("#count").html('蹲起个数：' + count);
                        break;
                }
            };
        });
    </script>
</body>
```

访问`localhost:5520/squat.html`（端口域名按照设定的来），显示`蹲起数量：0`。

每次点事件中“蹲起数量+1”那一项的“**发送**”按钮，对应蹲起数量加一。



# 冷却通道

### 示例：强制欢迎

每30秒欢迎一次用户；而若有25级及以上粉丝牌的用户进来，立刻欢迎，除非连续两个25级及以上的用户在5秒内进来，则只欢迎前一人；若是舰长，同上，多条冷却通道之间互不影响。而25级舰长，根据舰长优先级`**`超过25级优先级`*`，会优先发送欢迎舰长的弹幕。

```
(cd10:30)欢迎%ai_name%~
[%medal_level%>=25]*(cd11:5)欢迎%ai_name%，请多多关照~
[%guard%]**(cd12:5)欢迎舰长%ai_name%回家！
```

内置100个冷却通道，其中0~9已被系统使用，用户自定义建议为10~99，应该够用了。

>  注意：B站连续发送弹幕的冷却时间为1秒，与本程序的弹幕冷却系统无关。





# 弹幕样式

支持自定义实时弹幕的CSS样式，实时弹幕的右键菜单中，设置->标签样式。

**圆角矩形**

```css
background: white;
padding:5px;
border-radius: 10px;
```

**气泡图片**

```css
padding:10px;
border-image: url(:/bubbles/bubble1)
```

其中图片可使用本地绝对路径，例如：

```css
border-image: url(C:/Path/To/Image.png)
```

**类型选择**

支持按不同类型设置不同的样式，样式如下：

- msg：一些提示
- danmaku：弹幕（样式不影响左边头像显示）
- gift：送礼
- welcome：进入
- order-song：点歌提示文字：[点歌] 歌名
- guard-buy：开通/续费舰长
- welcome-guard：舰长进入
- attention：关注
- block：禁言
- share：分享直播间

以下是一个示例：

弹幕为默认气泡；礼物、上舰为圆角矩形卡片

```css
#danmaku {
    border-image: url(:/bubbles/bubble1);
    padding: 10px;
}
#gift, #guard-buy {
    background: #FFDAB9;
	padding: 5px;
    border-radius: 10px; 
}
```

