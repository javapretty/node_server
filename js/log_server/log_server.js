/*
*	描述：log_server脚本
*	作者：张亚磊
*	时间：2016/09/22
*/

function init(node_info) {
    log_info('log_server init, node_type:', node_info.node_type, ' node_id:', node_info.node_id, ' node_name:', node_info.node_name);
    global.node_info = node_info;
    global.timer.init();

    var msg = new Object();
    msg.node_info = node_info;
    for(var i = 0; i < node_info.endpoint_list.length; ++i) {
        if(node_info.endpoint_list[i].endpoint_type == Endpoint_Type.CONNECTOR) {
            send_msg(node_info.endpoint_list[i].endpoint_id, 0, Msg.SYNC_NODE_INFO, Msg_Type.NODE_MSG, 0, msg);     
        }
    }
}

function on_hotupdate(file_path) { }

function on_drop(drop_id) {}

function on_msg(msg) {}

function on_tick(timer_id) {
    var timer_handler = global.timer.get_timer_handler(timer_id);
    if (timer_handler != null) {
        timer_handler();
    }
}