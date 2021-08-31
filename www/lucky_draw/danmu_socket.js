// 用户信息
function UserInfo(id, name) {
    this.uid = id;
    this.uname = name;
}
var userQueue = new Array();
var currentUser = null;

// 奖品配置
function AwardInfo(name, prop) {
    this.name = name;
    this.prop = prop;
}
var awardList = new Array();
var awardPropSum = 0;
var awardPropCum = new Array();

// WebSocket
var ws;
$(document).ready(function () {
    ws = new WebSocket("ws://__DOMAIN__:__WS_PORT__");
    ws.onopen = function () {
        ws.send('{"cmd": "cmds", "data": ["LUCKY_DRAW"]}');
        ws.send('{"cmd": "GET_CONFIG", "data": ["lucky_draw"]}');
    };
    ws.onmessage = function (e) {
        console.log(e.data);
        var json = JSON.parse(e.data);
        var cmd = json['cmd'];
        switch (cmd) {
            case 'GET_CONFIG': // 开始抽奖
                var s = json['data']['lucky_draw'];
                var lines = s.split('\n');
                var maxIndex = Math.min(lines.length, 8);
                for (var i = 0; i < maxIndex; i++) {
                    var re = /^(.+?) (\d+)$/;
                    var r = '';
                    if (r = re.exec(lines[i])) {
                        // console.log(r[1], r[2]);
                        var prop = Number(r[2]);
                        awardList.push(new AwardInfo(r[1], prop));
                        awardPropSum += prop;
                        awardPropCum.push(awardPropSum);

                        // 插入空白礼物4
                        if (i == 3) {
                            awardList.push(new AwardInfo('抽不到的奖品', 0));
                            awardPropCum.push(awardPropSum);
                        }
                    }
                }

                // 设置显示文字
                for (var i = 0; i < awardList.length; i++) {
                    let el = names[i];
                    el.innerText = awardList[i].name;
                }
                break;
            case 'START': // 开始抽奖
                var uid = json['uid'];
                var uname = json['uname'];
                userQueue.push(new UserInfo(uid, uname));
                start();
                break;
        }
    };
});

/**
 * @param index 从 0 开始，0~8, 不应该有4
 */
function sendResult(giftIndex) {
    var giftName = awardList[giftIndex].name;
    console.log('抽到：' + giftName, giftIndex);
    ws.send('{"cmd": "LUCKY_DRAW_RESULT", "uid": ' + currentUser.uid +
        ', "uname": "' + currentUser.uname +
        '", "giftIndex": ' + giftIndex +
        ', "giftName": "' + giftName + '"}');
}

// 根据概率比例，获取可能的结果
function getRandomResult() {
    if (awardList.length == 0) // 全部随机
        return Math.floor(Math.random() * 8 + countMin);

    var r = Math.floor(Math.random() * awardPropSum);
    // console.log('random:', r, awardPropSum);
    for (var i = 0; i < awardPropCum.length; i++) {
        if (r < awardPropCum[i]) { // 就是这个i了
            var giftIndex = i; // 在九宫格中的位置
            // console.log('预计九宫格索引：' + giftIndex);

            // 获取需要的count
            for (var j = 0; j < indexArray.length; j++) {
                if (indexArray[j] == giftIndex) {
                    // 需要移动到j的位置
                    // console.log('预计数组位置：', j);
                    var arrayIndex = (currentArrayIndex + 7) % 8;
                    var needCount = (j + 8 - arrayIndex) % 8;
                    // console.log('currentArrayIndex:', arrayIndex, 'currentIndex', indexArray[arrayIndex], 'need:', needCount);
                    return 48 + needCount;
                }
            }
            return 48 - currentArrayIndex + i;
        }
    }
    console.log("错误的随机数：", r, awardPropSum);
}