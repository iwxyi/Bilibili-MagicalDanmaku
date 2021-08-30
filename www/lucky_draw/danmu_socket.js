var ws;
var currentUid = 0;
var currentUname = '';
$(document).ready(function () {
    ws = new WebSocket("ws://__DOMAIN__:__WS_PORT__");
    ws.onopen = function () {
        ws.send('{"cmd": "cmds", "data": ["LUCKY_DRAW"]}');
    };
    ws.onmessage = function (e) {
        console.log(e.data);
        var json = JSON.parse(e.data);
        var cmd = json['cmd'];
        switch (cmd) {
            case 'START': // 开始抽奖
                var uid = json['uid'];
                var uname = json['uname'];
                start();
                break;
        }
    };
});

/**
 * @param index 从 1 开始，1~9
 */
function sendResult(giftIndex, giftName) {
    console.log('抽到：' + giftName, giftIndex);
    ws.send('{"cmd": "LUCKY_DRAW_RESULT", "data": {"uid": ' + currentUid +
        ', "uname": "' + currentUname +
        '", "giftIndex": ' + giftIndex +
        ', "giftName": "' + giftName + '"}}');
}