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
	log_debug('master_server on_msg, cid:',msg.cid,' msg_type:',msg.msg_type,' msg_id:',msg.msg_id,' sid:', msg.sid);
	
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
	case Msg.HTTP_GET_NODE_STATUS:
	    //curl -d "{\"msg_id\":2,\"node_id\":0}" "http://127.0.0.1:8080" 
	   get_node_status(msg);
	   break;
	default:
		log_error('process_master_http_msg, msg_id not exist:', msg.msg_id);
		break;
	}
}

function get_node_status(msg) {
    var proc_info = get_proc_info();
    log_info("cpu_percent:",proc_info.cpu_percent," vm_size:",proc_info.vm_size," vm_rss:",proc_info.vm_rss,
        " vm_stk:", proc_info.vm_stk, " vm_exe:", proc_info.vm_exe, " vm_data:", proc_info.vm_data);

    var node_status = get_node_status();
    log_info("start_time:", node_status.start_time, " total_send:", node_status.total_send, " total_recv:", node_status.total_recv,
    " send_per_sec:", node_status.send_per_sec, " recv_per_sec:", node_status.recv_per_sec, " task_count:", node_status.task_count);
}