服务器是异步非阻塞的多进程多线程架构，运行于CentOS7操作系统。服务器底层使用C++编写，高效稳定，
使用TCP协议进行进程间以及与客户端的通信。同时也支持UDP、Websocket、Http协议。
网络通信协议使用自定义格式，具备加密功能，安全高效。游戏数据库支持MongoDB和Mysql，可以选择自己想要的数据库。
逻辑层使用简单高效的Javascript进行开发，集成Google V8引擎解析脚本，保证脚本运行效率，有js经验的开发人员可以快速上手。

服务器分为DaemonServer，MasterServer，CenterServer，GateServer，GameServer，PublicServer，ChatServer，DBServer，LogServer。

DaemonServer：守护服务器，管理本机所有服务器进程的重启，通过进程id控制。
MasterServer：管理服务器，管理进程的启动关闭，记录进程的状态，然后将进程状态转给DaemonServer管理，可以动态增加进程。
CenterServer：中心服务器，管理gate列表，选择gate，验证客户端token，管理gate和game进程动态增减。
GateServer：网关服务器，维持客户端连接，消息转发，选择game功能。
GameServer：游戏服务器，处理玩家游戏逻辑，场景，AOI等功能。
PublicServer：公共服务器，处理公会，排行榜，拍卖行等公共游戏逻辑。
ChatServer：聊天服务器，处理聊天，开房间，发红包等功能。
DBServer：数据库服务器，提供在线玩家数据缓存和公共数据缓存，提交数据存取接口。
LogServer：日志服务器，游戏日志存储功能。

