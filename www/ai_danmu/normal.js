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
usedMsgType = ["DANMU_MSG"];
configGroup = 'danmu';

var currentRoomId = '0';

// 用于存储弹幕信息的数组
var danmakuList = [];
var removing = false;

// 在文件开头添加样式定义
var styleElement = document.createElement('style');
styleElement.textContent = `
    @keyframes avatarRotate {
        0% { transform: scale(0) rotate(-180deg); opacity: 0; }
        60% { transform: scale(1.2) rotate(10deg); opacity: 1; }
        100% { transform: scale(1) rotate(0); opacity: 1; }
    }

    @keyframes avatarImgFade {
        0% { transform: scale(0.8); opacity: 0; filter: blur(10px); }
        100% { transform: scale(1); opacity: 1; filter: blur(0); }
    }

    @keyframes infoRowSlide {
        0% { transform: translateX(-20px); opacity: 0; }
        100% { transform: translateX(0); opacity: 1; }
    }

    @keyframes bubblePop {
        0% { transform: scale(0.8); opacity: 0; }
        50% { transform: scale(1.1); }
        100% { transform: scale(1); opacity: 1; }
    }

    @keyframes medalSpin {
        0% { transform: scale(0) rotate(-90deg); opacity: 0; }
        60% { transform: scale(1.2) rotate(10deg); opacity: 1; }
        100% { transform: scale(1) rotate(0); opacity: 1; }
    }

    @keyframes unameSlide {
        0% { transform: translateY(-10px); opacity: 0; }
        100% { transform: translateY(0); opacity: 1; }
    }

    @keyframes guardPop {
        0% { transform: scale(0); opacity: 0; }
        50% { transform: scale(1.2); opacity: 1; }
        100% { transform: scale(1); opacity: 1; }
    }

    .danmaku-item {
        transition: transform 0.3s cubic-bezier(0.34, 1.56, 0.64, 1), opacity 0.3s ease-out;
        will-change: transform, opacity;
    }

    .danmaku-enter {
        opacity: 1 !important;
    }

    .avatar-animate {
        animation: avatarRotate 0.6s cubic-bezier(0.34, 1.56, 0.64, 1) forwards;
    }

    .avatar-img-animate {
        animation: avatarImgFade 0.5s ease-out 0.1s forwards;
    }

    .info-row-animate {
        animation: infoRowSlide 0.5s ease-out 0.2s forwards;
        opacity: 0;
    }

    .bubble-animate {
        animation: bubblePop 0.5s cubic-bezier(0.34, 1.56, 0.64, 1) 0.3s forwards;
        opacity: 0;
    }

    .medal-animate {
        animation: medalSpin 0.6s cubic-bezier(0.34, 1.56, 0.64, 1) 0.2s forwards;
        opacity: 0;
    }

    .uname-animate {
        animation: unameSlide 0.5s ease-out 0.3s forwards;
        opacity: 0;
    }

    .guard-animate {
        animation: guardPop 0.5s cubic-bezier(0.34, 1.56, 0.64, 1) 0.4s forwards;
        opacity: 0;
    }

    #danmu-list {
        position: relative;
        overflow-y: auto;
        overflow-x: hidden;
    }
`;
document.head.appendChild(styleElement);

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
    switch (cmd) {
        case 'DANMU_MSG':
            addDanmaku(data);
            break;
    }
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
    var anchor_roomid = data['anchor_roomid'];
    var time = new Date().getTime() / 1000;
    var danmakuId = 'danmaku_' + time + '_' + Math.random().toString(36).substr(2, 9);  // 生成唯一ID

    // 检查是否需要隐藏弹幕
    if (hideViolationDanmu && data['is_violation']) {
        return;
    }
    if (hideNotImportantDanmu && data['is_not_important']) {
        return;
    }

    // 检查弹幕数量限制
    if (danmakuList.length >= msgCount) {
        // 移除最早的弹幕
        var oldestDanmaku = danmakuList[0];
        removeMsgHtml($('#' + oldestDanmaku.id));
        danmakuList.shift();
    }

    // 构建弹幕HTML
    var avatarClass = showBubble ? 'msg_line_avatar' : 'msg_line_avatar_small';
    var avatarImgClass = showBubble ? 'msg_line_avatar_img' : 'msg_line_avatar_img_small';
    
    // 构建气泡或纯文本
    var contentHtml = showBubble ? 
        `<div class="msg_line_bubble">
            <div class="msg_line_bubble_arrow"></div>
            <div class="msg_line_bubble_content">
                <span class="msg_line_text">${text}</span>
            </div>
        </div>` :
        `<div class="msg_line_pure_text">
            <span class="msg_line_text">${text}</span>
        </div>`;

    // 构建用户昵称
    var unameHtml = showUname ? 
        `<div class="msg_line_uname">${nickname}</div>` : '';

    // 构建舰长标识
    var guardHtml = '';
    if (showGuard && guard_level > 0) {
        var guardClass = '';
        switch (parseInt(guard_level)) {
            case 1:  // 总督
                guardClass = 'icon-guard1';
                break;
            case 2:  // 提督
                guardClass = 'icon-guard2';
                break;
            case 3:  // 舰长
                guardClass = 'icon-guard3';
                break;
        }
        if (guardClass) {
            console.log("生成舰长图标:", guardClass);
            guardHtml = `<i class="medal-guard" style="background-image: url('http://i0.hdslb.com/bfs/activity-plat/static/20200716/1d0c5a1b042efb59f46d4ba1286c6727/${guardClass}.png@44w_44h.webp')"></i>`;
        }
    }

    // 构建粉丝勋章
    var medalHtml = '';
    if (showMedal && data['medal_name'] && (!onlyMyMedal || anchor_roomid == currentRoomId)) {
        medalHtml = `
            <div class="fans-medal-item">
                <div class="fans-medal-label">
                    <span class="fans-medal-content">${data['medal_name']}</span>
                </div>
                <div class="fans-medal-level">${medal_level}</div>
            </div>`;
    }

    // 组合完整的弹幕HTML
    var msgHtml = '';
    if (showUname) {
        // 显示昵称模式：头像 + 两行信息
        msgHtml = `
            <li id="${danmakuId}" class="danmaku-item">
                <div class="msg_line show-info">
                    <div class="${avatarClass} avatar-animate">
                        <img src="${data['face_url'] || 'http://__DOMAIN__:__PORT__/api/header?uid=' + uid}"
                            class="header ${avatarImgClass} avatar-img-animate"
                            onerror="this.src='http://__DOMAIN__:__PORT__/api/header?uid=' + ${uid}">
                    </div>
                    <div class="msg_line_info">
                        <div class="msg_line_info_row info-row-animate">
                            ${showMedal ? `<div class="medal-animate">${medalHtml}</div>` : ''}
                            ${unameHtml ? `<div class="uname-animate">${unameHtml}</div>` : ''}
                            ${showGuard ? `<div class="guard-animate">${guardHtml}</div>` : ''}
                        </div>
                        <div class="msg_line_content bubble-animate">
                            ${contentHtml}
                        </div>
                    </div>
                </div>
            </li>`;
    } else {
        // 简洁模式：只显示头像和弹幕内容
        msgHtml = `
            <li id="${danmakuId}" class="danmaku-item">
                <div class="msg_line">
                    <div class="${avatarClass} avatar-animate">
                        <img src="${data['face_url'] || 'http://__DOMAIN__:__PORT__/api/header?uid=' + uid}"
                            class="header ${avatarImgClass} avatar-img-animate"
                            onerror="this.src='http://__DOMAIN__:__PORT__/api/header?uid=' + ${uid}">
                    </div>
                    <div class="msg_line_content bubble-animate">
                        ${contentHtml}
                    </div>
                </div>
            </li>`;
    }

    // 添加到列表
    var $newMsg = $(msgHtml);
    $("#danmu-list").append($newMsg);
    
    // 记录弹幕信息
    var danmakuInfo = {
        id: danmakuId,
        time: time,
        element: $newMsg,
        timer: null,
        isRemoving: false,
        position: 0
    };
    danmakuList.push(danmakuInfo);

    // 设置初始状态
    var containerHeight = $("#danmu-list").height();
    $newMsg.css({
        'transform': `translateY(${containerHeight}px)`,
        'opacity': '0',
        'position': 'absolute',
        'width': '100%',
        'left': '0'
    });
    
    // 等待一帧后开始动画
    requestAnimationFrame(() => {
        // 添加入场动画类
        $newMsg.addClass('danmaku-enter');
        
        // 更新所有弹幕的位置
        updateDanmakuPositions();
    });

    // 设置自动移除定时器
    danmakuInfo.timer = setTimeout(() => {
        console.log("定时器触发，准备移除弹幕，ID:", danmakuId);
        if (danmakuInfo && !danmakuInfo.isRemoving) {
            console.log("准备移除弹幕");
            danmakuInfo.isRemoving = true;
            var index = danmakuList.indexOf(danmakuInfo);
            if (index !== -1) {
                removeMsgHtml(danmakuInfo.element, index);
            } else {
                console.log("弹幕已不在列表中，直接移除元素");
                danmakuInfo.element.remove();
            }
        }
    }, showTime * 1000);
}

