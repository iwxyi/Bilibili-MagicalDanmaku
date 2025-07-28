#!/bin/bash

# 抖音 Signature 服务器启动脚本

echo "正在启动抖音 Signature 服务器..."

# 检查 Python 版本
python_version=$(python3 --version 2>&1)
echo "Python 版本: $python_version"

# 检查是否已安装依赖
if ! python3 -c "import aiohttp, playwright" 2>/dev/null; then
    echo "正在安装依赖..."
    pip3 install -r requirements.txt
    
    echo "正在安装 Playwright 浏览器..."
    playwright install chromium
fi

# 启动服务器
echo "启动服务器..."
python3 douyin_signature_server.py 