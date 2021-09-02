// 一些参数
var msgCount = 3; // 最大同时显示3条弹幕
var showTime = 10; // 弹幕最大持续时间：30秒
var showUname = false; // 显示用户昵称
var showBubble = true; // 显示气泡的背景

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

function readConfig(data) {
    if (typeof (data['showUname']) != undefined) {
        showUname = data['showUname'];
    }
    if (typeof (data['showBubble']) != undefined) {
        showBubble = data['showBubble'];
    }
    if (typeof (data['msgCount']) != undefined) {
        msgCount = data['msgCount'];
    }
    if (typeof (data['showTime']) != undefined) {
        showTime = data['showTime'];
    }

    // 初始化一些测试弹幕
    var timestamp = new Date().getTime() / 1000;
    addMsgHtml(629184597, '神奇弹幕', '准备就绪！', timestamp);
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
        // $("ul li:first").remove();
        removeMsgHtml($("ul li:first"));
    }

    // 获取数据
    var nickname = data['nickname'];
    var text = data['text'];
    var uid = data['uid'];
    var guard_level = data['guard_level'];
    var medal_level = data['medal_level'];
    var time = new Date().getTime();

    // 添加弹幕气泡
    addMsgHtml(uid, nickname, text, time);
}

/**
 * 添加弹幕HTML
 */
function addMsgHtml(uid, uname, text, timestamp) {
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
            ' + uname + '\
                </div>';
    }

    var msgHtml = '<li>\
        <div class="msg_line">\
            <div class="'+ avatarClass + '">\
                <img src="http://__DOMAIN__:__PORT__/api/header?uid=' + uid + '"\
                    class="header ' + avatarImgClass + '">\
            </div>'
        + unameHtml
        + bubbleHtml +
        '</div>\
        </li>';
    $("#list").append(msgHtml);
    /* $("ul li:last").click(function(){
        // 仅供测试，会导致时序混乱
        removeMsgHtml($(this));
    }); */
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