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
    imgArr = ['1', '2', '3', '4', '5', '6', '7', '8', '9',]

for (let i = 0; i < items.length; i++) {
    if (imgArr[i] === 'empty') continue; // 不使用图片
    let el = items[i];
    el.style.backgroundImage = `url(./img/${imgArr[i]}.png)`
}
let modal = util.getELe('.modal'),
    mask = util.getELe('.modal-cover'),
    modalInner = util.getELe('.modal span');
let tryBtn = util.getELe('.confirm-btn');
let targetGiftIndex = 0;

// 存放每一个奖项的下标
let indexArray = [0, 1, 2, 5, 8, 7, 6, 3],
    currentIndex = 0,
    rotateCount = 0,
    stopTimer = null;
let countMin = 50, resultRand = 0;
const rotate = () => {
    // 先给所有的奖项盒子加蒙层
    for (let j = 0; j < indexArray.length; j++) {
        covers[indexArray[j]].style.background = 'rgba(0, 0, 0, 0.3)';
    }
    // 将当前奖项的遮罩层去除
    covers[indexArray[currentIndex]].style.background = 'none';
    targetGiftIndex = indexArray[currentIndex];
    currentIndex++;
    if (currentIndex === indexArray.length) {
        currentIndex = 0;
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
        stopTimer = null;

        // 发送随机结果
        sendResult(targetGiftIndex + 1, '');
    }
    // 点击再试一次
    tryBtn.addEventListener('click', () => {
        modal.style.display = 'none'
        mask.style.display = 'none'
    })
}
// 给开始按钮绑定点击事件 点击后执行 rotate 
const start = () => {
    if (stopTimer != null) {
        console.log('上一轮抽奖尚未结束');
        return;
    }
    resultRand = Math.floor(Math.random() * 8 + countMin);
    rotateCount = 0;
    console.log('resultRand: ' + resultRand);
    stopTimer = setInterval(rotate, 300);
}
covers[4].addEventListener("click", start);