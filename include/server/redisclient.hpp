#ifndef REDISCLIENT_HPP
#define REDISCLIENT_HPP

#include <muduo/base/Logging.h>
#include <sw/redis++/redis++.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <mutex>
#include <set>
#include <vector>
#include <atomic>
#include <functional>

class RedisClient {
public:
    RedisClient(int pool_wait_ms=5000);
    ~RedisClient();
    std::shared_ptr<sw::redis::Redis> getConnFromPool();
    
    void releaseConnToPool(std::shared_ptr<sw::redis::Redis> conn);
    void setUserOnline(int user_id, const std::string& conn_id);
    bool isUserOnline(int user_id);
    std::string getUserConnId(int user_id);
    std::string getUserNode(int user_id);
    void setUserOffline(int user_id);

    // Offline messages
    void pushOfflineMessage(int user_id, const nlohmann::json& msg);
    std::vector<std::string> popOfflineMessages(int user_id);

    // Generic hash / kv operations
    void hset(const std::string& key, const std::string& field, const std::string& value);
    std::optional<std::string> hget(const std::string& key, const std::string& field);
    void hdel(const std::string& key, const std::string& field);
    void set(const std::string& key, const std::string& value, std::chrono::seconds ttl = std::chrono::seconds(0));
    std::optional<std::string> get(const std::string& key);
    void del(const std::string& key);
    void expire(const std::string& key, std::chrono::seconds ttl);
    void sadd(const std::string& key, const std::string& member);
    bool sismember(const std::string& key, const std::string& member);

    // Publishing
    void publishGroupMessage(int group_id, const nlohmann::json& msg);
    void publishNodeInbox(const std::string& destNode, const nlohmann::json& msg);

    // Subscriber management
    void subscribeGroupChannel(int group_id);
    void unsubscribeGroupChannel(int group_id);
    void setInboxCallback(std::function<void(const nlohmann::json&)> cb);

    std::string getLocalNodeId();
private:
    void startSubscriberThread();
    void dispatchMessageToLocal(const nlohmann::json& j);

    struct SubTask {
        enum class Type { Subscribe, Unsubscribe, Stop };
        Type type;
        int groupId;
        SubTask(Type t, int gid) : type(t), groupId(gid) {}
    };

    std::string localNodeId_; // 本地节点ID
    int pool_wait_ms_;        // 连接池等待时间
    std::atomic<bool> stopped_; // 停止标志
    std::thread sub_thread_;  // 订阅者线程
    std::mutex sub_mutex_;    // 订阅任务互斥锁
    std::mutex callback_mutex_; // 回调互斥锁
    std::vector<SubTask> sub_tasks_; // 订阅任务队列
    std::set<int> sub_group_set_; // 已订阅的群组ID集合
    std::condition_variable sub_cond_; // 条件变量，用于通知订阅者线程
    std::function<void(const nlohmann::json&)> inbox_callback_; // 消息回调函数
};

#endif // REDISCLIENT_HPP