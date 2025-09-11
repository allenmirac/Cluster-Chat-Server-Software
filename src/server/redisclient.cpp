#include "redisclient.hpp"
#include "redisconnectionpool.hpp"
#include "config.hpp"
#include <muduo/base/Logging.h>
#include <sw/redis++/redis++.h>
#include <thread>
#include <atomic>
#include <sstream>

using namespace sw::redis;

RedisClient::RedisClient(int pool_wait_ms)
    : pool_wait_ms_(pool_wait_ms), stopped_(false)
{
    auto& config = Config::instance();
    localNodeId_ = config.nodeId;
    
    LOG_INFO << "Starting RedisClient with node ID: " << localNodeId_;
    // Acquire a dedicated redis connection for pub/sub (subscriber) from pool.
    // We'll try to acquire but keep running even if temporarily unavailable.
    startSubscriberThread();
}

RedisClient::~RedisClient() {
    stopped_ = true;
    // signal subscriber thread to exit
    {
        std::lock_guard<std::mutex> lk(sub_mutex_);
        sub_tasks_.push_back({SubTask::Type::Stop, 0});
    }
    sub_cond_.notify_one();
    if (sub_thread_.joinable()) sub_thread_.join();
}

std::string RedisClient::getLocalNodeId() {
    static std::string cachedNodeId;
    
    if (!cachedNodeId.empty()) {
        return cachedNodeId;
    }
    
    // 优先使用配置文件中的节点ID
    auto& config = Config::instance();
    if (!config.nodeId.empty() && config.nodeId != "node-default") {
        cachedNodeId = config.nodeId;
        return cachedNodeId;
    }
    
    // 回退到其他生成方式
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    cachedNodeId = std::string(hostname) + "-" + std::to_string(getpid());
    
    return cachedNodeId;
}

std::shared_ptr<sw::redis::Redis> RedisClient::getConnFromPool() {
    auto pool = RedisConnectionPool::getInstance();
    if (!pool) return nullptr;
    return pool->acquire(pool_wait_ms_);
}

void RedisClient::releaseConnToPool(std::shared_ptr<sw::redis::Redis> conn) {
    auto pool = RedisConnectionPool::getInstance();
    if (!pool || !conn) return;
    pool->release(conn);
}

// ---------- Presence API ----------
void RedisClient::setUserOnline(int user_id, const std::string& conn_id) {
    auto conn = getConnFromPool();
    if (!conn) {
        LOG_ERROR << "setUserOnline: no redis connection";
        return;
    }
    try {
        std::string key = "user:" + std::to_string(user_id);
        conn->hset(key, "online", "1");
        conn->hset(key, "node", localNodeId_);
        conn->hset(key, "conn", conn_id);
        conn->expire(key, 120); // TTL 120s, heartbeat should refresh
    } catch (const std::exception& ex) {
        LOG_ERROR << "setUserOnline exception: " << ex.what();
    }
    releaseConnToPool(conn);
}

bool RedisClient::isUserOnline(int user_id) {
    auto conn = getConnFromPool();
    if (!conn) return false;
    try {
        auto val = conn->hget("user:" + std::to_string(user_id), "online");
        releaseConnToPool(conn);
        return val && *val == "1";
    } catch (const std::exception& ex) {
        LOG_ERROR << "isUserOnline exception: " << ex.what();
        releaseConnToPool(conn);
        return false;
    }
}

std::string RedisClient::getUserConnId(int user_id) {
    auto conn = getConnFromPool();
    if (!conn) return "";
    try {
        auto val = conn->hget("user:" + std::to_string(user_id), "conn");
        releaseConnToPool(conn);
        return val ? *val : "";
    } catch (const std::exception& ex) {
        LOG_ERROR << "getUserConnId exception: " << ex.what();
        releaseConnToPool(conn);
        return "";
    }
}

std::string RedisClient::getUserNode(int user_id) {
    auto conn = getConnFromPool();
    if (!conn) return "";
    try {
        auto val = conn->hget("user:" + std::to_string(user_id), "node");
        releaseConnToPool(conn);
        return val ? *val : "";
    } catch (const std::exception& ex) {
        LOG_ERROR << "getUserNode exception: " << ex.what();
        releaseConnToPool(conn);
        return "";
    }
}

void RedisClient::setUserOffline(int user_id) {
    auto conn = getConnFromPool();
    if (!conn) {
        LOG_ERROR << "setUserOffline: no redis connection";
        return;
    }
    try {
        std::string key = "user:" + std::to_string(user_id);
        conn->hdel(key, "online");
        conn->hdel(key, "conn");
        conn->hdel(key, "node");
    } catch (const std::exception& ex) {
        LOG_ERROR << "setUserOffline exception: " << ex.what();
    }
    releaseConnToPool(conn);
}

// ---------- Offline messages ----------
void RedisClient::pushOfflineMessage(int user_id, const nlohmann::json& msg) {
    auto conn = getConnFromPool();
    if (!conn) return;
    try {
        conn->lpush("offline:" + std::to_string(user_id), msg.dump());
        // Optionally limit list size: conn->ltrim(...)
    } catch (const std::exception& ex) {
        LOG_ERROR << "pushOfflineMessage exception: " << ex.what();
    }
    releaseConnToPool(conn);
}

