#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
抖音 Signature 服务器
提供 HTTP 接口获取抖音直播 signature
"""

import asyncio
import json
import logging
import time
from datetime import datetime, time as dt_time
from typing import Optional

import aiohttp
from aiohttp import web
from playwright.async_api import async_playwright, Browser, Page

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        # logging.FileHandler('douyin_signature_server.log', encoding='utf-8'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

class DouyinSignatureServer:
    def __init__(self):
        self.browser: Optional[Browser] = None
        self.page: Optional[Page] = None
        self.is_initialized = False
        self.last_refresh_time = None
        
    async def initialize_browser(self):
        """初始化浏览器"""
        try:
            playwright = await async_playwright().start()
            self.browser = await playwright.chromium.launch(
                headless=True,
                args=[
                    '--no-sandbox',
                    '--disable-dev-shm-usage',
                    '--disable-gpu',
                    '--disable-web-security',
                    '--disable-features=VizDisplayCompositor'
                ]
            )
            self.page = await self.browser.new_page()
            
            # 设置用户代理
            await self.page.set_extra_http_headers({
                'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36'
            })
            
            logger.info("浏览器初始化完成")
            return True
        except Exception as e:
            logger.error(f"浏览器初始化失败: {e}")
            return False
    
    async def load_douyin_homepage(self):
        """加载抖音首页"""
        try:
            logger.info("开始加载抖音首页...")
            await self.page.goto('https://live.douyin.com', wait_until='networkidle')
            
            # 等待 signature 相关 JS 加载
            await self.page.wait_for_function(
                'typeof window.byted_acrawler !== "undefined"',
                timeout=30000
            )
            
            self.is_initialized = True
            self.last_refresh_time = datetime.now()
            logger.info("抖音首页加载完成，signature 功能可用")
            return True
        except Exception as e:
            logger.error(f"加载抖音首页失败: {e}")
            return False
    
    async def get_signature(self, room_id: str, unique_id: str) -> Optional[str]:
        """获取 signature"""
        if not self.is_initialized or not self.page:
            logger.error("页面未初始化")
            return None
        
        try:
            # 生成 X-MS-STUB (MD5 哈希)
            import hashlib
            stub_str = f"live_id=1,aid=6383,version_code=180800,webcast_sdk_version=1.0.14-beta.0,room_id={room_id},sub_room_id=,sub_channel_id=,did_rule=3,user_unique_id={unique_id},device_platform=web,device_type=,ac=,identity=audience"

            # 计算 MD5 哈希
            stub_hash = hashlib.md5(stub_str.encode('utf-8')).hexdigest()
            
            # 执行 JS 获取 signature
            signature = await self.page.evaluate(f"""
                (() => {{
                    try {{
                        const result =  window.byted_acrawler.frontierSign({{'X-MS-STUB': '{stub_hash}'}});
                        // 如果返回的是对象，提取 X-Bogus 字段
                        if (result && typeof result === 'object' && result['X-Bogus']) {{
                            return result['X-Bogus'];
                        }} else if (result && typeof result === 'string') {{
                            return result;
                        }} else {{
                            return null;
                        }}
                    }} catch (e) {{
                        console.error('Signature 计算失败:', e);
                        return null;
                    }}
                }})()
            """)
            
            if signature:
                logger.info(f"获取 signature 成功: room_id={room_id}, unique_id={unique_id}")
                return signature
            else:
                logger.error("Signature 计算返回空值")
                return None
                
        except Exception as e:
            logger.error(f"获取 signature 失败: {e}")
            return None
    
    async def refresh_homepage(self):
        """刷新抖音首页"""
        logger.info("开始刷新抖音首页...")
        success = await self.load_douyin_homepage()
        if success:
            logger.info("抖音首页刷新成功")
        else:
            logger.error("抖音首页刷新失败")
        return success
    
    async def schedule_refresh(self):
        """定时刷新任务"""
        while True:
            now = datetime.now()
            target_time = now.replace(hour=2, minute=0, second=0, microsecond=0)
            
            # 如果今天已经过了2点，就等到明天2点
            if now.time() >= dt_time(2, 0):
                target_time = target_time.replace(day=target_time.day + 1)
            
            # 计算等待时间
            wait_seconds = (target_time - now).total_seconds()
            logger.info(f"下次刷新时间: {target_time}, 等待 {wait_seconds:.0f} 秒")
            
            await asyncio.sleep(wait_seconds)
            await self.refresh_homepage()

# 全局服务器实例
server_instance = DouyinSignatureServer()

async def handle_signature(request):
    """处理 signature 请求"""
    try:
        # 获取参数
        room_id = request.query.get('roomId')
        unique_id = request.query.get('uniqueId')
        
        if not room_id or not unique_id:
            return web.json_response({
                'success': False,
                'error': '缺少参数 roomId 或 uniqueId'
            }, status=400)
        
        # 获取 signature
        signature = await server_instance.get_signature(room_id, unique_id)
        
        if signature:
            return web.json_response({
                'success': True,
                'signature': signature,
                'roomId': room_id,
                'uniqueId': unique_id,
                'timestamp': int(time.time())
            })
        else:
            return web.json_response({
                'success': False,
                'error': '获取 signature 失败'
            }, status=500)
            
    except Exception as e:
        logger.error(f"处理请求失败: {e}")
        return web.json_response({
            'success': False,
            'error': f'服务器内部错误: {str(e)}'
        }, status=500)

async def handle_status(request):
    """处理状态查询请求"""
    return web.json_response({
        'success': True,
        'initialized': server_instance.is_initialized,
        'last_refresh_time': server_instance.last_refresh_time.isoformat() if server_instance.last_refresh_time else None,
        'timestamp': int(time.time())
    })

async def handle_refresh(request):
    """手动刷新请求"""
    try:
        success = await server_instance.refresh_homepage()
        return web.json_response({
            'success': success,
            'message': '刷新成功' if success else '刷新失败',
            'timestamp': int(time.time())
        })
    except Exception as e:
        logger.error(f"手动刷新失败: {e}")
        return web.json_response({
            'success': False,
            'error': f'刷新失败: {str(e)}'
        }, status=500)

async def init_app():
    """初始化应用"""
    app = web.Application()
    
    # 添加路由
    app.router.add_get('/signature', handle_signature)
    app.router.add_get('/status', handle_status)
    app.router.add_post('/refresh', handle_refresh)
    
    # 添加 CORS 支持
    async def cors_middleware(app, handler):
        async def middleware(request):
            if request.method == 'OPTIONS':
                response = web.Response()
            else:
                response = await handler(request)
            
            response.headers['Access-Control-Allow-Origin'] = '*'
            response.headers['Access-Control-Allow-Methods'] = 'GET, POST, OPTIONS'
            response.headers['Access-Control-Allow-Headers'] = 'Content-Type'
            return response
        return middleware
    
    app.middlewares.append(cors_middleware)
    
    return app

async def main():
    """主函数"""
    logger.info("启动抖音 Signature 服务器...")
    
    # 初始化浏览器
    if not await server_instance.initialize_browser():
        logger.error("浏览器初始化失败，退出程序")
        return
    
    # 加载抖音首页
    if not await server_instance.load_douyin_homepage():
        logger.error("抖音首页加载失败，退出程序")
        return
    
    # 创建应用
    app = await init_app()
    
    # 启动定时刷新任务
    asyncio.create_task(server_instance.schedule_refresh())
    
    # 启动服务器
    runner = web.AppRunner(app)
    await runner.setup()
    site = web.TCPSite(runner, '0.0.0.0', 5531)
    
    logger.info("服务器启动成功，监听端口 5531")
    logger.info("API 接口:")
    logger.info("  GET  /signature?roomId=xxx&uniqueId=yyy  - 获取 signature")
    logger.info("  GET  /status                           - 查询服务器状态")
    logger.info("  POST /refresh                          - 手动刷新首页")
    
    await site.start()
    
    try:
        # 保持服务器运行
        await asyncio.Future()
    except KeyboardInterrupt:
        logger.info("收到退出信号，正在关闭服务器...")
    finally:
        await runner.cleanup()
        if server_instance.browser:
            await server_instance.browser.close()
        logger.info("服务器已关闭")

if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("程序被用户中断")
    except Exception as e:
        logger.error(f"程序异常退出: {e}")
