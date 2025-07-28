# 抖音 Signature 服务器

这是一个 Python HTTP 服务器，用于获取抖音直播的 signature 参数，替代 Qt WebEngine 方案。

## 功能特点

- 🚀 **高性能**: 使用 Playwright 无头浏览器，加载一次抖音首页后保持运行
- ⏰ **自动刷新**: 每天凌晨 2 点自动刷新抖音首页
- 🔄 **手动刷新**: 支持手动刷新接口
- 📊 **状态监控**: 提供服务器状态查询接口
- 🌐 **跨平台**: 支持 Windows、macOS、Linux

## 安装依赖

### 方法一：使用启动脚本（推荐）

**Linux/macOS:**
```bash
chmod +x start_server.sh
./start_server.sh
```

**Windows:**
```cmd
start_server.bat
```

### 方法二：手动安装

```bash
# 安装 Python 依赖
pip install -r requirements.txt

# 安装 Playwright 浏览器
playwright install chromium
```

## 启动服务器

```bash
python douyin_signature_server.py
```

服务器将在端口 5531 上启动。

## API 接口

### 1. 获取 Signature

**请求:**
```
GET /signature?roomId=房间ID&uniqueId=用户ID
```

**响应:**
```json
{
    "success": true,
    "signature": "计算出的signature",
    "roomId": "房间ID",
    "uniqueId": "用户ID",
    "timestamp": 1640995200
}
```

### 2. 查询服务器状态

**请求:**
```
GET /status
```

**响应:**
```json
{
    "success": true,
    "initialized": true,
    "last_refresh_time": "2024-01-01T02:00:00",
    "timestamp": 1640995200
}
```

### 3. 手动刷新首页

**请求:**
```
POST /refresh
```

**响应:**
```json
{
    "success": true,
    "message": "刷新成功",
    "timestamp": 1640995200
}
```

## 在 Qt 项目中使用

修改你的 `douyin_liveservice.cpp` 中的 `getSignature` 方法：

```cpp
QString DouyinLiveService::getSignature(QString roomId, QString uniqueId)
{
    // 使用本地 HTTP 服务器
    QString url = QString("http://localhost:5531/signature?roomId=%1&uniqueId=%2")
                     .arg(roomId).arg(uniqueId);
    
    MyJson response = getToJson(url);
    if (response.i("success") == 1) {
        return response.s("signature");
    } else {
        qWarning() << "获取 signature 失败:" << response.s("error");
        return "";
    }
}
```

## 日志文件

服务器运行时会生成 `douyin_signature_server.log` 日志文件，记录运行状态和错误信息。

## 注意事项

1. **首次启动**: 首次启动时会下载 Chromium 浏览器，可能需要几分钟时间
2. **内存使用**: 服务器会保持一个浏览器实例运行，大约占用 100-200MB 内存
3. **网络要求**: 需要能够访问 `https://live.douyin.com`
4. **端口占用**: 确保端口 5531 未被其他程序占用

## 故障排除

### 1. 启动失败

- 检查 Python 版本（需要 3.7+）
- 检查网络连接
- 查看日志文件中的错误信息

### 2. Signature 获取失败

- 检查服务器状态：`GET /status`
- 尝试手动刷新：`POST /refresh`
- 检查抖音首页是否正常加载

### 3. 浏览器启动失败

- 确保已安装 Chromium：`playwright install chromium`
- 在 Linux 上可能需要安装额外依赖：`sudo apt-get install libnss3 libatk-bridge2.0-0 libdrm2 libxkbcommon0 libxcomposite1 libxdamage1 libxrandr2 libgbm1 libasound2`

## 许可证

本项目遵循 MIT 许可证。 