/**
 * 更新所有弹幕的位置
 */
function updateDanmakuPositions() {
    var $allItems = $("#danmu-list li");
    var gap = 8;
    var totalHeight = 0;
    
    // 先计算每个弹幕的高度
    var heights = [];
    $allItems.each(function() {
        var $item = $(this);
        var itemHeight = $item.outerHeight(true);
        heights.push(itemHeight);
        totalHeight += itemHeight + gap;
    });
    
    // 更新弹幕列表的高度
    $("#danmu-list").css('height', totalHeight + 'px');
    
    // 设置每个弹幕的位置
    $allItems.each(function(index) {
        var $item = $(this);
        var currentTop = 0;
        
        // 计算当前位置
        for (var i = 0; i < index; i++) {
            currentTop += heights[i] + gap;
        }
        
        // 设置位置
        $item.css({
            'transform': `translateY(${currentTop}px)`,
            'position': 'absolute',
            'width': '100%',
            'left': '0'
        });
    });
    
    // 滚动到底部
    var danmuList = document.getElementById('danmu-list');
    if (danmuList && danmuList.scrollHeight > danmuList.clientHeight) {
        danmuList.scrollTo({
            top: danmuList.scrollHeight - danmuList.clientHeight,
            behavior: 'smooth'
        });
    }
}

