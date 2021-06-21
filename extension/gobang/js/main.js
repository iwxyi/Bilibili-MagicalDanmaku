/**
 * UI
 * @author Airing
 */

var canvas = document.getElementById("chess");
var context = canvas.getContext("2d");
var me = true;              // 判断该轮黑白棋落子权
var over = false;           // 判断游戏是否结束
var chessBoard = [];        // 棋盘二维数组,存储棋盘信息

/**
 * 开始按钮逻辑:初始化棋盘,并让电脑黑棋先行(7,7)位置
 */
function startGame() {
    
    // 初始化棋盘信息
    for (var i = 0; i < 15; i++) {
        chessBoard[i] = [];
        for (var j = 0; j < 15; j++) {
            chessBoard[i][j] = 0;
        }
    }
    
    // 清除棋盘
    cleanChess();
    // 绘制棋盘
    drawChess();
    
    // 轮到玩家(白棋)行棋
    me = true;
    // 重置游戏结束标志
    over = false;
    
    // 初始化赢法统计数组
    for (var i = 0; i < count; i++) {
        myWin[i] = 0;
        airingWin[i] = 0;
    }
    
    // 让电脑先行，(7,7)处绘制黑棋，并存储信息
    oneStep(7, 7, false);
    chessBoard[7][7] = 2;
}

/**
 * 清除棋盘
 */
function cleanChess() {
    context.fillStyle = "#FFFFFF";
    context.fillRect(0, 0, canvas.width, canvas.height);
}

/**
 * 绘制棋盘
 */
function drawChess() {
    for (var i = 0; i < 15; i++) {
        context.strokeStyle = "#BFBFBF";
        context.beginPath();
        context.moveTo(15 + i *30, 15);
        context.lineTo(15 + i *30, canvas.height - 15);
        context.closePath();
        context.stroke();
        context.beginPath();
        context.moveTo(15, 15 + i *30);
        context.lineTo(canvas.width - 15, 15 + i * 30);
        context.closePath();
        context.stroke();
    }
}

/**
 * 绘制棋子
 * @param i     棋子x轴位置
 * @param j     棋子y轴位置
 * @param me    棋子颜色
 */
function oneStep(i, j ,me) {
    context.beginPath();
    context.arc(15 + i * 30, 15 + j * 30, 13, 0, 2 * Math.PI);
    context.closePath();
    var gradient = context.createRadialGradient(15 + i * 30 + 2, 15 + j * 30 - 2, 13, 15 + i * 30 + 2, 15 + j * 30 - 2, 0);
    if (me) {
        gradient.addColorStop(0, "#D1D1D1");
        gradient.addColorStop(1, "#F9F9F9");
    } else {
        gradient.addColorStop(0, "#0A0A0A");
        gradient.addColorStop(1, "#636766");
    }
    context.fillStyle = gradient;
    context.fill();
}


function setChess(i, j) {
    // 如果该位置没有棋子,则允许落子
    if (chessBoard[i][j] == 0) {
        // 绘制棋子(玩家)
        oneStep(i, j, me);
        // 改变棋盘信息(该位置有棋子)
        chessBoard[i][j] = 1;

        // 遍历赢法统计数组
        for (var k = 0; k < count; k++) {
            if (wins[i][j][k]) {
                // 如果存在赢法,则玩家此赢法胜算+1(赢法为5胜取胜)
                myWin[k]++;
                // 如果存在赢法,则电脑此赢法胜算赋值为6(永远不等于5,永远无法在此处取胜)
                airingWin[k] = 6;
                // 玩家落子后,此处赢法数组凑够5,玩家取胜
                if (myWin[k] == 5) {
                    window.alert("You Win");
                    // 游戏结束
                    over = true;
                }
            }

        }
        //airingGo2()

        // 如果游戏没有结束,轮到电脑行棋
        if (!over) {
            me = !me;
            airingGo();
        }
    }
}


/**
 * canvas 鼠标点击事件
 * @param e
 */
canvas.onclick = function(e) {
    if (over) {
        return;
    }

    var x = e.offsetX;
    var y = e.offsetY;
    var i = Math.floor(x / 30);
    var j = Math.floor(y / 30);
    setChess(i, j);
};

