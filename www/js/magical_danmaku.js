var usedMsgType = []; // !需要使用的弹幕
var configGroup = ''; // !不留空表示需要读取配置

var appWs;
$(document).ready(function () {
    appWs = new WebSocket("ws://__DOMAIN__:__WS_PORT__");
    appWs.onopen = function () {
        var json = { cmd: "cmds", data: usedMsgType };
        appWs.send(JSON.stringify(json));

        // 读取配置
        if (configGroup != '') {
            appWs.send('{"cmd": "GET_CONFIG", "group": "' + configGroup + '"}');
        }

        try {
            socketInited(); // !socket初始化完毕
        } catch (err) {
            // console.log(err);
        }
    };
    appWs.onmessage = function (e) {
        // console.log(e.data);
        var json = JSON.parse(e.data);
        try {
            var cmd = json['cmd'];
            if (cmd == 'GET_CONFIG') {
                readConfig(json['data']); // !读取配置
            } else {
                parseCmd(cmd, json); // !解析弹幕
            }
        } catch (err) {
            // console.log(err);
        }
    };
});

function saveConfig(key, value) {
    console.log(key, value);
    appWs.send('{"cmd": "SET_CONFIG", "group" : "' + configGroup + '", "data": {"' + key + '": ' + value + '}}');
}