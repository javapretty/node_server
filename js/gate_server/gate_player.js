/*
*	描述：gate_player类
*	作者：张亚磊
*	时间：2016/02/24
*/

function Gate_Player() {
	this.game_endpoint = 0;
	this.cid = 0;
	this.account = "";
  this.last_hb_time = util.now_sec();
  this.latency = 0;
}

Gate_Player.prototype.on_heartbeat = function(msg) {
	this.last_hb_time = util.now_sec();
	this.latency = this.last_hb_time - msg.client_time;
	if (this.latency < 0) this.latency = 0;
	var msg_res = new s2c_0();
	msg_res.server_time = this.last_hb_time;
	send_msg(Endpoint.GATE_CLIENT_SERVER, msg.cid, Msg.RES_HEARTBEAT, Msg_Type.S2C, 0, msg_res);
}