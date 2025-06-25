/*
* 一个弹幕显示的工具：
* 要求可以根据用户的设置，如弹幕数量、超时时长等来动态配置；可以自由适配各种窗口尺寸。测试弹幕与实际弹幕同理
* 所有弹幕靠左对齐，头像呈一列；弹幕气泡宽度由弹幕内容决定，自适应；昵称文字为白色黑底，在各种场景都能看清；粉丝牌为彩色边框的圆角矩形，里面字体稍小；舰长图标为函数getGuardIcon的返回值
* 每条弹幕有两种模式：当"显示昵称"关闭时，仅显示头像和弹幕气泡，两者水平居中对齐；当"显示昵称"开启时，左边为头像，右边为昵称行（粉丝牌（如果有）+昵称+舰长图标（如果有））+弹幕气泡，其中头像与右边的昵称行和弹幕气泡整体居中对齐
* 弹幕靠左下角显示，新弹幕在旧弹幕下面，旧弹幕有超出数量或定时消失。
* 有包含层次感的入场动画和出场动画，其余弹幕往上移动的时候也要有非线性动画，不能突然瞬移
* 相同UID的连续弹幕，后续弹幕不显示头像和昵称行（因为是相同数据）；但是当上面的旧弹幕消失时，下面连续UID的新的第一条弹幕要显示回头像和昵称行（因为是重复元素，有没有可能类似flutter的hero动画？）
*/

/// 用户配置
var msgCount = 3; // 最大同时显示3条弹幕
var showTime = 10; // 弹幕最大持续时间：30秒
var showUname = false; // 显示用户昵称
var showBubble = true; // 显示气泡的背景
var showMedal = false; // 显示粉丝牌（总开关）
var onlyMyMedal = false; // 仅显示自己的粉丝牌
var showGuard = true; // 显示舰长图标
// 智能弹幕
var hideViolationDanmu = true; // 隐藏违规弹幕
var hideNotImportantDanmu = false; // 隐藏不重要的弹幕
var smartStyleByType = false; // 智能样式

/// 程序配置
usedMsgType = ["DANMU_MSG", "GPT_RESPONSE"];
configGroup = 'danmu';

var currentRoomId = '0';

// 全局变量
var danmakuList = []; // 存储当前显示的弹幕
var lastUid = null; // 记录上一条弹幕的UID
var lastDanmuTime = {}; // 记录每个UID最后发送弹幕的时间

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
    if (data['showGuard'] != undefined) {
        showGuard = data['showGuard'];
    }
    if (data['hideViolationDanmu'] != undefined) {
        hideViolationDanmu = data['hideViolationDanmu'];
    }
    if (data['hideNotImportantDanmu'] != undefined) {
        hideNotImportantDanmu = data['hideNotImportantDanmu'];
    }

    // 初始化一些测试弹幕
    addTestDanmaku();
}

function parseCmd(cmd, data) {
    // 当"智能弹幕"至少有一个打开时，才使用"GPT_RESPONSE"
    // 否则直接使用"DANMU_MSG"
    if (hideViolationDanmu || hideNotImportantDanmu || smartStyleByType) {
        switch (cmd) {
            case 'GPT_RESPONSE':
                addDanmaku(data);
                break;
        }
    } else {
        switch (cmd) {
            case 'DANMU_MSG':
                addDanmaku(data);
                break;
        }
    }
}

function getGuardIcon(guard_level) {
    return "http://i0.hdslb.com/bfs/activity-plat/static/20200716/1d0c5a1b042efb59f46d4ba1286c6727/icon-guard" + guard_level + ".png@44w_44h.webp";
}

/**
 * 添加一条弹幕
 */
