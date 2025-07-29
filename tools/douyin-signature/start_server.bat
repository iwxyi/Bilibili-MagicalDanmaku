@echo off
chcp 65001 >nul

echo 正在启动抖音 Signature 服务器...

REM 检查 Python 版本
python --version
if errorlevel 1 (
    echo Python 未安装或不在 PATH 中
    pause
    exit /b 1
)

REM 使用虚拟环境
REM 检查是否存在虚拟环境
if not exist venv (
    echo 正在创建虚拟环境...
    python -m venv venv
)
REM 激活虚拟环境
call venv\Scripts\activate

REM 检查是否已安装依赖
python -c "import aiohttp, playwright" 2>nul
if errorlevel 1 (
    echo 正在安装依赖...
    pip install -r requirements.txt
    
    echo 正在安装 Playwright 浏览器...
    playwright install chromium
)

REM 实测多个win上的playwright的浏览器路径总是错误
REM 通过 playwright install --list 来查看已安装的浏览器路径，和运行py报错的路径对比
REM 实际安装路径为：C:\Users\Administrator\AppData\Local\ms-playwright
REM 需要的检测路径：[Python安装路径]\lib\site-packages\playwright\driver\package\.local-browsers

REM 启动服务器
echo 启动服务器...
python douyin_signature_server.py

pause 