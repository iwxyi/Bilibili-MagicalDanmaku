
//检测用户活动状态，并在空闲时定时显示一言
var userAction = false,
	hitokotoTimer = null,
	messageTimer = null,
	messageArray = ["好久不见，日子过得好快呢……", "大坏蛋！你都多久没碰人家了呀，嘤嘤嘤～", "嗨～快来逗我玩吧！", "拿小拳拳锤你胸口！"];



function loadWidget(waifuPath, apiPath) {
	localStorage.removeItem("waifu-display");
	sessionStorage.removeItem("waifu-text");
	$("body").append(`<div id="waifu">
			<div id="waifu-tips"></div>
			<canvas id="live2d" width="300" height="300"></canvas>
			<div id="waifu-tool">
				<span class="fa fa-lg fa-user-circle"></span>
				<span class="fa fa-lg fa-street-view"></span>
			</div>
		</div>`);
	$("#waifu").show().animate({ bottom: 0 }, 0);

	function registerEventListener() {
		$("#waifu-tool .fa-user-circle").click(loadOtherModel);
		$("#waifu-tool .fa-street-view").click(loadRandModel);
	}
	registerEventListener();

	function welcomeMessage() {
		var now = new Date().getHours();
		if (now > 5 && now <= 7) text = "早上好！一日之计在于晨，美好的一天就要开始了。";
		else if (now > 7 && now <= 11) text = "上午好！工作顺利嘛，不要久坐，多起来走动走动哦！";
		else if (now > 11 && now <= 14) text = "中午了，工作了一个上午，现在是午餐时间！";
		else if (now > 14 && now <= 17) text = "午后很容易犯困呢，今天的运动目标完成了吗？";
		else if (now > 17 && now <= 19) text = "傍晚了！窗外夕阳的景色很美丽呢，最美不过夕阳红～";
		else if (now > 19 && now <= 21) text = "晚上好，今天过得怎么样？";
		else if (now > 21 && now <= 23) text = ["已经这么晚了呀，早点休息吧，晚安～", "深夜时要爱护眼睛呀！"];
		else text = "你是夜猫子呀？这么晚还不睡觉，明天起的来嘛？";
		showMessage(text, 7000, 8);
	}
	welcomeMessage();
	setInterval(() => {
		if (!saying) {
			if (!hitokotoTimer) hitokotoTimer = setInterval(showHitokoto, 25000);
		} else {
			saying = true;
			clearInterval(hitokotoTimer);
			hitokotoTimer = null;
		}
	}, 1000);
	
	function showHitokoto() {
		if (Math.random() < 0.6 && messageArray.length > 0)
		{
			showMessage(messageArray[Math.floor(Math.random() * messageArray.length)], 6000, 7);
		}
		else {
			var text = `好久都没有人送礼物了，快送个礼物逗我玩吧~`;
			setTimeout(() => {
				showMessage(text, 4000, 7);
			}, 6000);
		};
	}

	function initModel() {
		var modelId = localStorage.getItem("modelId"),
			modelTexturesId = localStorage.getItem("modelTexturesId");
		if (modelId == null) {
			//首次访问加载 指定模型 的 指定材质
			var modelId = 1, //模型 ID
				modelTexturesId = 53; //材质 ID
		}
		loadModel(modelId, modelTexturesId);
		$.getJSON(waifuPath, function(result) {
			$.each(result.seasons, function(index, tips) {
				var now = new Date(),
					after = tips.date.split("-")[0],
					before = tips.date.split("-")[1] || after;
				if ((after.split("/")[0] <= now.getMonth() + 1 && now.getMonth() + 1 <= before.split("/")[0]) && (after.split("/")[1] <= now.getDate() && now.getDate() <= before.split("/")[1])) {
					var text = Array.isArray(tips.text) ? tips.text[Math.floor(Math.random() * tips.text.length)] : tips.text;
					text = text.replace("{year}", now.getFullYear());
					//showMessage(text, 7000, true);
					messageArray.push(text);
				}
			});
		});
	}
	initModel();

	function loadModel(modelId, modelTexturesId) {
		localStorage.setItem("modelId", modelId);
		if (modelTexturesId === undefined) modelTexturesId = 0;
		localStorage.setItem("modelTexturesId", modelTexturesId);
		loadlive2d("live2d", `${apiPath}/get/?id=${modelId}-${modelTexturesId}`, console.log(`Live2D 模型 ${modelId}-${modelTexturesId} 加载完成`));
	}

	function loadRandModel() {
		var modelId = localStorage.getItem("modelId"),
			modelTexturesId = localStorage.getItem("modelTexturesId");
			//可选 "rand"(随机), "switch"(顺序)
		$.ajax({
			cache: false,
			url: `${apiPath}/rand_textures/?id=${modelId}-${modelTexturesId}`,
			dataType: "json",
			success: function(result) {
				if (result.textures["id"] == 1 && (modelTexturesId == 1 || modelTexturesId == 0)) showMessage("我还没有其他衣服呢！", 4000, 10);
				else showMessage("我的新衣服好看嘛？", 4000, 10);
				loadModel(modelId, result.textures["id"]);
			}
		});
	}

	function loadOtherModel() {
		var modelId = localStorage.getItem("modelId");
		$.ajax({
			cache: false,
			url: `${apiPath}/switch/?id=${modelId}`,
			dataType: "json",
			success: function(result) {
				loadModel(result.model["id"]);
				showMessage(result.model["message"], 4000, 10);
			}
		});
	}
}

function initWidget(waifuPath = "/waifu-tips.json", apiPath = "") {
	if (screen.width <= 768) return;
	$("body").append(`<div id="waifu-toggle" style="margin-left: -100px;">
			<span>看板娘</span>
		</div>`);
	$("#waifu-toggle").hover(() => {
		$("#waifu-toggle").animate({ "margin-left": -30 }, 500);
	}, () => {
		$("#waifu-toggle").animate({ "margin-left": -50 }, 500);
	}).click(() => {
		$("#waifu-toggle").animate({ "margin-left": -100 }, 1000, () => {
			$("#waifu-toggle").hide();
		});
		if ($("#waifu-toggle").attr("first-time")) {
			loadWidget(waifuPath, apiPath);
			$("#waifu-toggle").attr("first-time", false);
		} else {
			localStorage.removeItem("waifu-display");
			$("#waifu").show().animate({ bottom: 0 }, 0);
		}
	});
	if (localStorage.getItem("waifu-display") && new Date().getTime() - localStorage.getItem("waifu-display") <= 86400000) {
		$("#waifu-toggle").attr("first-time", true).css({ "margin-left": -50 });
	} else {
		loadWidget(waifuPath, apiPath);
	}
}

function showMessage(text, timeout, priority) {
	if (!text) return;
	if (!sessionStorage.getItem("waifu-text") || sessionStorage.getItem("waifu-text") <= priority) {
		if (messageTimer) {
			clearTimeout(messageTimer);
			messageTimer = null;
		}
		saying = true;
		if (Array.isArray(text)) text = text[Math.floor(Math.random() * text.length)];
		sessionStorage.setItem("waifu-text", priority);
		$("#waifu-tips").stop().html(text).fadeTo(200, 1);
		messageTimer = setTimeout(() => {
			sessionStorage.removeItem("waifu-text");
			$("#waifu-tips").fadeTo(1000, 0);
			saying = false;
		}, timeout);
	}
}

