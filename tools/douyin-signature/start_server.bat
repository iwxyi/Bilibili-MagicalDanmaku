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

REM 检查是否已安装依赖
python -c "import aiohttp, playwright" 2>nul
if errorlevel 1 (
    echo 正在安装依赖...
    pip install -r requirements.txt
    
    echo 正在安装 Playwright 浏览器...
    playwright install chromium
)

REM 启动服务器
echo 启动服务器...
python douyin_signature_server.py

pause 