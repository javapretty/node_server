/*
*	描述：配置文件
*	作者：张亚磊
*	时间：2016/03/24
*/

function Config() {
	this.node_json = null;			//进程节点配置
	this.item_json = null;			//道具配置
	this.level_json = null;			//玩家等级相关配置
	this.util_json = null;			//公共配置
	
	this.init = function() {
		try {
			var node_str = read_json("config/node/node_conf.json");
			this.node_json = JSON.parse(node_str);
			
			var item_str = read_json("config/bag/item.json");
			this.item_json = JSON.parse(item_str);
			
			var level_str = read_json("config/player/level.json");
			this.level_json = JSON.parse(level_str);
			
			var util_str = read_json("config/util/util.json");
			this.util_json = JSON.parse(util_str);
			
		} catch (err) {
			print(err.message);
		}
	}
}
