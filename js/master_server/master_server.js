/*
*	描述：master_server脚本
*	作者：张亚磊
*	时间：2016/10/27
*/

function init(node_info) {
    log_info('master_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
    global.node_info = node_info;
    global.timer.init();

    var msg = new node_2();
    msg.node_info = node_info;
    send_msg(Endpoint.MASTER_CENTER_CONNECTOR, 0, Msg.SYNC_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);
}

function on_hotupdate(file_path) { }

function on_drop(cid) { }

function on_msg(msg) {	
	if (msg.msg_type == Msg_Type.NODE_MSG) {
		process_master_node_msg(msg);
	} else if (msg.msg_type == Msg_Type.HTTP_MSG) {
		process_master_http_msg(msg);
	}
}

function on_tick(timer_id) {
    var timer_handler = global.timer.get_timer_handler(timer_id);
    if (timer_handler != null) {
        timer_handler();
    }
}

function process_master_node_msg(msg) {
	switch(msg.msg_id) {
	    case Msg.SYNC_NODE_INFO:
		    send_msg(Endpoint.MASTER_CENTER_CONNECTOR, 0, msg.msg_id, msg.msg_type, msg.sid, msg);
		    break;
	    case Msg.SYNC_NODE_STATUS: 
	        global.node_status_map.set(msg.node_status.node_id, msg.node_status);
	        break;
	    default:
		    log_error('process_master_node_msg, msg_id not exist:', msg.msg_id);
		    break;
	}
}

//可以使用curl命令，向服务器发送post消息
function process_master_http_msg(msg) {
	switch(msg.msg_id) {
	    case Msg.HTTP_CREATE_NODE_PROCESS:
	       //curl -d "{\"msg_id\":1,\"node_type\":7,\"node_id\":70003,\"endpoint_gid\":1,\"node_name\":\"game_server3\"}" "http://127.0.0.1:8080" 
		    fork_process(msg.node_type, msg.node_id, msg.endpoint_gid, msg.node_name);
		    break;
	    case Msg.HTTP_REQ_NODE_STATUS:
	  	     //curl -d "{\"msg_id\":2}" "http://127.0.0.1:8080"
	  	     req_node_status(msg);
	  	     break;
	    case Msg.HTTP_HOT_UPDATE:
	        //curl -d "{\"msg_id\":3,\"file_list\":[\"game_server/game_player.js\"]}" "http://127.0.0.1:8080"
	        hot_update(msg);
	        break;
	    default:
		    log_error('process_master_http_msg, msg_id not exist:', msg.msg_id);
		    break;
	}
}

function req_node_status(msg) {
    var msg_res = new http_201();
    for (var value of global.node_status_map.values()) {
        msg_res.node_list.push(value);
    }
	send_msg(Endpoint.MASTER_HTTP_SERVER, msg.cid, Msg.HTTP_RES_NODE_STATUS, msg.msg_type, 0, msg_res);
}

function hot_update(msg) {
    for(var i = 0; i < msg.file_list.length; ++i) {
        if (msg.file_list[i] == "global.js") {
            continue;
        }

        if (msg.file_list[i].indexOf(".js")) {
            require(msg.file_list[i]);
        }
        else if (msg.file_list[i].indexOf(".xml")) {

        }
    }
}