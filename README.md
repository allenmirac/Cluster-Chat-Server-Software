# Cluster-Chat-Server
集群聊天服务器--软件分层设计和高性能服务开发，结合了网络编程、数据库设计和并发处理等核心技术。

## 项目核心组件与技术栈
1. 网络层（muduo库）
* 角色：负责处理所有客户端的连接、数据收发和解包封包。
* 关键点：
    * Reactor模型：muduo是主从Reactor多线程模型。主Reactor（通常一个线程）负责接收新连接，然后将连接分发给子Reactor（多个线程）进行IO读写。这已经支持了高并发。
    * 线程安全：需要特别注意跨线程调用和数据共享。muduo通过`ventLoop::runInLoop()` 等机制确保任务在正确的线程中执行。
    * 消息协议：自定义应用层协议。
    * 数据序列化：使用`nlohmann/json`格式。

2. 业务逻辑层 (核心服务)
实现了多种服务。
* 用户服务：
    * 功能：注册、登录、注销、查询用户信息。
    * 密码需要实现为加盐哈希处理。
* 好友服务：
    * 功能：添加好友、删除好友、获取好友列表、查询好友状态（在线/离线）。
    * 关键：这是一个多对多关系。需要在数据库中建立好友关系表。（实现好友请求确认功能？）
* 群组服务：
    * 功能：创建群组、解散群组、加入群组、退出群组、获取群成员列表、指定管理员等。
    * 关键：这是一个一对多和多对多的混合关系。关系表中可以包含成员角色（创建者、普通成员）等字段。
* 聊天服务：
    * 一对一聊天（P2P）:
        * 流程：用户A发送消息给服务器 -> 服务器查询用户B的连接信息 -> 通过B的连接将消息转发出去。
        * 关键：维护一个 用户ID 到 其连接的`TcpConnectionPtr`对象 的映射。这个映射必须是线程安全的（用`std::unordered_map + std::mutex` ）。
    * 群组聊天（Group）：
        * 流程：用户A在群G中发送消息 -> 服务器查询群G的所有成员ID -> 遍历成员ID，找到在线的成员连接 -> 将消息逐个转发。
        * 关键：需要考虑性能，特别是大群（几百人）的消息风暴？

3. 数据持久层（MySQL）
* 角色：持久化所有状态数据。用户信息、好友关系、群组信息、聊天记录（如果需要离线消息和漫游）等。
* 数据库连接：使用数据库连接池，默认使用8个连接，因为每个IO线程都不能同步等待数据库查询。工作线程从池中获取一个连接，执行操作后迅速归还。

4. 架构图（逻辑视图）

```test
+---------------------------------------------------+
|                  Client (App/Web)                 |
+--------------------------+------------------------+
                           |
+--------------------------|------------------------+
|         muduo Network Layer (TcpServer)           |
|  +-----------------+-----------------------------+|
|  | Main Reactor    |     Sub Reactors (Threads)  ||
|  | (Acceptor)      | (IO Thread 1, IO Thread 2...)|
|  +-----------------+-----------------------------+|
+--------------------------|------------------------+
                           |
+--------------------------|------------------------+
|               ChatService / Business Logic        |
|  +--------+ +--------+ +--------+ +------------+  |
|  | User   | | Friend | | Group  | | Chat       |  |
|  | Service| | Service| | Service| | Service    |  |
|  +--------+ +--------+ +--------+ +------------+  |
+--------------------------|------------------------+
                           |
+--------------------------|------------------------+
|               Database Connection Pool            |
+--------------------------|------------------------+
                           |
+--------------------------|------------------------+
|                     MySQL Database                |
+---------------------------------------------------+
```

5. 解析典型工作流程：用户A发送一条消息给用户B

* 消息接收：用户A的TCP连接所在的IO线程收到二进制数据流。
* 解包：根据预设的协议（JSON格式），解析出完整的消息包和消息类型（`ONE_CHAT_MSG`）。
* 分发处理：IO线程调用`ChatService`中对应的消息类型处理器（`ChatService::oneChat`）。
* 业务处理：
    * 查询用户B是否是否在线（查`userConnMap_`）
    * 如果在线，通过获取到的`TcpConnectionPtr`，通过`conn->send()`直接发送消息。
    * 如果不在线，将消息存储在OfflineMessage表中。

## 项目运行环境

- **系统**：Ubuntu 22.04 (推荐虚拟机/WSL 环境)
- **编译器**：gcc (Ubuntu 11.4.0-1ubuntu1~22.04.2) 11.4.0
- **CMake**：cmake version 3.22.1
- **Boost 库**：需要提前安装
- **MySQL**：mysql  Ver 14.14 Distrib 5.7.36, for Linux (x86_64) using  EditLine wrapper
- **Muduo 网络库**：需要提前安装

## 安装依赖
1. 安装 Boost 库
```bash
sudo apt update
sudo apt install -y libboost-all-dev
```

2. 安装 Muduo 库

> Muduo 官方地址：[muduo (GitHub)](https://github.com/chenshuo/muduo)
```bash
# 安装依赖
sudo apt install -y cmake g++ git

# 下载源码
git clone https://github.com/chenshuo/muduo.git
cd muduo

# 构建 & 安装
mkdir build && cd build
cmake ..
make -j4
sudo make install
```

安装完成后，头文件位于 `/usr/local/include/muduo`，库文件位于 `/usr/local/lib`。

3. 数据库初始化
执行建表和示例数据脚本：
```bash
mysql -u root -p chat < sql/init_chat.sql
mysql -u root -p chat < sql/init_chat_data.sql
```

## 学习收获
1. JSON 序列化/反序列化：用于客户端和服务端通信。

2. Muduo 搭建服务器的步骤

* 创建 `TcpServer` 对象
* 创建 `EventLoop` 事件循环
* 在构造函数中注册连接 & 读写回调
* 设置线程数量（muduo 自动调度）

3. 数据库表设计：根据业务拆分表结构。

4. 函数回调机制：使用 `std::function` 和 `std::bind` 解耦网络层与业务层。

5. 设计原则：开闭原则（对扩展开放，对修改关闭）。例如客户端 `mainmenu`，可通过参数自动回调对应功能。

6. 调试工具：使用 valgrind 检查内存非法访问和泄漏（`valgrind ./ChatClient 127.0.0.1 2222`）

## 遇到的问题
1. 构建/编译

* 不熟悉 Makefile → 改用 CMake 自动生成。
* 嵌套 CMakeLists.txt 的写法（每个源代码目录单独一个文件）。
* 变量拼写错误导致找不到库（用 message 调试）。
* 链接问题 → 在 CMakeLists.txt 手动补充缺失源码文件。

2. 语言/机制

* 理解 `std::function` 和 `std::bind` 的用法。
* `unordered_map` 存储连接对象时需要加锁保证线程安全。
* MySQL 结果集 `res->next()` 会移动指针，应改为 `while(res->next())`。
* 插入自增主键后需要获取 `last_insert_id()`。
* JSON 无法直接序列化 `vector<User>`，需先转为 `vector<string>`。

3. 运行时

* 内存泄漏问题 → 使用 valgrind 定位。


## Bug 状态
持续修复ing
| 状态    | 描述                                          |
| ----- | ------------------------------------------- |
| ✅ 已修复 | 可以和非好友聊天 → 在 `ChatService::oneChat` 中增加好友检查 |
| ⏳ 待解决 | 可以添加不存在的好友 → 需在插入前验证用户 ID 是否存在              |

