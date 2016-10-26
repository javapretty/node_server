/*
*	描述：枚举类型
*	作者：张亚磊
*	时间：2016/02/24
*/

if (typeof Error_Code == "undefined") {
	var Error_Code = {};

	Error_Code.RET_OK								= 0;		//成功返回
	Error_Code.NEED_CREATE_ROLE			= 1;		//需要创建角色
	Error_Code.DISCONNECT_SELF 			= 2;		//服务主动关闭
	Error_Code.DISCONNECT_RELOGIN		= 3;		//账号重登录
	Error_Code.DISCONNECT_NOLOGIN 		= 4;		//账号没有登录
	Error_Code.TOKEN_NOT_EXIST				= 5;		//token不存在
	Error_Code.TOKEN_TIMEOUT				= 6;		//token到期
	Error_Code.CLIENT_PARAM_ERROR 		= 7;		//客户端参数错误
	Error_Code.CONFIG_ERROR 				= 8;		//配置文件错误
	Error_Code.ROLE_HAS_EXIST 				= 9;	//角色已存在
	Error_Code.ROLE_NOT_EXIST 				= 10;	//角色不存在
	Error_Code.PLAYER_SAVE_COMPLETE	= 11;	//保存玩家完成
	Error_Code.PLAYER_KICK_OFF 			= 12;	//玩家被踢下线
	Error_Code.PLAYER_OFFLINE 				= 13;	//玩家已离线
	Error_Code.LEVEL_NOT_ENOUGH 			= 14;	//角色等级不足
	
	Error_Code.BAG_FULL							= 15;	//背包已满
	Error_Code.ITEM_NOT_EXIST				= 16;	//物品不存在
	Error_Code.ITEM_NOT_ENOUGH				= 17;	//物品不足
	Error_Code.COPPER_NOT_ENOUGH			= 18;	//铜钱不足
	Error_Code.GOLD_NOT_ENOUGH				= 19;	//元宝不足
	
	Error_Code.GUILD_HAS_EXIST				= 20;	//公会已存在
	Error_Code.GUILD_NOT_EXIST				= 21;	//公会不存在
}

if (typeof Msg_Type == "undefined") {
	var Msg_Type = {};
	Msg_Type.C2S		= 1;	//客户端发到服务器的消息
	Msg_Type.S2C		= 2;	//服务器发到客户端的消息
	Msg_Type.NODE_C2S	= 3;	//客户端经过gate中转发到后端服务器的消息
	Msg_Type.NODE_S2C	= 4;	//后端服务器经过gate中转发到gate的消息
	Msg_Type.NODE_MSG	= 5;	//服务器进程节点间通信的消息
}

if (typeof Node_Type == "undefined") {
	var Node_Type = {};
	Node_Type.DB_SERVER		= 1;
	Node_Type.LOG_SERVER	= 2;
	Node_Type.CENTER_SERVER	= 3;
	Node_Type.PUBLIC_SERVER	= 4;
	Node_Type.GAME_SERVER	= 5;
	Node_Type.GATE_SERVER	= 6;
}

if (typeof Node_Id == "undefined") {
	var Node_Id = {};
	Node_Id.DB_SERVER		= 10001;
	Node_Id.LOG_SERVER	= 20001;
	Node_Id.CENTER_SERVER	= 30001;
	Node_Id.PUBLIC_SERVER	= 40001;
	Node_Id.GAME_SERVER1	= 50001;
	Node_Id.GAME_SERVER2	= 50002;
	Node_Id.GATE_SERVER1	= 60001;
	Node_Id.GATE_SERVER2	= 60002;
}

if (typeof Endpoint == "undefined") {
	var Endpoint = {};

	Endpoint.DB_SERVER				= 1001;
	Endpoint.LOG_SERVER				= 2001;
		
	Endpoint.CENTER_CLIENT_SERVER	= 3001;
	Endpoint.CENTER_SERVER			= 3002;
	
	Endpoint.PUBLIC_SERVER			= 4001;
	Endpoint.PUBLIC_HTTP_SERVER		= 4002;
	Endpoint.PUBLIC_LOG_CONNECTOR	= 4003;
	Endpoint.PUBLIC_DB_CONNECTOR	= 4004;
	
	Endpoint.GAME_SERVER			= 5001;
	Endpoint.GAME_CENTER_CONNECTOR 	= 5002;
	Endpoint.GAME_LOG_CONNECTOR		= 5003;
	Endpoint.GAME_DB_CONNECTOR		= 5004;
	Endpoint.GAME_PUBLIC_CONNECTOR	= 5005;
	
	Endpoint.GATE_CLIENT_SERVER 	= 6001;
	Endpoint.GATE_CENTER_CONNECTOR 	= 6002;
	Endpoint.GATE_PUBLIC_CONNECTOR	= 6003;
	Endpoint.GATE_GAME1_CONNECTOR	= 6004;
	Endpoint.GATE_GAME2_CONNECTOR	= 6005;
}

if (typeof Public_Data_Type == "undefined") {
	var Public_Data_Type = {};
	Public_Data_Type.ALL_DATA			= 0; 	//所有数据
	Public_Data_Type.GUILD_DATA			= 1; 	//公会数据
	Public_Data_Type.CREATE_GUILD_DATA	= 2;	//创建公会数据
	Public_Data_Type.RANK_DATA 			= 3;	//排行榜数据
}

if (typeof Rank_Type == "undefined") {
	var Rank_Type = {};
	Rank_Type.LEVEL_RANK	= 1;	//等级排行
	Rank_Type.COMBAT_RANK 	= 2;	//战力排行
}

if (typeof DB_Id == "undefined") {
	var DB_Id = {};
	DB_Id.GAME		= 1001;	
	DB_Id.LOG 		= 1002;
}
