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
		} catch (err) {
			log_error(err.message);
		}
	}
}
