/*
*	描述：session类
*	作者：张亚磊
*	时间：2016/02/24
*/

function Session() {
	this.gate_endpoint = 0;		//client连接的gate endpoint_id
	this.game_endpoint = 0;		//与game_server连接的endpoint_id
	this.public_endpoint = 0;	//与public_server连接的endpoint_id
	this.cid = 0;							//client与gate连接的cid
	this.sid = 0;							//gate生成的全局唯一session_id
	this.account = "";				//client帐号名
	this.last_hb_time = 0;		//上次心跳时间
  	this.latency = 0;					//上次心跳到本次心跳经过的时间
}

Session.prototype.on_heartbeat = function(msg) {
	this.last_hb_time = util.now_sec();
	this.latency = this.last_hb_time - msg.client_time;
	if (this.latency < 0) this.latency = 0;
	var msg_res = new s2c_0();
	msg_res.server_time = this.last_hb_time;
	send_msg(Endpoint.GATE_CLIENT_SERVER, msg.cid, Msg.RES_HEARTBEAT, Msg_Type.S2C, 0, msg_res);
}