function addDanmaku(data) {
    // 获取数据
    var nickname = data['nickname'];
    var text = data['text'];
    var uid = data['uid'];
    var guard_level = data['guard_level'];
    var medal_level = data['medal_level'];
    var medal_name = data['medal_name'];
    var anchor_roomid = data['anchor_roomid'];
    var time = new Date().getTime() / 1000;
    var face_url = data['face_url'];

    // 判断是否需要添加弹幕
    if (hideViolationDanmu && isViolationDanmu(text)) {
        return;
    }
    if (hideNotImportantDanmu && !isImportantDanmu(text)) {
        return;
    }

    // 创建弹幕元素
    var danmuItem = $('<li>').addClass('danmu-item');
    
    // 添加头像
    var avatar = $('<img>')
        .addClass('danmu-avatar')
        .attr('src', face_url)
        .attr('alt', nickname);
    
    // 创建右侧内容容器
    var contentDiv = $('<div>').addClass('danmu-content');
    
    // 根据设置决定是否显示用户信息
    if (showUname) {
        var userInfo = $('<div>').addClass('danmu-user-info');
        
        // 添加粉丝牌
        if (showMedal && medal_name && (!onlyMyMedal || anchor_roomid === currentRoomId)) {
            var medal = $('<span>')
                .addClass('danmu-medal')
                .css('border-color', getMedalColor(medal_level))
                .text(medal_name + ' ' + medal_level);
            userInfo.append(medal);
        }
        
        // 添加昵称
        var nicknameSpan = $('<span>')
            .addClass('danmu-nickname')
            .text(nickname);
        userInfo.append(nicknameSpan);
        
        // 添加舰长图标
        if (showGuard && guard_level > 0) {
            var guardIcon = $('<img>')
                .addClass('danmu-guard-icon')
                .attr('src', getGuardIcon(guard_level))
                .attr('alt', '舰长');
            userInfo.append(guardIcon);
        }
        
        contentDiv.append(userInfo);
    }
    
    // 添加弹幕气泡
    var bubble = $('<div>')
        .addClass('danmu-bubble')
        .text(text);
    contentDiv.append(bubble);
    
    // 组装弹幕元素
    if (showUname) {
        danmuItem.append(avatar).append(contentDiv);
    } else {
        // 不显示昵称时，头像和气泡水平居中对齐
        var centerWrapper = $('<div>')
            .css({
                'display': 'flex',
                'align-items': 'center',
                'justify-content': 'center'
            });
        centerWrapper.append(avatar).append(contentDiv);
        danmuItem.append(centerWrapper);
    }
    
    // 处理连续弹幕
    if (uid === lastUid && time - lastDanmuTime[uid] < 5) {
        // 连续弹幕不显示头像和用户信息，并添加continuous类
        danmuItem.addClass('continuous');
        danmuItem.find('.danmu-avatar, .danmu-user-info').hide();
    } else {
        // 更新最后发送时间
        lastDanmuTime[uid] = time;
    }
    
    // 更新最后发送的UID
    lastUid = uid;
    
    // 添加到DOM
    $('#danmu-list').append(danmuItem);
    
    // 添加到弹幕列表
    danmakuList.push({
        element: danmuItem,
        time: time,
        uid: uid
    });
    
    // 检查是否需要移除旧弹幕
    checkAndRemoveDanmaku();
    
    // 设置定时移除
    setTimeout(function() {
        removeDanmaku(danmuItem);
    }, showTime * 1000);
}

/**
 * 检查并移除超出数量的旧弹幕
 */
function checkAndRemoveDanmaku() {
    while (danmakuList.length > msgCount) {
        var oldestDanmu = danmakuList.shift();
        removeDanmaku(oldestDanmu.element);
    }
}

/**
 * 移除弹幕
 */
function removeDanmaku(element) {
    if (!element || !element.parent().length) return;
    
    element.addClass('removing');
    setTimeout(function() {
        element.remove();
        // 检查是否需要显示下一条连续弹幕的头像和用户信息
        var nextDanmu = element.next('.danmu-item');
        if (nextDanmu.length && nextDanmu.hasClass('continuous')) {
            nextDanmu.removeClass('continuous');
            nextDanmu.find('.danmu-avatar, .danmu-user-info').show();
        }
    }, 300); // 与CSS动画时长匹配
}

