/*
*	描述：master_server脚本
*	作者：张亚磊
*	时间：2016/10/27
*/

function init(node_info) {
    log_info('master_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
    global.node_info = node_info;
    global.timer.init();

    for(var i = 0; i < node_info.endpoint_list.length; ++i) {
    	if(node_info.endpoint_list[i].endpoint_type == Endpoint_Type.CONNECTOR) {
    	    util.sync_node_info(node_info.endpoint_list[i].endpoint_id);
    	}
    }    
}

function on_hotupdate(file_path) { }

function on_drop_eid(eid) { 
    util.sync_node_info(eid);
}

function on_drop_cid(cid) { }

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
	        global.node_cid_map.set(msg.node_info.node_id, msg.cid);
		    send_msg(Endpoint.MASTER_CENTER_CONNECTOR, 0, msg.msg_id, msg.msg_type, msg.sid, msg);
		    break;
	    case Msg.SYNC_NODE_STATUS: 
	        global.node_status_map.set(msg.node_status.node_id, msg.node_status);
	        break;
	    case Msg.SYNC_NODE_STACK_INFO:
	        send_msg(Endpoint.MASTER_HTTP_SERVER, msg.sid, Msg.HTTP_RES_STACK_INFO, Msg_Type.HTTP_MSG, 0, msg);
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
	        close_client(Endpoint.MASTER_HTTP_SERVER, msg.cid);
		    break;
	    case Msg.HTTP_REQ_NODE_STATUS:
	  	     //curl -d "{\"msg_id\":2}" "http://127.0.0.1:8080"
	  	     req_node_status(msg);
	  	     break;
	    case Msg.HTTP_REQ_STACK_INFO:
	        //curl -d "{\"msg_id\":3,\"node_id\":70001}" "http://127.0.0.1:8080"
	        req_stack_info(msg);
	        break;
	    case Msg.HTTP_HOT_UPDATE:
	        //curl -d "{\"msg_id\":4,\"node_id\":70001,\"file_path\":\"js/game_server/game_player.js\"}" "http://127.0.0.1:8080"
	        hot_update(msg);
	        break;
	    default:
		    log_error('process_master_http_msg, msg_id not exist:', msg.msg_id);
		    break;
	}
}

function req_node_status(msg) {
    var msg_res = new Object();
    msg_res.node_list = new Array();
    global.node_status_map.forEach(function(value, key, map) {
        msg_res.node_list.push(value);
    });
	send_msg(Endpoint.MASTER_HTTP_SERVER, msg.cid, Msg.HTTP_RES_NODE_STATUS, msg.msg_type, 0, msg_res);
}

function req_stack_info(msg) {
    var eid = 0;
    var node_type = parseInt(msg.node_id / 10000);
    switch(node_type) {
        case Node_Type.GATE_SERVER:
            eid = Endpoint.GATE_MASTER_CONNECTOR;
            break;
        case Node_Type.DATA_SERVER:
            eid = Endpoint.DATA_MASTER_CONNECTOR;
            break;
        case Node_Type.LOG_SERVER:
            eid = Endpoint.LOG_MASTER_CONNECTOR;
            break;
        case Node_Type.PUBLIC_SERVER:
            eid = Endpoint.PUBLIC_MASTER_CONNECTOR;
            break;
        case Node_Type.GAME_SERVER:
            eid = Endpoint.GAME_MASTER_CONNECTOR;
            break;
        default:
            break;
    }

    var node_status = global.node_status_map.get(msg.node_id);
    if(eid > 0 && node_status != null) {
        get_node_stack(msg.node_id, eid, 0, msg.cid);
    }
    else {
        log_error("node_id error:", msg.node_id);
        close_client(Endpoint.MASTER_HTTP_SERVER, msg.cid);
    }
}

function hot_update(msg) {
    if (msg.file_path == "global.js") {
        close_client(Endpoint.MASTER_HTTP_SERVER, msg.cid);
        return;
    }

    if (msg.file_path.indexOf(".js")) {
        require(msg.file_path);
    }
    else if (msg.file_path.indexOf(".xml")) {

    }
}