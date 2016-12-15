/*
*	描述：基础函数类
*	作者：张亚磊
*	时间：2016/03/15
*/

var util = function () {};

//一天的秒数
util.whole_day_sec = 60 * 60 * 24;
//一天的毫秒数
util.whole_day_msec = util.whole_day_sec * 1000;

//获取当前时间戳(秒单位)
util.now_sec = function() {
	return Math.round(new Date().getTime()/1000);
}

//获取当前时间戳(毫秒单位)
util.now_msec = function(date) {
	return new Date().getTime(); 
}

//获取当前时间到指定定时时间的间隔(每日)
util.get_next_day_tick = function(hour, min = 0, sec = 0) {
	var now = util.now_sec();
	var date = new Date();
	date.setHours(hour);
	date.setMinutes(min);
	date.setSeconds(sec);
	date.setMilliseconds(0);
	var time = date.getTime() / 1000;
	var next_tick = time - now;
	return next_tick > 0 ? next_tick : (next_tick + util.whole_day_sec);
}

util.get_node_status = function() {
    var node_status = get_node_status();
    var proc_info = get_proc_info();
    var heap_info = get_heap_info();
    var all_node_status = new Object();
    //进程状态信息
    all_node_status.node_type = node_status.node_type;
    all_node_status.node_id = node_status.node_id;
    all_node_status.node_name = node_status.node_name;
    all_node_status.start_time = node_status.start_time;
    all_node_status.total_send = node_status.total_send;
    all_node_status.total_recv = node_status.total_recv;
    all_node_status.send_per_sec = node_status.send_per_sec;
    all_node_status.recv_per_sec = node_status.recv_per_sec;
    all_node_status.task_count = node_status.task_count;
    //获取cpu内存信息
    all_node_status.cpu_percent = proc_info.cpu_percent;
    all_node_status.vm_size = proc_info.vm_size;
    all_node_status.vm_rss = proc_info.vm_rss;
    all_node_status.vm_data = proc_info.vm_data;
    //获取v8堆信息
    all_node_status.heap_total = heap_info.heap_total;
    all_node_status.heap_used = heap_info.heap_used;
    all_node_status.external_mem = heap_info.external_mem;
    return all_node_status;
}

util.sync_node_status = function(eid, cid, session_count) {
    var msg = new Object();
    msg.node_status = util.get_node_status();
    msg.node_status.session_count = session_count;
    send_msg(eid, cid, Msg.SYNC_NODE_STATUS, Msg_Type.NODE_MSG, 0, msg);
}