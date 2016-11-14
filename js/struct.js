
function Logout_Info() {
	this.role_id = 0;
	this.role_name = "";
	this.account = "";
	this.level = 0;
	this.exp = 0;
	this.gender = 0;
	this.career = 0;
	this.create_time = 0;
	this.login_time = 0;
	this.logout_time = 0;
}

function Rank_Info() {
	this.rank_type = 0;
	this.min_role_id = 0;
	this.min_value = 0;
	this.member_map = new Map();
}

function Mail_Info() {
	this.role_id = 0;
	this.total_count = 0;
	this.mail_map = new Map();
}

function Bag_Info() {
	this.role_id = 0;
	this.copper = 0;
	this.gold = 0;
	this.item_map = new Map();
}

function Idx_Info() {
	this.type = "";
	this.value = 0;
}

function Player_Data() {
	this.role_info = new Role_Info();
	this.mail_info = new Mail_Info();
	this.bag_info = new Bag_Info();
}

function Rank_Member_Detail() {
	this.role_id = 0;
	this.role_name = "";
	this.value = 0;
}

function Token_Info() {
	this.cid = 0;
	this.token = "";
	this.token_time = 0;
}

function Node_Info() {
	this.node_type = 0;
	this.node_id = 0;
	this.endpoint_gid = 0;
	this.max_session_count = 0;
	this.node_name = "";
	this.node_ip = "";
	this.endpoint_list = new Array();
}

function Endpoint_Info() {
	this.endpoint_type = 0;
	this.endpoint_gid = 0;
	this.endpoint_id = 0;
	this.endpoint_name = "";
	this.server_ip = "";
	this.server_port = 0;
	this.protocol_type = 0;
	this.receive_timeout = 0;
}

function Mail_Detail() {
	this.pickup = 0;
	this.mail_id = 0;
	this.send_time = 0;
	this.sender_type = 0;
	this.sender_id = 0;
	this.sender_name = "";
	this.mail_title = "";
	this.mail_content = "";
	this.copper = 0;
	this.gold = 0;
	this.item_info = new Array();
}

function Item_Info() {
	this.item_id = 0;
	this.amount = 0;
	this.bind_type = 0;
}

function Guild_Member_Detail() {
	this.role_id = 0;
	this.role_name = "";
	this.level = 0;
}

function Guild_Info() {
	this.guild_id = 0;
	this.guild_name = "";
	this.chief_id = 0;
	this.create_time = 0;
	this.member_list = new Array();
}

function Role_Info() {
	this.role_id = 0;
	this.role_name = "";
	this.account = "";
	this.level = 0;
	this.exp = 0;
	this.gender = 0;
	this.career = 0;
	this.create_time = 0;
	this.login_time = 0;
	this.logout_time = 0;
	this.guild_id = 0;
	this.guild_name = "";
}

function Create_Role_Info() {
	this.account = "";
	this.role_name = "";
	this.gender = 0;
	this.career = 0;
}

function http_1() {
	this.node_type = 0;
	this.node_id = 0;
	this.endpoint_gid = 0;
	this.node_name = "";
}

function node_255() {
	this.struct_name = "";
	this.key_index = 0;
}

function node_253() {
	this.struct_name = "";
	this.key_index = 0;
	this.data_type = 0;
}

function node_252() {
	this.db_id = 0;
	this.struct_name = "";
	this.index_list = new Array();
}

function node_251() {
	this.save_type = 0;
	this.vector_data = 0;
	this.db_id = 0;
	this.struct_name = "";
	this.player_data = new Player_Data();
	this.guild_list = new Array();
	this.rank_list = new Array();
	this.logout_info = new Logout_Info();
	this.data_type = 0;
}

function node_250() {
	this.db_id = 0;
	this.struct_name = "";
	this.key_index = 0;
	this.data_type = 0;
}

function node_249() {
	this.type = "";
	this.id = 0;
}

function node_248() {
	this.db_id = 0;
	this.table_name = "";
	this.type = "";
}

function node_247() {
	this.table_name = "";
	this.key_index = 0;
}

function node_246() {
	this.db_id = 0;
	this.table_name = "";
	this.index_name = "";
	this.query_name = "";
	this.query_value = "";
}

function node_8() {
	this.role_id = 0;
	this.guild_id = 0;
	this.guild_name = "";
}

function node_7() {
}

function node_6() {
	this.error_code = 0;
}

function node_5() {
	this.login = 0;
	this.role_info = new Role_Info();
}

function node_254() {
	this.struct_name = "";
	this.key_index = 0;
	this.data_type = 0;
}

function node_4() {
	this.gate_nid = 0;
}

function node_3() {
	this.account = "";
	this.token = "";
	this.client_cid = 0;
	this.game_nid = 0;
}

function node_2() {
	this.node_info = new Node_Info();
}

function node_1() {
	this.node_code = 0;
}

function s2c_202() {
	this.guild_id = 0;
}

function c2s_202() {
}

function s2c_201() {
	this.guild_info = new Guild_Info();
}

function c2s_201() {
	this.guild_name = "";
}

function s2c_9() {
	this.item_list = new Array();
}

function c2s_9() {
}

function s2c_8() {
	this.mail_id_list = new Array();
}

function c2s_8() {
	this.mail_id = 0;
}

function s2c_7() {
	this.mail_id_list = new Array();
}

function c2s_7() {
	this.mail_id = 0;
}

function s2c_6() {
	this.mail_list = new Array();
}

function c2s_6() {
}

function s2c_5() {
	this.error_code = 0;
}

function c2s_5() {
	this.role_info = new Create_Role_Info();
}

function s2c_4() {
	this.role_info = new Role_Info();
}

function c2s_4() {
	this.account = "";
}

function s2c_3() {
}

function c2s_3() {
	this.account = "";
	this.token = "";
}

function s2c_2() {
	this.gate_ip = "";
	this.gate_port = 0;
	this.token = "";
}

function c2s_2() {
	this.account = "";
}

function s2c_1() {
	this.server_time = 0;
}

function c2s_1() {
	this.client_time = 0;
}
