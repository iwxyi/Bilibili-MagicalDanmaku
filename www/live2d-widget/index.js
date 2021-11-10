// 注意：live2d_path参数应使用绝对路径
const live2d_path = "./";

// 获取script DOM元素
var s = document.getElementsByTagName("script")[0]; 

// 加载live2d.min.js
var live2d = document.createElement("script");
live2d.src = live2d_path + "live2d.min.js";
s.parentNode.insertBefore(live2d, s);

// 加载waifu-tips.js
var waifu_tips = document.createElement("script");
waifu_tips.src = live2d_path + "waifu-tips.js";
s.parentNode.insertBefore(waifu_tips, s);

// 加载waifu.css
$("<link>").attr({ href: live2d_path + "waifu.css", rel: "stylesheet" }).appendTo("head");

// 初始化看板娘，会自动加载指定目录下的waifu-tips.json
$(window).on("load", function() {
	initWidget(live2d_path + "waifu-tips.json", "https://live2d.fghrsh.net/api");
});
//initWidget第一个参数为waifu-tips.json的路径
//第二个参数为api地址（无需修改）
//api后端可自行搭建，参考https://github.com/fghrsh/live2d_api