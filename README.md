# Cluster-Chat-Server
集群聊天服务器--软件分层设计和高性能服务开发

## 集群聊天项目
### 项目运行环境
环境是虚拟机Ubuntu 20.04

gcc是9.4

cmake是3.22.3

安装了boost库

使用的Mysql库是C++ 版本的，MySQL8.0
### 我学会了

1、JSON数据的序列化和反序列化的使用

2、基于muduo网络库开发服务器程序的步骤

> 1、组合TcpServer对象

> 2、创建EventLoop事件循环对象的指针

> 3、明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数

> 4、在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写事件的回调函数

> 5、设置合适的服务端线程数量，muduo库会自己分配线程和worker线程

3、如何根据项目的功能来设计数据库的表。

4、通过函数回调机制来处理服务器与业务之间的耦合。

5、设计原则之一：开放封闭原则的内容是对扩展开放，对修改关闭，客户端中对于mainmenu函数功能的设计，通过不同的参数，可以自动回调相应的函数

6、使用valgrind来分析非法访问内存问题。

### 遇到的问题

1、编写`Makefile`文件不熟练，多试错，反正试错不需要成本，并且多去复现up的例子。发现编写`CMakeLists.txt`，可以自动生成`Makefile`

2、函数回调机制理解不了，`std::function`和`std::bind`的使用。`std::function`是函数对象包装器，将函数指针包装起来，

3、不知道如何编写嵌套的`CMake`，这时候需要给每个源文件目录编写`CMakeLists.txt`，头文件目录除外，这样可以使得项目维护起来更加容易。

4、业务层ORM，业务层操作的都是对象 DAO 数据层，具体的操作数据库

5、编写写测试文件的`CMakeLists.txt`搜索不到相对路径下的原文件，原来是下面的添加库的语句错了

（还是Chatgpt检查出来的(┬┬﹏┬┬)），说明message还是挺有用的，因为语法错了，不会报错，只能通过打印变量来查看。

```cmake
find_library(MYSQLCPPCONN_LIB mysqlcppconn muduo_net muduo_base pthread)
message(STATUS "MYSQLCPPCONN_LIB: ${MYSQLCPPCONN_LIB}")
```

6、不知道聊天服务转发消息的时候要不要控制线程安全，转发消息要先获取 conn ，是使用一个`unordered_map`来存储的，这个结构需要锁来控制，确保查询数据正确。

7、不知道`if(res->next())`判断完成后就会移动指针，需要直接改成`while(res->next())`来获取数据。

8、Json字符串无法将 `vector<User>`直接加入，需要额外转化成`vector<string>`。

9、向一个有自增主键的表里边加入数据后不知道如何获取回主键。

10、客户端编译时不能链接部分文件，直接在CMakeLists.txt里面手动添加了这个文件

```cmake
set(SRC_LIST1 ../server/model/user.cpp)
```

11、出现了内存泄漏但不知道如何解决。发现可以使用valgrind工具来分析，命令格式

```shell
valgrind ./ChatClient 127.0.0.1 2222
```

### 暂未解决的bug

1、好友列表里面没有他，但是可以和他聊天

2、可以添加不存在的好友