std::vector<std::string> RedisClient::popOfflineMessages(int user_id) {
    std::vector<std::string> messages;
    auto conn = getConnFromPool();
    if (!conn) return messages;
    try {
        conn->lrange("offline:" + std::to_string(user_id), 0, -1, std::back_inserter(messages));
        conn->del("offline:" + std::to_string(user_id));
    } catch (const std::exception& ex) {
        LOG_ERROR << "popOfflineMessages exception: " << ex.what();
    }
    releaseConnToPool(conn);
    return messages;
}

// ---------- Generic hash / kv ----------
void RedisClient::hset(const std::string& key, const std::string& field, const std::string& value) {
    auto conn = getConnFromPool();
    if (!conn) return;
    try { conn->hset(key, field, value); } catch (const std::exception& ex) { LOG_ERROR << "hset ex: " << ex.what(); }
    releaseConnToPool(conn);
}

std::optional<std::string> RedisClient::hget(const std::string& key, const std::string& field) {
    auto conn = getConnFromPool();
    if (!conn) return std::nullopt;
    try {
        auto opt = conn->hget(key, field);
        releaseConnToPool(conn);
        return opt;
    } catch (const std::exception& ex) {
        LOG_ERROR << "hget ex: " << ex.what();
        releaseConnToPool(conn);
        return std::nullopt;
    }
}

void RedisClient::hdel(const std::string& key, const std::string& field) {
    auto conn = getConnFromPool();
    if (!conn) return;
    try { conn->hdel(key, field); } catch (const std::exception& ex) { LOG_ERROR << "hdel ex: " << ex.what(); }
    releaseConnToPool(conn);
}

void RedisClient::set(const std::string& key, const std::string& value, std::chrono::seconds ttl) {
    auto conn = getConnFromPool();
    if (!conn) return;
    try {
        if (ttl.count() > 0) conn->set(key, value, std::chrono::seconds(ttl));
        else conn->set(key, value);
    } catch (const std::exception& ex) {
        LOG_ERROR << "set ex: " << ex.what();
    }
    releaseConnToPool(conn);
}

std::optional<std::string> RedisClient::get(const std::string& key) {
    auto conn = getConnFromPool();
    if (!conn) return std::nullopt;
    try {
        auto val = conn->get(key);
        releaseConnToPool(conn);
        return val;
    } catch (const std::exception& ex) {
        LOG_ERROR << "get ex: " << ex.what();
        releaseConnToPool(conn);
        return std::nullopt;
    }
}

void RedisClient::del(const std::string& key) {
    auto conn = getConnFromPool();
    if (!conn) return;
    try { conn->del(key); } catch (const std::exception& ex) { LOG_ERROR << "del ex: " << ex.what(); }
    releaseConnToPool(conn);
}

void RedisClient::expire(const std::string& key, std::chrono::seconds ttl) {
    auto conn = getConnFromPool();
    if (!conn) return;
    try { conn->expire(key, static_cast<long long>(ttl.count())); } catch (const std::exception& ex) { LOG_ERROR << "expire ex: " << ex.what(); }
    releaseConnToPool(conn);
}

void RedisClient::sadd(const std::string& key, const std::string& member) {
    auto conn = getConnFromPool();
    if (!conn) return;
    try { conn->sadd(key, member); } catch (const std::exception& ex) { LOG_ERROR << "sadd ex: " << ex.what(); }
    releaseConnToPool(conn);
}

bool RedisClient::sismember(const std::string& key, const std::string& member) {
    auto conn = getConnFromPool();
    if (!conn) return false;
    try {
        bool res = conn->sismember(key, member);
        releaseConnToPool(conn);
        return res;
    } catch (const std::exception& ex) {
        LOG_ERROR << "sismember ex: " << ex.what();
        releaseConnToPool(conn);
        return false;
    }
}

// ---------- Publishing ----------
void RedisClient::publishGroupMessage(int group_id, const nlohmann::json& msg) {
    auto conn = getConnFromPool();
    if (!conn) return;
    try {
        conn->publish("group:" + std::to_string(group_id), msg.dump());
    } catch (const std::exception& ex) {
        LOG_ERROR << "publishGroupMessage ex: " << ex.what();
    }
    releaseConnToPool(conn);
}

void RedisClient::publishNodeInbox(const std::string& destNode, const nlohmann::json& msg) {
    auto conn = getConnFromPool();
    if (!conn) return;
    try {
        conn->publish("node:" + destNode + ":inbox", msg.dump());
    } catch (const std::exception& ex) {
        LOG_ERROR << "publishNodeInbox ex: " << ex.what();
    }
    releaseConnToPool(conn);
}

