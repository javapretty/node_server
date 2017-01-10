/*
*	描述：枚举类型
*	作者：张亚磊
*	时间：2016/02/24
*/

if (typeof Error_Code == "undefined") {
	var Error_Code = {};

	Error_Code.RET_OK				= 0;	//成功返回	
	Error_Code.DISCONNECT_SELF 		= 1;	//服务主动关闭
	Error_Code.DISCONNECT_RELOGIN	= 2;	//账号重登录
	Error_Code.DISCONNECT_NOLOGIN 	= 3;	//账号没有登录
	Error_Code.TOKEN_ERROR			= 4;	//token错误
	Error_Code.TOKEN_TIMEOUT		= 5;	//token到期
	Error_Code.CLIENT_PARAM_ERROR 	= 6;	//客户端参数错误
	Error_Code.CONFIG_ERROR 		= 7;	//配置文件错误
	Error_Code.GENERATE_ID_ERROR 	= 8;	//生成id错误
	Error_Code.PLAYER_DATA_ERROR	= 9;	//玩家数据错误
	Error_Code.ROLE_NOT_EXIST 		= 10;	//角色不存在
	Error_Code.ROLE_HAS_EXIST 		= 11;	//角色已存在
	Error_Code.ROLE_OFFLINE 		= 12;	//角色已离线
	Error_Code.LEVEL_NOT_ENOUGH 	= 13;	//角色等级不足
	
	Error_Code.BAG_FULL				= 14;	//背包已满
	Error_Code.ITEM_NOT_EXIST		= 15;	//物品不存在
	Error_Code.ITEM_NOT_ENOUGH		= 16;	//物品不足
	Error_Code.GOLD_NOT_ENOUGH		= 17;	//金币不足
	Error_Code.DIAMOND_NOT_ENOUGH	= 18;	//钻石不足
	
	Error_Code.GUILD_HAS_EXIST		= 19;	//公会已存在
	Error_Code.GUILD_NOT_EXIST		= 20;	//公会不存在

	Error_Code.AWARD_GET_ALREADY	= 21;	//奖励已经领取
}

if (typeof Msg_Type == "undefined") {
	var Msg_Type = {};
	Msg_Type.TCP_C2S	= 1;	//客户端发到服务器的消息
	Msg_Type.TCP_S2C	= 2;	//服务器发到客户端的消息
	Msg_Type.NODE_C2S   = 3;	//客户端经过gate中转发到服务器的消息
	Msg_Type.NODE_S2C   = 4;	//服务器经过gate中转发到客户端的消息
	Msg_Type.NODE_MSG   = 5;	//服务器进程节点间通信的消息
	Msg_Type.DATA_MSG   = 6;    //经过data中转发到data子进程的消息
	Msg_Type.HTTP_MSG	= 7;	//http消息
	Msg_Type.WS_C2S		= 8;	//websocket客户端消息
	Msg_Type.WS_S2C		= 9;	//websocket服务器消息
}

if (typeof Node_Type == "undefined") {
	var Node_Type = {};
	Node_Type.CENTER_SERVER = 1;
	Node_Type.GATE_SERVER 	= 2;
	Node_Type.DATA_SERVER	= 3;
	Node_Type.LOG_SERVER	= 4;
	Node_Type.MASTER_SERVER	= 5;
	Node_Type.PUBLIC_SERVER	= 6;
	Node_Type.GAME_SERVER	= 7;
}

if (typeof Endpoint_Type == "undefined") {
	var Endpoint_Type = {};
	Endpoint_Type.CLIENT_SERVER = 1;	//接受客户端链接的server
	Endpoint_Type.SERVER 	= 2;		//接受内部节点连接的server
	Endpoint_Type.CONNECTOR	= 3;		//内部节点链接器
}

if (typeof Endpoint == "undefined") {
	var Endpoint = {};

	Endpoint.CENTER_CLIENT_SERVER	= 1;
	Endpoint.CENTER_NODE_SERVER		= 2;
	
	Endpoint.GATE_CLIENT_SERVER 	= 1;
	Endpoint.GATE_NODE_SERVER 		= 2;
	Endpoint.GATE_CENTER_CONNECTOR  = 3;
	Endpoint.GATE_MASTER_CONNECTOR  = 4;
	
	Endpoint.DATA_SERVER            = 1;
	Endpoint.DATA_MASTER_CONNECTOR  = 2;
	Endpoint.DATA_CONNECTOR         = 1;
	
	Endpoint.LOG_SERVER				= 1;
	Endpoint.LOG_MASTER_CONNECTOR   = 2;
	Endpoint.LOG_CONNECTOR          = 1;
	
	Endpoint.MASTER_SERVER			= 1;
	Endpoint.MASTER_HTTP_SERVER		= 2;
	Endpoint.MASTER_CENTER_CONNECTOR= 3;
	
	Endpoint.PUBLIC_SERVER			= 1;
	Endpoint.PUBLIC_DATA_CONNECTOR	= 2;
	Endpoint.PUBLIC_LOG_CONNECTOR   = 3;
	Endpoint.PUBLIC_MASTER_CONNECTOR= 4;
	
	Endpoint.GAME_DATA_CONNECTOR	= 1;
	Endpoint.GAME_LOG_CONNECTOR		= 2;
	Endpoint.GAME_MASTER_CONNECTOR	= 3;
	Endpoint.GAME_PUBLIC_CONNECTOR	= 4;
	Endpoint.GAME_GATE1_CONNECTOR	= 5;
	Endpoint.GAME_GATE2_CONNECTOR	= 6;
}

if (typeof Select_Data_Type == "undefined") {
    var Select_Data_Type = {};
    Select_Data_Type.INT64 = 1;     //查询int64类型值
    Select_Data_Type.STRING = 2;    //查询string类型值
    Select_Data_Type.ACCOUNT = 3;   //查询帐号信息
}

if (typeof DB_Data_Type == "undefined") {
	var DB_Data_Type = {};
	DB_Data_Type.ACCOUNT	= 1;    //账号数据
	DB_Data_Type.PLAYER		= 2;    //玩家数据
	DB_Data_Type.GUILD		= 3;    //公会数据
	DB_Data_Type.RANK 		= 4;    //排行榜数据
	DB_Data_Type.LOGOUT 	= 5;    //离线日志数据
}

if (typeof DB_Id == "undefined") {
	var DB_Id = {};
	DB_Id.GAME	= 1001;	
	DB_Id.LOG 	= 1002;
}

if (typeof DB_Ret_Code == "undefined") {
    var DB_Ret_Code = {};
    DB_Ret_Code.RET_OK = 0;
    DB_Ret_Code.OPT_FAIL = -1;
    DB_Ret_Code.DATA_NOT_EXIST = -2;
}

if (typeof Save_Type == "undefined") {
	var Save_Type = {};
	Save_Type.SAVE_CACHE 		  = 1;    //保存缓存
	Save_Type.SAVE_DB_AND_CACHE   = 2;    //保存数据库和缓存
	Save_Type.SAVE_DB_CLEAR_CACHE = 3;    //保存数据库清空缓存
}

if (typeof Rank_Type == "undefined") {
	var Rank_Type = {};
	Rank_Type.LEVEL_RANK	= 1;    //等级排行
	Rank_Type.COMBAT_RANK 	= 2;    //战力排行
}
