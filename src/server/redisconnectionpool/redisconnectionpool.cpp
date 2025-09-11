#include "redisconnectionpool.hpp"
#include <muduo/base/Logging.h>

RedisConnectionPool* RedisConnectionPool::getInstance() {
    static RedisConnectionPool pool;
    return &pool;
}

void RedisConnectionPool::init(const std::string& host, int port, int pool_size, const std::string& password) {
    host_ = host;
    port_ = port;
    password_ = password;
    std::unique_lock<std::mutex> lock(mutex_);
    for (int i = 0; i < pool_size; ++i) {
        sw::redis::ConnectionOptions opts;
        opts.host = host;
        opts.port = port;
        opts.socket_timeout = std::chrono::milliseconds(2000);
        if (!password.empty()) opts.password = password;
        try {
            auto conn = std::make_shared<sw::redis::Redis>(opts);
            connections_.push(conn);
        } catch (const std::exception& ex) {
            LOG_ERROR << "RedisConnectionPool init connection failed: " << ex.what();
        }
    }
    LOG_INFO << "RedisConnectionPool initialized with " << connections_.size() << " connections";
}

std::shared_ptr<sw::redis::Redis> RedisConnectionPool::acquire(int timeout_ms) {
    std::unique_lock<std::mutex> lk(mutex_);
    if (stopped_) return nullptr;
    if (connections_.empty()) {
        if (timeout_ms <= 0) {
            cond_.wait(lk, [this] { return !connections_.empty() || stopped_; });
        } else {
            if (!cond_.wait_for(lk, std::chrono::milliseconds(timeout_ms), [this] { return !connections_.empty() || stopped_; })) {
                return nullptr;
            }
        }
    }
    if (stopped_ || connections_.empty()) return nullptr;
    auto conn = connections_.front();
    connections_.pop();
    return conn;
}

void RedisConnectionPool::release(std::shared_ptr<sw::redis::Redis> conn) {
    if (!conn) return;
    std::unique_lock<std::mutex> lk(mutex_);
    connections_.push(conn);
    cond_.notify_one();
}

void RedisConnectionPool::stop() {
    {
        std::unique_lock<std::mutex> lk(mutex_);
        if (stopped_) return;
        stopped_ = true;
        while (!connections_.empty()) {
            auto conn = connections_.front();
            connections_.pop();
        }
    }
    cond_.notify_all();
}