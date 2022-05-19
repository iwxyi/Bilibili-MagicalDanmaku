usedMsgType = ["QQ_MSG"];
configGroup = 'qq_danmu';

// 全局变量
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
    if (data['showHeader'] != undefined) {
        showHeader = data['showHeader'];
    }
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
}

function parseCmd(cmd, data) {
    switch (cmd) {
        case 'QQ_MSG':
            parseMsg(data);
            break;
    }
}
