#ifndef REDISCONNECTIONPOOL_H
#define REDISCONNECTIONPOOL_H

#include <sw/redis++/redis++.h>
#include <muduo/base/ThreadLocal.h>
#include <muduo/net/EventLoop.h>
#include <queue>
#include <shared_mutex>
#include <string>
#include <memory>
#include <condition_variable>

class RedisConnectionPool {
public:
    static RedisConnectionPool* getInstance();
    void init(const std::string& host = "127.0.0.1", int port = 6379, int pool_size = 8, const std::string& password = "");
    std::shared_ptr<sw::redis::Redis> acquire(int timeout_ms);
    void release(std::shared_ptr<sw::redis::Redis> conn);
    void stop();

private:
// 写入用 std::unique_lock<std::shared_mutex> lock(mutex_);
// 读操作用 std::shared_lock<std::shared_mutex> lock(mutex_);
    RedisConnectionPool() : stopped_(false) {}
    ~RedisConnectionPool() {
        stop();
    }
    std::string host_;
    int port_;
    bool stopped_;
    std::string password_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<std::shared_ptr<sw::redis::Redis>> connections_;
    muduo::ThreadLocal<std::shared_ptr<sw::redis::Redis>> thread_local_conn_;
};


#endif