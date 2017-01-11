
function Logout_Info() {
	this.role_id = 0;
	this.role_name = "";
	this.account = "";
	this.gender = 0;
	this.career = 0;
	this.level = 0;
	this.exp = 0;
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
	this.item_map = new Map();
}

function Activity_Info() {
	this.role_id = 0;
	this.seven_day_start_time = 0;
	this.seven_day_award_status = new Array();
	this.sign_in_award_status = new Array();
}

function Role_Info() {
	this.role_id = 0;
	this.role_name = "";
	this.account = "";
	this.gender = 0;
	this.career = 0;
	this.level = 0;
	this.exp = 0;
	this.combat = 0;
	this.create_time = 0;
	this.login_time = 0;
	this.logout_time = 0;
	this.gold = 0;
	this.diamond = 0;
	this.guild_id = 0;
	this.guild_name = "";
	this.speed = 0;
	this.last_scene = 0;
	this.last_x = 0;
	this.last_y = 0;
}

function Account_Info() {
	this.account = "";
	this.role_list = new Array();
}

function Idx_Info() {
	this.type = "";
	this.value = 0;
}

function Player_Data() {
	this.role_info = new Role_Info();
	this.activity_info = new Activity_Info();
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
	this.heartbeat_timeout = 0;
}

function Node_Status() {
	this.node_type = 0;
	this.node_id = 0;
	this.node_name = "";
	this.start_time = 0;
	this.total_send = 0;
	this.total_recv = 0;
	this.send_per_sec = 0;
	this.recv_per_sec = 0;
	this.task_count = 0;
	this.msg_list = new Array();
	this.session_count = 0;
	this.cpu_percent = 0;
	this.vm_size = 0;
	this.vm_rss = 0;
	this.vm_data = 0;
	this.heap_total = 0;
	this.heap_used = 0;
	this.external_mem = 0;
}

function Msg_Count() {
	this.msg_id = 0;
	this.msg_count = 0;
}

function Mail_Detail() {
	this.pickup = false;
	this.mail_id = 0;
	this.send_time = 0;
	this.sender_type = 0;
	this.sender_id = 0;
	this.sender_name = "";
	this.mail_title = "";
	this.mail_content = "";
	this.gold = 0;
	this.diamond = 0;
	this.item_list = new Array();
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

function Create_Role_Info() {
	this.account = "";
	this.role_name = "";
	this.gender = 0;
	this.career = 0;
}

function Account_Role_Info() {
	this.role_id = 0;
	this.role_name = "";
	this.gender = 0;
	this.career = 0;
	this.level = 0;
	this.combat = 0;
}

function Position() {
	this.x = 0;
	this.y = 0;
}
