// 一些参数
var msgCount = 3; // 最大同时显示3条弹幕
var showTime = 10; // 弹幕最大持续时间：30秒
var showUname = false; // 显示用户昵称
var showBubble = true; // 显示气泡的背景
var showMedal = true; // 显示粉丝牌（总开关）
var onlyMyMedal = false; // 仅显示自己的粉丝牌
var showGuard = true;

usedMsgType = ["DANMU_MSG"];
configGroup = 'danmu';

// 全局变量
var removing = false;
var danmakuTimes = new Array();
var removeTimer = setInterval(function () {
    if (danmakuTimes.length == 0) {
        return;
    }
    var timestamp = new Date().getTime() / 1000;
    if (timestamp - danmakuTimes[0] >= showTime) {
        removeMsgHtml($("ul li:first"));
    }
}, 1000);
var currentRoomId = '0';

function socketInited() {
    var json = {
        cmd: "GET_INFO",
        data: {
            room_id: "%room_id%",
        }
    }
    appWs.send(JSON.stringify(json));
}

function readInfo(data) {
    currentRoomId = data['room_id'];
}

function readConfig(data) {
    if (data['showUname'] != undefined) {
        showUname = data['showUname'];
    }
    if (data['showBubble'] != undefined) {
        showBubble = data['showBubble'];
    }
    if (data['msgCount'] != undefined) {
        msgCount = data['msgCount'];
    }
    if (data['showTime'] != undefined) {
        showTime = data['showTime'];
    }
    if (data['showMedal'] != undefined) {
        showMedal = data['showMedal'];
    }
    if (data['onlyMyMedal'] != undefined) {
        onlyMyMedal = data['onlyMyMedal'];
    }

    // 初始化一些测试弹幕
    var timestamp = new Date().getTime() / 1000;
    addMsgHtml({ "uid": "629184597", "nickname": "神奇弹幕", "text": "准备就绪！", "anchor_roomid": "629184597", "guard_level": "21", "medal_name": "懒一夕", "medal_level": "1" }, timestamp);
}

function parseCmd(cmd, data) {
    switch (cmd) {
        case 'DANMU_MSG':
            parseDanmaku(data);
            break;
    }
}

function parseDanmaku(data) {
    const maxCount = msgCount;
    var len = $("#list li").length;
    // console.log("数量：", len);
    if (len >= maxCount) { // 超过了数量
        // 删除第一个元素
        removeMsgHtml($("ul li:first"));
    }
    
    var time = new Date().getTime() / 1000;

    // 添加弹幕气泡
    addMsgHtml(data);
}

/**
 * 添加弹幕HTML
 */
function addMsgHtml(data, timestamp) {
    // 获取数据
    var nickname = data['nickname'];
    var text = data['text'];
    var uid = data['uid'];
    var guard_level = data['guard_level'];
    var medal_level = data['medal_level'];
    var time = new Date().getTime();

    danmakuTimes.push(timestamp);

    var avatarClass = 'msg_line_avatar';
    var avatarImgClass = 'msg_line_avatar_img';
    var bubbleHtml = '<div class="msg_line_bubble">\
                            <div class="msg_line_bubble_arrow"></div>\
                            <div class="msg_line_bubble_content">\
                                <span class="msg_line_text">' + text + '</span>\
                            </div>\
                       </div>';
    if (!showBubble) {
        bubbleHtml = '<div class="msg_line_pure_text">\
                    <span class="msg_line_text">' + text + '</span>\
                </div>';
        avatarClass = 'msg_line_avatar_small';
        avatarImgClass = 'msg_line_avatar_img_small';
    }

    var unameHtml = '';
    if (showUname) {
        unameHtml = '<div class="msg_line_uname">\
            ' + nickname + '\
                </div>';
    }

    //榜单
    var rankHtml = "";
    if (showMedal && showGuard) {
        var rank_level = data["extra"]["info"][4][4];
        if (rank_level == 1) {
            rankHtml = '<i class="rank-icon rank-icon-1 v-middle"></i>'

        }
        else if (guard_level == 2) {
            guardHtml = '<i class="medal-guard" style="background-image: url(http://i0.hdslb.com/bfs/activity-plat/static/20200716/1d0c5a1b042efb59f46d4ba1286c6727/icon-guard2.png@44w_44h.webp)"></i>'
        }
        else if (guard_level == 3) {
            guardHtml = '<i class="medal-guard" style="background-image: url(http://i0.hdslb.com/bfs/activity-plat/static/20200716/1d0c5a1b042efb59f46d4ba1286c6727/icon-guard3.png@44w_44h.webp)"></i>'
        }
    }
    
    var anchor_roomid = data['anchor_roomid'];
    var guardHtml = "";
    // 舰长标识
    if (showGuard) {
        //舰长级别:  0无舰长,1总督,2提督,3舰长
        var guard_level = data['guard_level'];
        if (guard_level == 1) {
            guardHtml = '<i class="medal-guard" style="background-image: url(http://i0.hdslb.com/bfs/activity-plat/static/20200716/1d0c5a1b042efb59f46d4ba1286c6727/icon-guard1.png@44w_44h.webp)"></i>'
        }
        else if (guard_level == 2) {
            guardHtml = '<i class="medal-guard" style="background-image: url(http://i0.hdslb.com/bfs/activity-plat/static/20200716/1d0c5a1b042efb59f46d4ba1286c6727/icon-guard2.png@44w_44h.webp)"></i>'
        }
        else if (guard_level == 3) {
            guardHtml = '<i class="medal-guard" style="background-image: url(http://i0.hdslb.com/bfs/activity-plat/static/20200716/1d0c5a1b042efb59f46d4ba1286c6727/icon-guard3.png@44w_44h.webp)"></i>'
        }
    }

    // 粉丝勋章
    var medalHtml = "";
    if (showMedal && data['medal_name'] != undefined) {
        //如果佩戴了粉丝牌  后面数值为当前房间的ID
        if (!onlyMyMedal || anchor_roomid == currentRoomId) {
            var medal_name = data['medal_name'];
            var medal_level = data['medal_level'];
            medalHtml = '<div class="fans-medal-item">\
					<div class="fans-medal-label">\
						<span class="fans-medal-content">' + medal_name + '</span>\
					</div>\
					<div class="fans-medal-level">' + medal_level + '</div>\
				</div>';
        }
    }

    var msgHtml = '<li>\
        <div class="msg_line">\
            <div class="'+ avatarClass + '">\
                <img src="http://__DOMAIN__:__PORT__/api/header?uid=' + uid + '"\
                    class="header ' + avatarImgClass + '">\
            </div>'
        + rankHtml
        + guardHtml
        + medalHtml
        + unameHtml
        + bubbleHtml +
        '</div>\
        </li>';
    $("#list").append(msgHtml);
}

function removeMsgHtml(msg) {
    if (removing)
        return;
    removing = true;
    msg.slideUp(500, function () {
        removing = false;
        msg.remove();
        danmakuTimes.shift();
    });
}