/**
 * 获取粉丝牌颜色
 */
function getMedalColor(level) {
    const colors = {
        1: '#6b9ac4',
        2: '#6b9ac4',
        3: '#6b9ac4',
        4: '#6b9ac4',
        5: '#6b9ac4',
        6: '#6b9ac4',
        7: '#6b9ac4',
        8: '#6b9ac4',
        9: '#6b9ac4',
        10: '#6b9ac4',
        11: '#6b9ac4',
        12: '#6b9ac4',
        13: '#6b9ac4',
        14: '#6b9ac4',
        15: '#6b9ac4',
        16: '#6b9ac4',
        17: '#6b9ac4',
        18: '#6b9ac4',
        19: '#6b9ac4',
        20: '#6b9ac4',
        21: '#6b9ac4',
        22: '#6b9ac4',
        23: '#6b9ac4',
        24: '#6b9ac4',
        25: '#6b9ac4',
        26: '#6b9ac4',
        27: '#6b9ac4',
        28: '#6b9ac4',
        29: '#6b9ac4',
        30: '#6b9ac4'
    };
    return colors[level] || '#6b9ac4';
}

/**
 * 判断是否为违规弹幕
 */
function isViolationDanmu(text) {
    // TODO: 实现违规弹幕检测逻辑
    return false;
}

/**
 * 判断是否为重要弹幕
 */
function isImportantDanmu(text) {
    // TODO: 实现重要弹幕检测逻辑
    return true;
}

/**
 * 添加测试弹幕
 */
function addTestDanmaku() {
    // 测试数据数组
    const testData = [
        {
            uid: "629184597",
            nickname: "神奇弹幕",
            text: "这是一条测试弹幕！",
            anchor_roomid: "629184597",
            guard_level: 3,  // 舰长
            medal_name: "懒一夕",
            medal_level: "1",
            face_url: "https://i1.hdslb.com/bfs/face/29183e0e21b60c01a95bb5c281566edb22af0f43.jpg"
        },
        {
            uid: "123456789",
            nickname: "测试用户1",
            text: "Hello World！",
            anchor_roomid: "629184597",
            guard_level: 2,  // 提督
            medal_name: "测试粉丝牌",
            medal_level: "20",
            face_url: "https://i1.hdslb.com/bfs/face/29183e0e21b60c01a95bb5c281566edb22af0f43.jpg"
        },
        {
            uid: "987654321",
            nickname: "测试用户2",
            text: "这是一条很长的测试弹幕，用来测试弹幕的换行效果！这是一条很长的测试弹幕，用来测试弹幕的换行效果！",
            anchor_roomid: "629184597",
            guard_level: 1,  // 总督
            medal_name: "高级粉丝",
            medal_level: "30",
            face_url: "https://i1.hdslb.com/bfs/face/29183e0e21b60c01a95bb5c281566edb22af0f43.jpg"
        },
        {
            uid: "456789123",
            nickname: "测试用户3",
            text: "舰长来了！",
            anchor_roomid: "629184597",
            guard_level: 0,  // 无舰长
            medal_name: "舰长粉丝",
            medal_level: "40",
            face_url: "https://i1.hdslb.com/bfs/face/29183e0e21b60c01a95bb5c281566edb22af0f43.jpg"
        }
    ];

    // 随机选择一条测试数据
    const randomData = testData[Math.floor(Math.random() * testData.length)];
    
    // 添加调试信息
    console.log("测试弹幕数据:", randomData);
    console.log("当前弹幕列表长度:", danmakuList.length);
    console.log("DOM中弹幕数量:", $("#danmu-list li").length);
    console.log("显示时间:", showTime);
    
    // 添加弹幕
    addDanmaku(randomData);
}
