/*!
 * eazyDanmuku v1.0.7
 * (c) 2020 Peng Pan
 * @license MIT
 */
class EasyDanmaku {
    constructor(params) {
        /* ------ 初始化属性 start--------   */
        this.container = this.checkParams(params); //弹幕容器 | danmaku Parent container
        this.wrapperStyle = params.wrapperStyle || null; //弹幕样式 | danmaku style
        this.line = params.line || 10; //弹幕总行数 | danmaku lines(default ten line)
        this.speed = params.speed || 5; //单条弹幕速度 | danmaku speed(default five 5)
        this.runtime = params.runtime || 10; //弹幕列表播放时长,单位为秒 | running time
        this.colourful = params.colourful || false; //彩色弹幕 | colourful danmaku(default false) 
        this.loop = params.loop || false; //是否循环播放 | loop play
        this.hover = params.hover || false; //鼠标悬停是否暂停 | hover pause
        this.coefficient = params.coefficient || 1.38; //同时刻弹幕系数  | danmaku Density factor
        /* ------ 内部属性 start --------   */
        this.originIndex = 0; //弹幕列表起始下标 | danmaku Density factor
        this.originList = null; //备用列表  | Alternate list
        this.offsetValue = this.container.offsetHeight / this.line; //弹幕排列偏移量 | danmaku offsetValue
        this.vipIndex = 0; //vip弹幕下标 | vip danmaku subscript
        this.overflowArr = []; //溢出队列  | danmaku overflow Array
        this.clearIng = false; //是否正在处理溢出弹幕 | whether  handle overflow danmaku
        this.cleartimer = null; //定时器(处理溢出弹幕) | 
        this.init();
        this.handleEvents(params); //事件注册 | handle Event
    }
    /**
     * @description 事件注册
     * @private
     * @param {object} params 事件
     */
    handleEvents(params) {
        this.onComplete = params.onComplete || null;
        this.onHover = params.onHover || null;
    }
    /**
     * @description easyDanmaku初始化 设置弹幕容器基础样式，初始化弹幕赛道
     * @private
     */
    init() {
        this.runstatus = 1 // 0 | 1
        this.aisle = [];
        this.container.style.overflow = 'hidden';
        if (this.hover) this.handleMouseHover();
        if (Utils.getStyle(this.container, 'position') !== 'relative' && Utils.getStyle(this.container, 'position') !== 'fixed') {
            this.container.style.position = 'relative';
        }
        for (let i = 0; i < this.line; i++) {
            this.aisle.push({
                normalRow: true,
                vipRow: true
            });
        }
    }
    /**
     * @description 初始化参数类型校验
     * @private
     * @param {object} 初始化参数
     * @returns {HTMLElement} 弹幕容器 
     */
    checkParams(params) {
        if (!document.querySelector(params.el)) throw `Could not find the ${params.el} element`
        if (params.wrapperStyle && typeof params.wrapperStyle !== 'string') throw `The type accepted by the wrapperStyle parameter is string`
        if (params.line && typeof params.line !== 'number') throw `The type accepted by the line parameter is number`
        if (params.speed && typeof params.speed !== 'number') throw `The type accepted by the speed parameter is number`
        if (params.colourful && typeof params.colourful !== 'boolean') throw `The type accepted by the colourful parameter is boolean`
        if (params.runtime && typeof params.runtime !== 'number') throw `The type accepted by the runtime parameter is number`
        if (params.loop && typeof params.loop !== 'boolean') throw `The type accepted by the loop parameter is boolean`
        if (params.coefficient && typeof params.coefficient !== 'number') throw `The type accepted by the coefficient parameter is number`
        if (params.hover && typeof params.hover !== 'boolean') throw `The type accepted by the hover parameter is boolean`
        if (params.onComplete && typeof params.onComplete !== 'function') throw `The type accepted by the onComplete parameter is function`
        if (params.onHover && typeof params.onHover !== 'function') throw `The type accepted by the onHover parameter is function`
        return document.querySelector(params.el)
    }

