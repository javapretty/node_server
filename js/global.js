/*
*	描述：全局变量管理器
*	作者：张亚磊
*	时间：2016/11/22
*/

require("js/enum.js");
require("js/message.js");
require("js/config.js");
require("js/util.js");
require("js/timer.js");
require("js/public_server/guild.js");
require("js/public_server/rank.js");

//全局变量类
var global = function () {};

//进程节点信息
global.node_info = new Object();
//配置管理器
global.config = new Config();
global.config.init();
//定时器
global.timer = new Timer();

///////////////////center_server变量////////////////////////
//sid idx
global.sid_idx = 0;
//sid set
global.sid_set = new Set();
//gate列表
global.gate_list = new Array();
//game列表
global.game_list = new Array();
//master cid列表
global.master_list = new Array();
//account--Token_Info
global.account_token_map = new Map();

///////////////////master_server变量////////////////////////
//node_id--master_cid
global.node_cid_map = new Map();
//node_id--node_status
global.node_status_map = new Map();

///////////////////gate_server变量////////////////////////
//node_id--game_cid
global.game_cid_map = new Map();
//client_cid--session
global.cid_session_map = new Map();
//sid--session
global.sid_session_map = new Map();
//account--session
global.account_session_map = new Map();

///////////////////game_server变量////////////////////////
//sid--Create_Role_Info
global.sid_login_map = new Map();
//sid--logout_time
global.sid_logout_map = new Map();
//sid----game_player
global.sid_game_player_map = new Map();
//role_id---game_player
global.role_id_game_player_map = new Map();
//role_name---game_player
global.role_name_game_player_map = new Map();

///////////////////public_server变量////////////////////////
//sid----public_player
global.sid_public_player_map = new Map();
//role_id---public_player
global.role_id_public_player_map = new Map();
//公会管理器
global.guild_manager = new Guild_Manager();
//排行榜管理器
global.rank_manager = new Rank_Manager();