var usedMsgType = []; // !需要使用的cmd类型
var configGroup = ''; // !留空表示不需要读取配置

var appWs;
$(document).ready(function () {
    appWs = new WebSocket("ws://__DOMAIN__:__WS_PORT__");
    appWs.onopen = function () {
        var json = { cmd: "cmds", data: usedMsgType };
        appWs.send(JSON.stringify(json));

        // 自动读取配置
        if (configGroup != '') {
            var json = { cmd: "GET_CONFIG", group: configGroup };
            appWs.send(JSON.stringify(json));
        }

        try {
            socketInited(); // !socket初始化完毕
        } catch (err) {
            // console.log(err);
        }
    };
    appWs.onmessage = function (e) {
        console.log(e.data);
        var json = JSON.parse(e.data);
        try {
            var cmd = json['cmd'];
            var data = json['data'];
            if (cmd == 'GET_CONFIG') {
                readConfig(data); // !读取配置
            } else if (cmd == 'GET_INFO') {
                readInfo(data); // !读取信息
            } else {
                parseCmd(cmd, data); // !解析自己的命令
            }
        } catch (err) {
            // console.log(err);
        }
    };
});

function saveConfig(key, value) {
    console.log('setConfig', key, value);
    appWs.send('{"cmd": "SET_CONFIG", "group" : "' + configGroup + '", "data": {"' + key + '": ' + value + '}}');
}