/*
*	描述：data_server脚本
*	作者：张亚磊
*	时间：2016/09/22
*/

function init(node_info) {
    log_info('data_server init, node_type:', node_info.node_type, ' node_id:', node_info.node_id, ' node_name:', node_info.node_name);
    global.node_info = node_info;
    global.timer.init();

    for(var i = 0; i < node_info.endpoint_list.length; ++i) {
        if (node_info.endpoint_list[i].endpoint_type == Endpoint_Type.CONNECTOR) {
            util.sync_node_info(node_info.endpoint_list[i].endpoint_id);
        }
    }
}

function on_hotupdate(file_path) { }

function on_drop_eid(eid) {
    util.sync_node_info(eid);
}

function on_drop_cid(cid) { }

function on_msg(msg) { }

function on_tick(timer_id) {
    var timer_handler = global.timer.get_timer_handler(timer_id);
    if (timer_handler != null) {
        timer_handler();
    }
}