// 封装工具函数
const util = {
    getELe: (str) => {
        return document.querySelector(str)
    },
    getELes: (str) => {
        return document.querySelectorAll(str)
    }
}

// 设置背景图片
let items = util.getELes(".item-box"),
    covers = util.getELes('.cover'),
    names = util.getELes('.item-name'),
    imgArr = ['1', '2', '3', '4', '5', '6', '7', '8', '9',]

for (let i = 0; i < items.length; i++) {
    if (imgArr[i] === 'empty') continue; // 不使用图片
    let el = items[i];
    el.style.backgroundImage = `url(./img/${imgArr[i]}.png)`;
}

// 设置弹窗
let modal = util.getELe('.modal'),
    mask = util.getELe('.modal-cover'),
    modalInner = util.getELe('.modal span');
let tryBtn = util.getELe('.confirm-btn');

// 存放每一个奖项的下标
let indexArray = [0, 1, 2, 5, 8, 7, 6, 3],
    currentArrayIndex = 0, // 当前在indexArray中的下标（不是九宫格索引）
    rotateCount = 0, // 移动次数
    stopTimer = null;
let currentGiftIndex = 0; // 当前转到的礼物九宫格Index（一开始就决定好了）
let countMin = 48, resultRand = 0;
const rotate = () => {
    // 先给所有的奖项盒子加蒙层
    for (let j = 0; j < indexArray.length; j++) {
        covers[indexArray[j]].style.background = 'rgba(0, 0, 0, 0.3)';
    }
    // 将当前奖项的遮罩层去除
    covers[indexArray[currentArrayIndex]].style.background = 'none';
    currentGiftIndex = indexArray[currentArrayIndex];
    currentArrayIndex++;
    if (currentArrayIndex === indexArray.length) {
        currentArrayIndex = 0;
    }
    // 通过count调整旋转速度
    rotateCount++;
    // 根据count 重新调整计时器速度
    if (rotateCount === 5 || rotateCount === 45) {
        clearInterval(stopTimer);
        stopTimer = setInterval(rotate, 200);
    }
    if (rotateCount === 10 || rotateCount === 35) {
        clearInterval(stopTimer);
        stopTimer = setInterval(rotate, 100);
    }
    if (rotateCount === 15) {
        clearInterval(stopTimer);
        stopTimer = setInterval(rotate, 50);
    }
    // 固定抽中某个奖项
    // if (count === 40) {
    //     clearInterval(stopTimer);
    //     count = 0;
    //     rand = 0;
    //     setTimeout(() => {
    //         modalInner.innerText = '亲！恭喜你中奖大宝SOD蜜一瓶！^_^ 😄';
    //         modal.style.display = 'block'
    //         mask.style.display = 'block'
    //     }, 500);
    // }

    // 当等于上面的随机数时，抽奖结束
    if (rotateCount === resultRand) {
        clearInterval(stopTimer);

        // 发送随机结果
        sendResult(currentGiftIndex, '');

        // 如果有排队的，则继续抽奖
        if (userQueue.length > 0) {
            // 展示几秒钟结果，再继续抽奖
            // 这里不把stopTimer设置成null是为了避免中途有新抽奖
            setTimeout(function () {
                stopTimer = null;
                start();
            }, 2000);
        } else {
            // 结束这一波抽奖
            stopTimer = null;
        }
    }
    // 点击再试一次
    // tryBtn.addEventListener('click', () => {
    //     modal.style.display = 'none'
    //     mask.style.display = 'none'
    // })
}
// 给开始按钮绑定点击事件 点击后执行 rotate 
const start = () => {
    if (stopTimer != null) {
        console.log('已加入抽奖队列，当前人数：' + userQueue.length);
        return;
    }
    if (userQueue.length <= 0) {
        console.log('没有可用于抽奖的用户，使用测试用户');
        currentUser = new UserInfo(123455, '测试用户');
    } else {
        currentUser = userQueue.shift();
    }

    resultRand = getRandomResult();
    rotateCount = 0;
    console.log('resultRand: ' + resultRand);
    stopTimer = setInterval(rotate, 300);

    $('#current-uname').text(currentUser.uname);
}
covers[4].addEventListener("click", start);