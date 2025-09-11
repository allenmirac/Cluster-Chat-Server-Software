#include <vector>
#include <thread>
#include <chrono>
#include "mysqlconnectionpool.hpp"
#include "offlinemessage.hpp"

bool OfflineMessage::insert(int userId, const string& message) {
    const int maxRetries = 3;  // 最大重试次数
    int retryCount = 0;
    bool success = false;

    while (retryCount < maxRetries && !success) {
        MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
        sql::Connection *conn = mysqlPool->getConnection();
        if (!conn) {
            LOG_ERROR << "OfflineMessage::insert: getConnection() returned nullptr, retry " << retryCount + 1;
            retryCount++;
            std::this_thread::sleep_for(std::chrono::seconds(1));  // 等待1秒重试
            continue;
        }

        try {
            // 先处理缓冲队列中的失败消息（如果有）
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                while (!failedQueue_.empty()) {
                    auto& item = failedQueue_.front();
                    // 执行插入（原代码逻辑）
                    std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("INSERT INTO OfflineMessage (userid, message) VALUES (?, ?)"));
                    pstmt->setInt(1, item.first);
                    pstmt->setString(2, item.second);
                    pstmt->executeUpdate();
                    failedQueue_.pop();
                    LOG_INFO << "OfflineMessage::insert: Processed queued message for user " << item.first;
                }
            }

            // 插入当前消息
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement("INSERT INTO OfflineMessage (userid, message) VALUES (?, ?)"));
            pstmt->setInt(1, userId);
            pstmt->setString(2, message);
            pstmt->executeUpdate();
            success = true;
            LOG_INFO << "OfflineMessage::insert: Success for user " << userId;
        } catch (sql::SQLException &e) {
            LOG_ERROR << "OfflineMessage::insert: SQLException: " << e.what() << " (code: " << e.getErrorCode() << ")";
        } catch (...) {
            LOG_ERROR << "OfflineMessage::insert: Unknown exception";
        }

        // 归还连接
        mysqlPool->releaseConnection(conn);
    }

    if (!success) {
        // 重试失败，放入队列缓冲（模拟Redis队列）
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            failedQueue_.push({userId, message});
            LOG_WARN << "OfflineMessage::insert: Failed after retries, queued message for user " << userId << ". Queue size: " << failedQueue_.size();
        }
        // 可选：如果队列过大，触发警报或持久化到文件，但为简单起见，仅队列缓冲
    }
    return success;
}
bool OfflineMessage::remove(int userId)
{
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    sql::Connection *conn = mysqlPool->getConnection();
    if (!conn) {
        LOG_ERROR << "OfflineMessage::insert: getConnection() returned nullptr";
        return false;
    }
    // 使用预处理语句，防止 sql 注入
    try
    {
        sql::PreparedStatement *pstmt;
        pstmt = conn->prepareStatement("DELETE FROM OfflineMessage where userid=?");
        pstmt->setInt(1, userId);
        pstmt->executeUpdate();

        delete pstmt;
        mysqlPool->releaseConnection(conn);
        return true;
    }
    catch (sql::SQLException &e)
    {
        LOG_ERROR << "OfflineMessage::insert, SQL Exception: " << e.what();
    }
    return false;
}
vector<string> OfflineMessage::query(int userId)
{
    MySQLConnectionPool *mysqlPool = MySQLConnectionPool::getInstance();
    sql::Connection *conn = mysqlPool->getConnection();
    if (!conn) {
        LOG_ERROR << "OfflineMessage::insert: getConnection() returned nullptr";
        return vector<string>();
    }
    vector<string> v;
    sql::Statement *stmt;
    sql::ResultSet *res;
    try
    {
        stmt = conn->createStatement();
        string sql = "select userid, message from OfflineMessage where userid=" + to_string(userId);
        res = stmt->executeQuery(sql);
        string queryRes;
        while(res->next()){
            queryRes = res->getString(2);
            v.push_back(queryRes);
        }
        delete res;
        delete stmt;
        mysqlPool->releaseConnection(conn);
        if(!v.empty())
        {
            // LOG_INFO << "v:" << v[0];
            return v;
        }
        else
        {
            LOG_ERROR << "UserModel::query, 用户 [" << userId << "] 没有离线消息";
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR << "OfflineMessage::query error" << e.what() << '\n';
    }
    return vector<string>();
}