#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <string>
#include <iostream>

using namespace std;
using namespace muduo;
using namespace muduo::net;

class ChatServer{
public:
    ChatServer(EventLoop* loop,
               const InetAddress& listenAddr,
               const string& nameArg,
               TcpServer::Option option = TcpServer::Option::kNoReusePort)
        : server_(loop, listenAddr, nameArg), loop_(loop)
    {
        // 注册用户连接的创建和断开的回调
        server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

        // 注册用户读写事件的回调
        server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        server_.setThreadNum(4);
    }

    void start(){
        server_.start();
    }

private:

    // 处理连接的创建和断开
    void onConnection(const TcpConnectionPtr &conn){
        if(conn->connected()){
            cout<<conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << endl;
        }else{
            cout<<conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << endl;
            conn->shutdown();
            // loop_->quit();
        }
    }

    // 处理消息
    void onMessage(const TcpConnectionPtr& conn,
                   Buffer *buffer,
                   Timestamp time)
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data: "<<buf<< "time: "<<time.toFormattedString()<<endl;
        string msg = "From server: " + buf;
        conn->send(msg);
    }
    TcpServer server_;
    EventLoop *loop_;

};

int main(){

    EventLoop loop;
    InetAddress addr("127.0.0.1", 2222);
    ChatServer server(&loop, addr, "CharServer");

    server.start();
    loop.loop();
    return 0;
}
// g++ -o muduoserver_ muduoserver_.cpp  -lmuduo_net -lmuduo_base -lpthread

// create table User(id INT PIMARY KEY auto_increment,name VARCHAR(50) NOT NULL UNIQUE,password varchar(50) not null,state ENUM('online', offline) default 'online');
// CREATE TABLE Friend(userid INT NOT NULL, friendid INT NOT NULL);
// CREATE TABLE AllGroup(id INT PRIMARY KEY AUTO_INCREMENT, groupname VARCHAR(50) NOT NULL, groupdesc VARCHAR(200) DEFAULT '');
// CREATE TABLE GroupUser(groupid INT PRIMARY KEY, userid INT NOT NULL, grouprole ENUM('creator', 'normal') DEFAULT 'normal');
// CREATE TABLE OfflineMessage(userid INT PRIMARY KEY, message VARCHAR(500) NOT NULL);


