// 一些参数
var msgCount = 3; // 最大同时显示3条弹幕
var showTime = 10; // 弹幕最大持续时间：30秒

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

function socketInited() {
    // 初始化一些弹幕
    var timestamp = new Date().getTime() / 1000;
    addMsgHtml(629184597, '神奇弹幕准备就绪！', timestamp);
}

function readConfig(data) {
    if (typeof (data['msgCount']) != undefined) {
        msgCount = data['msgCount'];
    }
    if (typeof (data['showTime']) != undefined) {
        showTime = data['showTime'];
    }
}

function parseCmd(cmd, json) {
    switch (cmd) {
        case 'DANMU_MSG':
            parseDanmaku(json['data']);
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

    // 添加弹幕气泡
    addMsgHtml(uid, text);
}

/**
 * 添加弹幕HTML
 */
function addMsgHtml(uid, text, timestamp) {
    danmakuTimes.push(timestamp);
    var bubbleHtml = '<li>\
            <div class="msg_line">\
                <div class="msg_line_avatar">\
                    <img src="http://__DOMAIN__:__PORT__/api/header?uid=' + uid + '"\
                        class="header msg_line_avatar_img">\
                </div>\
                <div class="msg_line_bubble">\
                    <div class="msg_line_bubble_arrow"></div>\
                    <div class="msg_line_bubble_content">\
                        <span>' + text + '</span>\
                    </div>\
                </div>\
            </div>\
        </li>';
    $("#list").append(bubbleHtml);
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