    /**
     * @description 单条弹幕发送 批量弹幕使用batchSend方法
     * @param {string} content 弹幕内容
     * @param {string} normalClass 弹幕默认样式
     * @param {function} callback 弹幕播放结束后的回调函数 @returns {object} 本条弹幕的一些基本信息
     * @public
     */
    send(content, normalClass = null, callback = null) {
        if (this.runstatus == 0) {
            this.overflowArr.push({
                content,
                normalClass
            });
            return
        }
        if (content.length < 1) return
        let sheet = document.createElement('div');
        let index = 0;
        let speed = this.speed;
        let timer = null;
        let origin = 0;
        sheet.innerHTML = content;
        sheet.style.display = 'inline-block';
        sheet.classList.add('default-style')
        if (normalClass || this.wrapperStyle) {
            sheet.classList.add(normalClass || this.wrapperStyle);
        }
        containerAppendChild.call(this);

        function containerAppendChild() {
            index = Math.round(Math.random() * (this.line - 1));
            if (this.aisle[index].normalRow) {
                this.aisle[index].normalRow = false;
                this.container.appendChild(sheet);
                speed += sheet.offsetWidth / sheet.parentNode.offsetWidth * 2;
                sheet.style.cssText = `
                    text-align:center;
                    min-width:130px;
                    will-change: transform;
                    position:absolute;
                    right: -${sheet.offsetWidth+130}px;
                    transition: transform ${speed}s linear;
                    transform: translateX(-${sheet.parentNode.offsetWidth + sheet.offsetWidth + 130}px);
                    top: ${index * this.offsetValue}px;
                    line-height:${this.offsetValue}px;
                    color:${this.colourful?'#'+('00000'+(Math.random()*0x1000000<<0).toString(16)).substr(-6):void(0)}
                `
                //开启弹幕通道
                let unit = (sheet.parentNode.offsetWidth + sheet.offsetWidth) / speed / 60;
                timer = setInterval(() => {
                    origin += unit;
                    if (origin > sheet.offsetWidth * this.coefficient) {
                        this.aisle[index].normalRow = true;
                        clearInterval(timer);
                    }
                }, 16.66)

                // 删除已播放弹幕
                setTimeout(() => {
                    if (sheet.getAttribute('relieveDel') == 1) return
                    if (callback) callback({
                        runtime: speed,
                        target: sheet,
                        width: sheet.offsetWidth
                    });
                    sheet.remove();
                }, speed * 1000);
            } else {
                // 重新选择通道
                let some = this.aisle.some(item => item.normalRow === true);
                some ? containerAppendChild.call(this) : (() => {
                    this.overflowArr.push({
                        content,
                        normalClass
                    });
                    if (!this.clearIng) {
                        //开始清理溢出弹幕
                        this.clearOverflowDanmakuArray();
                    }
                })()
            }
        }
    }
    /**
     * @description 批量发送弹幕
     * @param {Array} list
     * @param {Boolean} hasAvatar 是否包含头像
     * @param {string} normalClass 此批弹幕样式
     * @public
     */
    batchSend(list, hasAvatar = false, normalClass = null) {
        let runtime = this.runtime || list.length * 1.23;
        this.originList = list;
        this.hasAvatar = hasAvatar;
        this.normalClass = normalClass;
        let timer = setInterval(() => {
            if (this.originIndex > list.length - 1) {
                clearInterval(timer);
                this.originIndex = 0;
                if (this.onComplete) this.onComplete();
                if (this.loop) this.batchSend(this.originList, hasAvatar, normalClass);
            } else {
                if (hasAvatar) {
                    this.send(`<img src="${list[this.originIndex].avatar}" class="sender-header" />
                        <span>${list[this.originIndex].content}</span>
                    `, normalClass || this.wrapperStyle);
                    /* this.send(`<div class="mdui-chip">
                    <img class="mdui-chip-icon" src="${list[this.originIndex].avatar}"  />
                    <span class="mdui-chip-title">${list[this.originIndex].content}</span>
                  </div>`); */
                } else {
                    this.send(list[this.originIndex], normalClass || this.wrapperStyle);
                }
                this.originIndex++;
            }
        }, runtime / list.length * 1000)
    }

    /**
     * @description 居中发送弹幕
     * @param {string} content 
     * @param {string} normalClass
     * @param {number} duration
     * @param {function} callback
     * @public
     */
    centeredSend(content, normalClass, duration = 3000, callback = null) {

        let sheet = document.createElement('div');
        let index = 0;
        sheet.innerHTML = content;
        if (normalClass || this.wrapperStyle) {
            sheet.classList.add(normalClass || this.wrapperStyle);
        }
        containerAppendChild.call(this);

        function containerAppendChild() {
            if (this.aisle[index].vipRow) {
                this.container.appendChild(sheet);
                sheet.style.cssText = `
                    position:absolute;
                    left:50%;
                    transform:translateX(-50%);
                    top: ${index * this.offsetValue}px;
                `
                this.aisle[index].vipRow = false;
                setTimeout(() => {
                    if (callback) callback({
                        duration: duration,
                        target: sheet,
                        width: sheet.offsetWidth
                    });
                    sheet.remove();
                    this.aisle[index].vipRow = true;
                }, duration)

            } else {

                index++;
                if (index > this.line - 1) return
                containerAppendChild.call(this);
            }
        }

    }

