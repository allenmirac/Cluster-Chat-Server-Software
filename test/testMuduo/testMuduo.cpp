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
        server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
        server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
        server_.setThreadNum(4);
    }
    void start(){
        server_.start();
    }
private:
    void onConnection(const TcpConnectionPtr &conn){
        if(conn->connected()){
            cout<<conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << endl;
        }else{
            cout<<conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << endl;
            conn->shutdown();
        }
    }

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