// ---------- Subscriber thread & dynamic subscribe management ----------
void RedisClient::startSubscriberThread() {
    sub_thread_ = std::thread([this]() {
        // Acquire a dedicated connection specifically for subscribe
        auto pool = RedisConnectionPool::getInstance();
        if (!pool) {
            LOG_ERROR << "startSubscriberThread: no pool";
            return;
        }
        auto sub_conn = pool->acquire(pool_wait_ms_);
        if (!sub_conn) {
            LOG_ERROR << "startSubscriberThread: failed to acquire redis conn for subscriber";
            return;
        }

        try {
            auto subscriber = sub_conn->subscriber();
            // setup on_message to dispatch to local handler
            subscriber.on_message([this](const std::string &channel, const std::string &msg) {
                // channel can be "node:<nodeId>:inbox" or "group:<groupId>"
                try {
                    nlohmann::json j = nlohmann::json::parse(msg);
                    // push to internal dispatch queue
                    dispatchMessageToLocal(j);
                } catch (const std::exception& ex) {
                    LOG_ERROR << "subscriber on_message parse error: " << ex.what();
                }
            });

            // Always subscribe to this node's inbox
            std::string nodeChannel = "node:" + localNodeId_ + ":inbox";
            subscriber.subscribe(nodeChannel);

            // Also subscribe to any pre-registered group channels in sub_group_set_
            {
                std::lock_guard<std::mutex> lk(sub_mutex_);
                for (auto gid : sub_group_set_) {
                    subscriber.subscribe("group:" + std::to_string(gid));
                }
            }

            // consume loop (with timeout) so we can process dynamic subscribe tasks periodically
            while (!stopped_) {
                // do consume with timeout so we can check for queued subscribe/unsubscribe tasks
                try {
                    // consume with timeout - many versions support a timeout overload
                    subscriber.consume();//std::chrono::milliseconds(200));
                } catch (const sw::redis::Error &ex) {
                    std::string err = ex.what();
                    if (err.find("Resource temporarily unavailable") != std::string::npos) {
                        // 只是没有消息，忽略即可
                    } else {
                        LOG_ERROR << "subscriber.consume exception: " << ex.what();
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                    }
                }

                // process pending subscribe/unsubscribe tasks
                std::vector<SubTask> tasks;
                {
                    std::lock_guard<std::mutex> lk(sub_mutex_);
                    tasks.swap(sub_tasks_);
                }
                for (auto &t : tasks) {
                    if (t.type == SubTask::Type::Subscribe) {
                        try {
                            subscriber.subscribe("group:" + std::to_string(t.groupId));
                            std::lock_guard<std::mutex> lk(sub_mutex_);
                            sub_group_set_.insert(t.groupId);
                        } catch (const std::exception& ex) {
                            LOG_ERROR << "subscribe task failed: " << ex.what();
                        }
                    } else if (t.type == SubTask::Type::Unsubscribe) {
                        try {
                            subscriber.unsubscribe("group:" + std::to_string(t.groupId));
                            std::lock_guard<std::mutex> lk(sub_mutex_);
                            sub_group_set_.erase(t.groupId);
                        } catch (const std::exception& ex) {
                            LOG_ERROR << "unsubscribe task failed: " << ex.what();
                        }
                    } else if (t.type == SubTask::Type::Stop) {
                        // exit loop
                        stopped_ = true;
                        break;
                    }
                }
            }

            // cleanup: unsubscribe all
            try {
                subscriber.unsubscribe(nodeChannel);
                std::lock_guard<std::mutex> lk(sub_mutex_);
                for (auto gid : sub_group_set_) {
                    subscriber.unsubscribe("group:" + std::to_string(gid));
                }
            } catch (...) {
                // ignore
            }
        } catch (const std::exception& ex) {
            LOG_ERROR << "subscriber thread exception: " << ex.what();
        }

        // release the subscriber connection back to pool
        pool->release(sub_conn);
    });
}

// enqueue subscription request (thread-safe) - node will call this when local node has members of group
void RedisClient::subscribeGroupChannel(int group_id) {
    std::lock_guard<std::mutex> lk(sub_mutex_);
    // if already subscribed, skip
    if (sub_group_set_.count(group_id)) return;
    sub_tasks_.push_back({SubTask::Type::Subscribe, group_id});
}

// enqueue unsubscribe - call when no local member remains
void RedisClient::unsubscribeGroupChannel(int group_id) {
    std::lock_guard<std::mutex> lk(sub_mutex_);
    if (!sub_group_set_.count(group_id)) return;
    sub_tasks_.push_back({SubTask::Type::Unsubscribe, group_id});
}

// Dispatch incoming message JSON to ChatService (via callback)
void RedisClient::dispatchMessageToLocal(const nlohmann::json& j) {
    // caller should be ChatService::instance() - we only call the registered handler
    std::lock_guard<std::mutex> lk(callback_mutex_);
    if (inbox_callback_) {
        inbox_callback_(j);
    } else {
        LOG_WARN << "dispatchMessageToLocal: no inbox_callback_ registered";
    }
}

// Register a callback for messages received by subscriber thread
void RedisClient::setInboxCallback(std::function<void(const nlohmann::json&)> cb) {
    std::lock_guard<std::mutex> lk(callback_mutex_);
    inbox_callback_ = std::move(cb);
}
