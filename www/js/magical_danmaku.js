var usedMsgType = []; // !需要使用的cmd类型
var configGroup = ''; // !留空表示不需要读取配置

var appWs;
var reconnectTimer = null;
var isReconnecting = false;
var reconnectInterval = 3000; // 3秒重连间隔

function connectWebSocket() {
    if (isReconnecting) return;
    
    try {
        appWs = new WebSocket("ws://__DOMAIN__:__WS_PORT__");
        
        appWs.onopen = function () {
            console.log('WebSocket连接成功');
            isReconnecting = false;
            if (reconnectTimer) {
                clearTimeout(reconnectTimer);
                reconnectTimer = null;
            }
            
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
        
        appWs.onclose = function (e) {
            console.log('WebSocket连接关闭，准备重连...');
            scheduleReconnect();
        };
        
        appWs.onerror = function (e) {
            console.log('WebSocket连接错误:', e);
            if (appWs.readyState === WebSocket.CONNECTING || appWs.readyState === WebSocket.OPEN) {
                appWs.close();
            }
        };
        
    } catch (err) {
        console.log('创建WebSocket连接失败:', err);
        scheduleReconnect();
    }
}

function scheduleReconnect() {
    if (isReconnecting || reconnectTimer) return;
    
    isReconnecting = true;
    console.log('将在3秒后尝试重连...');
    
    reconnectTimer = setTimeout(function() {
        console.log('开始重连...');
        isReconnecting = false;
        reconnectTimer = null;
        connectWebSocket();
    }, reconnectInterval);
}

$(document).ready(function () {
    connectWebSocket();
});

function saveConfig(key, value) {
    console.log('setConfig: ' + key + '=' + value);
    appWs.send('{"cmd": "SET_CONFIG", "group" : "' + configGroup + '", "data": {"' + key + '": ' + value + '}}');
}