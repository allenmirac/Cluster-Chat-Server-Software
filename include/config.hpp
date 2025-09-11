#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#include <muduo/base/Logging.h>

class Config {
public:
    static Config& instance() {
        static Config instance;
        return instance;
    }

    bool load(const std::string& configPath = "config.json") {
        try {
            std::ifstream configFile(configPath);
            if (!configFile.is_open()) {
                LOG_ERROR << "Cannot open config file: " << configPath;
                return false;
            }

            nlohmann::json config = nlohmann::json::parse(configFile);
            
            // 解析节点配置
            nodeId = config.value("node_id", "node-default");
            
            // 解析服务器配置
            if (config.contains("server")) {
                auto& server = config["server"];
                serverIp = server.value("ip", "0.0.0.0");
                serverPort = server.value("port", 8000);
            }
            
            // 解析Redis配置
            if (config.contains("redis")) {
                auto& redis = config["redis"];
                redisUrl = redis.value("url", "redis://localhost:6379");
                redisPoolSize = redis.value("pool_size", 10);
                redisPoolWaitMs = redis.value("pool_wait_ms", 5000);
            }
            
            // 解析日志配置
            if (config.contains("log")) {
                auto& log = config["log"];
                logLevel = log.value("level", "info");
                logPath = log.value("path", "./logs/chat.log");
            }
            
            LOG_INFO << "Config loaded successfully from " << configPath;
            return true;
            
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to parse config file: " << e.what();
            return false;
        }
    }

    // 配置项
    std::string nodeId;
    std::string serverIp;
    int serverPort;
    std::string redisUrl;
    int redisPoolSize;
    int redisPoolWaitMs;
    std::string logLevel;
    std::string logPath;

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
};