/**
 * 移除弹幕HTML
 */
function removeMsgHtml(msg, index) {
    if (!msg || !msg.length) {
        console.log("移除被阻止：msg存在=", !!msg, "msg长度=", msg ? msg.length : 0);
        return;
    }
    
    // 如果正在移除，将新的移除操作加入队列
    if (removing) {
        setTimeout(() => removeMsgHtml(msg, index), 100);
        return;
    }
    
    removing = true;
    
    // 清除定时器
    var danmakuId = msg.attr('id');
    var danmakuInfo = danmakuList.find(item => item.id === danmakuId);
    if (danmakuInfo && danmakuInfo.timer) {
        clearTimeout(danmakuInfo.timer);
        danmakuInfo.timer = null;
    }
    
    console.log("开始移除弹幕，ID:", danmakuId);
    
    // 获取要移动的弹幕
    var $nextItems = msg.nextAll('li');
    var itemHeight = msg.outerHeight(true);
    var gap = 8;
    
    // 添加移除动画类
    msg.find('.msg_line').addClass('removing');
    
    // 让后续弹幕向上移动
    $nextItems.each(function() {
        var $item = $(this);
        var currentTop = parseInt($item.css('transform').split(',')[5] || 0);
        $item.css('transform', `translateY(${currentTop - itemHeight - gap}px)`);
    });
    
    // 等待动画完成后再移除元素
    setTimeout(() => {
        try {
            if (msg.length > 0 && msg.parent().length > 0) {
                // 从数组中移除
                if (typeof index === 'number' && index > -1 && index < danmakuList.length) {
                    danmakuList.splice(index, 1);
                } else {
                    var idx = danmakuList.findIndex(item => item.id === danmakuId);
                    if (idx > -1) {
                        danmakuList.splice(idx, 1);
                    }
                }
                
                // 移除元素
                msg.remove();
                
                // 更新所有弹幕的位置
                updateDanmakuPositions();
            }
        } catch (e) {
            console.error("移除弹幕时发生错误:", e);
        } finally {
            removing = false;
        }
    }, 300);
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
