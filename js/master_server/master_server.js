/*
*	描述：master_server脚本
*	作者：张亚磊
*	时间：2016/10/27
*/

function init(node_info) {
	log_info('master_server init, node_type:',node_info.node_type,' node_id:',node_info.node_id,' node_name:',node_info.node_name);
}

function on_hotupdate(file_path) { }

function on_drop(cid) { }

function on_msg(msg) {
	log_debug('master_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id);
	
	if (msg.msg_type == Msg_Type.NODE_MSG) {
		process_master_node_msg(msg);
	} else if (msg.msg_type == Msg_Type.HTTP_MSG) {
		process_master_http_msg(msg);
	}
}

function on_tick(timer_id) {}

function process_master_node_msg(msg) {
	switch(msg.msg_id) {
	case Msg.SYNC_NODE_INFO:
		send_msg(Endpoint.MASTER_CENTER_CONNECTOR, 0, msg.msg_id, msg.msg_type, msg.sid, msg);
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
	    //curl -d "{\"msg_id\":4,\"file_list\":[\"game_server/game_player.js\"]}" "http://127.0.0.1:8080"
	    hot_update(msg);
	    break;
	default:
		log_error('process_master_http_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}

function req_node_status(msg) {
    var proc_info = get_proc_info();
    log_info("cpu_percent:",proc_info.cpu_percent," vm_size:",proc_info.vm_size," vm_rss:",proc_info.vm_rss,
        " vm_stk:", proc_info.vm_stk, " vm_exe:", proc_info.vm_exe, " vm_data:", proc_info.vm_data);

    var node_status = get_node_status();
    log_info("start_time:", node_status.start_time, " total_send:", node_status.total_send, " total_recv:", node_status.total_recv,
    " send_per_sec:", node_status.send_per_sec, " recv_per_sec:", node_status.recv_per_sec, " task_count:", node_status.task_count);
    
    var node_status = new Node_Status();
    node_status.cpu_percent = proc_info.cpu_percent;
    node_status.vm_size = proc_info.vm_size;
    node_status.vm_rss = proc_info.vm_rss;
    node_status.vm_stk = proc_info.vm_stk;
    node_status.vm_exe = proc_info.vm_exe;
    node_status.vm_data = proc_info.vm_data;
    node_status.start_time = node_status.start_time;
    node_status.total_send = node_status.total_send;
    node_status.total_recv = node_status.total_recv;
    node_status.send_per_sec = node_status.send_per_sec;
    node_status.recv_per_sec = node_status.recv_per_sec;
    node_status.task_count = node_status.task_count;
    var msg_res = new http_3();
    msg_res.node_list.push(node_status);
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