    /**
     * @description 播放
     * @public
     */
    play() {
        const allDanmaku = this.container.children;
        for (let i = 0; i < allDanmaku.length; i++) {
            this.controlDanmakurunStatus(allDanmaku[i], 1);
        }
        this.runstatus = 1;
        if (this.overflowArr.length !== 0) this.clearOverflowDanmakuArray();
    }

    /**
     * @description 暂停
     * @public
     */
    pause() {
        const allDanmaku = this.container.children;
        for (let i = 0; i < allDanmaku.length; i++) {
            this.controlDanmakurunStatus(allDanmaku[i], 0);
        }
        this.runstatus = 0;
    }

    /**
     * @description 控制弹幕运动状态 暂停|播放
     * @param {HTMLElement} 目标dom
     * @param {number} 1(播放) | 0(暂停)
     * 
     */
    controlDanmakurunStatus(target, status) {
        const extensiveStatus = {
            play: 1,
            pause: 0
        }
        const RegExpforTranslateX = /-(\S*),/;
        if (status === extensiveStatus.play) {
            // 播放
            clearTimeout(target.timer)
            const translateX = Utils.getStyle(target, 'transform').match(RegExpforTranslateX)[1];
            target.style['transition'] = `transform ${this.speed}s linear`;
            target.style['transform'] = `translateX(-${target.parentNode.offsetWidth + parseInt(translateX) + target.offsetWidth + 130}px)`;
            target.timer = setTimeout(() => {
                target.remove();
            }, this.speed * 1000);
        } else if (status === extensiveStatus.pause) {
            // 暂停
            const translateX = Utils.getStyle(target, 'transform').match(RegExpforTranslateX)[1];
            target.style['transition'] = 'transform 0s linear';
            target.style['transform'] = `translateX(-${translateX}px)`;
            target.setAttribute('relieveDel', 1);

        }
    }


    /**
     * @description 鼠标移入悬停
     * @private
     */
    handleMouseHover() {
        Utils.eventDelegation(this.container, 'default-style', 'mouseover', (activeDom) => {
            activeDom.style['z-index'] = 1000;
            this.controlDanmakurunStatus(activeDom, 0); //暂停
            if (this.onHover) this.onHover(activeDom);
        })
        Utils.eventDelegation(this.container, 'default-style', 'mouseout', (activeDom) => {
            activeDom.style.zIndex = 1
            this.controlDanmakurunStatus(activeDom, 1); //播放

        })
    }

    /**
     * @description 用于处理溢出弹幕
     * @public
     */
    clearOverflowDanmakuArray() {
        clearInterval(this.cleartimer);
        this.clearIng = true;
        let timerLimit = 0;
        this.cleartimer = setInterval(() => {
            if (this.overflowArr.length === 0) {
                timerLimit++;

                if (timerLimit > 20) {
                    // 无新入溢出弹幕关闭清理
                    clearInterval(this.cleartimer);
                    this.clearIng = false;
                }
            } else {
                this.send(this.overflowArr[0].content, this.overflowArr[0].normalClass || this.wrapperStyle);
                this.overflowArr.shift();
            }
        }, 500)
    }
}

/**
 * @class 工具类
 */
class Utils {

    /**
     * @description 获取元素样式
     * @public
     * @param {string} el 获取样式的节点 
     * @param {string} attr 获取的样式名
     * @returns {string} 元素样式
     */
    static getStyle(el, attr) {
        return window.getComputedStyle(el, null)[attr];
    }

    /**
     * @description 事件委托
     * @param {string}   parent 被事件委托对象
     * @param {string}   childClassName  事件委托的对象类名
     * @param {string}   EventName  所委托的事件名
     * @param {function} callBackFn 触发事件的回调(e:触发事件的元素)
     * @private
     */
    static eventDelegation(parent, childName, EventName, callBackFn) {
        parent.addEventListener(EventName, (e) => {
            const containerDom = e.target;
            if (containerDom.className.includes(childName)) {
                callBackFn(containerDom)
            }
        })
    }
}