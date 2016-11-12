/*
*	描述：data_server脚本
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
	log_info('data_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
	config.init();
	timer.init(Node_Type.DATA_SERVER);	
	//init_db(node_info);
	//连接game数据库
	connect_db();
	
	//db_connector进程启动时候，向db_server进程同步自己信息
	if (node_info.endpoint_gid == 2) {
		var msg = new node_2();
		msg.node_info = node_info;
		send_msg(Endpoint.DB_CONNECTOR, 0, Msg.SYNC_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);
	}
}

function on_drop(cid) { }

function on_msg(msg) {
	log_debug('data_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id,' sid:', msg.sid);
}

function on_tick(timer_id) {}

function connect_db() {
	var node_info = config.node_json['node_info'];
	for(var i = 0; i < node_info.length; i++) {
		if(node_info[i]['node_type'] == Node_Type.DATA_SERVER) {
			var mysql_conf = node_info[i]['mysql_db'];
			if(mysql_conf != null){
				for(var j = 0; j < mysql_conf.length; j++) {
					var ret = connect_mysql(mysql_conf[j]['db_id'], mysql_conf[j]['ip'], mysql_conf[j]['port'], mysql_conf[j]['user'],
						mysql_conf[j]['password'], mysql_conf[j]['pool_name']);
					if(!ret) {
						log_error("connect to db ", mysql_conf['db_id'], ' error');
						return;
					}	
				}
			}
			
			var mongo_conf = node_info[i]['mongo_db'];
			if(mongo_conf != null) {
				for(var j = 0; j < mongo_conf.length; j++) {
					var ret = connect_mongo(mongo_conf[j]['db_id'], mongo_conf[j]['ip'], mongo_conf[j]['port']);
					if(!ret) {
						log_error("connect to db ", mongo_conf['db_id'], ' error');
						return;
					}	
				}
			}
		}
	}
}