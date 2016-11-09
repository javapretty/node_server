/*
*	描述：log_server脚本
*	作者：张亚磊
*	时间：2016/09/22
*/

require('enum.js');
require('message.js');
require('struct.js');
require('config.js');
require('util.js');
require('timer.js');

//配置管理器
var config = new Config();
//定时器
var timer = new Timer();

function init(node_info) {
	log_info('log_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
	config.init();
	timer.init(Node_Type.LOG_SERVER);
	init_log(node_info);	
	//连接log数据库
	connect_db();
	
	//log_connector进程启动时候，向log_server进程同步自己信息
	if (node_info.endpoint_gid == 2) {
		var msg = new node_2();
		msg.node_info = node_info;
		send_msg(Endpoint.LOG_CONNECTOR, 0, Msg.SYNC_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);
	}
}

function on_drop(drop_id) {}

function on_msg(msg) {	
	switch(msg.msg_id) {
	case Msg.SYNC_LOG_PLAYER_LOGOUT:
		log_player_logout(msg);
		break;
	default:
		log_error('log_server on_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}

function on_tick(timer_id) {}

function connect_db() {
	var node_info = config.node_json['node_info'];
	for(var i = 0; i < node_info.length; i++) {
		if(node_info[i]['node_type'] == Node_Type.LOG_SERVER){
			var mysql_conf = node_info[i]['mysql_db'];
			if (mysql_conf == null)
				return;
				
			for(var j = 0; j < mysql_conf.length; j++) {
				var ret = connect_mysql(mysql_conf[j]['db_id'], mysql_conf[j]['ip'], mysql_conf[j]['port'], mysql_conf[j]['user'],
						mysql_conf[j]['password'], mysql_conf[j]['pool_name']);
				if(!ret) {
					return log_error("connect to db ", mysql_conf['db_id'], ' error');
				}	
			}
		}
	}
}

function log_player_logout(msg) {
	save_db_data(Save_Flag.SAVE_CACHE_DB, DB_Id.LOG, "log.logout", msg.logout_info);
}
