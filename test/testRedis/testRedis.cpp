#include <iostream>
#include <muduo/base/Logging.h>
#include "redisclient.hpp"
#include "redisconnectionpool.hpp"

int main() {
    RedisConnectionPool *redisPool = RedisConnectionPool::getInstance();
    redisPool->init();
    RedisClient redis(5000);

    int userId = 1001;

    // 测试上线
    redis.setUserOnline(userId, "conn-12345");
    std::cout << "User online? " << redis.isUserOnline(userId) << std::endl;

    // 测试获取connId
    std::string connId = redis.getUserConnId(userId);
    std::cout << "ConnId: " << connId << std::endl;

    // 测试离线消息
    nlohmann::json msg;
    msg["from"] = 1002;
    msg["to"] = 1001;
    
    msg["msg"] = "hello";
    redis.pushOfflineMessage(userId, msg);

    auto msgs = redis.popOfflineMessages(userId);
    for (auto &m : msgs) {
        std::cout << "OfflineMsg: " << m << std::endl;
    }

    // 下线
    redis.setUserOffline(userId);
    std::cout << "User online? " << redis.isUserOnline(userId) << std::endl;

    return 0;
}
