-- =====================================
-- 数据库：chat
-- =====================================
CREATE DATABASE IF NOT EXISTS chat DEFAULT CHARSET=utf8mb4;
USE chat;

-- =====================================
-- 用户表
-- =====================================
CREATE TABLE IF NOT EXISTS User (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(50) NOT NULL UNIQUE,
    password VARCHAR(100) NOT NULL,
    state ENUM('online', 'offline') DEFAULT 'online'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================
-- 好友表
-- 互为好友只存一条记录
-- =====================================
CREATE TABLE IF NOT EXISTS Friend (
    userid INT NOT NULL,
    friendid INT NOT NULL,
    PRIMARY KEY(userid, friendid),
    FOREIGN KEY(userid) REFERENCES User(id) ON DELETE CASCADE,
    FOREIGN KEY(friendid) REFERENCES User(id) ON DELETE CASCADE,
    CHECK (userid < friendid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================
-- 群组表
-- =====================================
CREATE TABLE IF NOT EXISTS AllGroup (
    id INT PRIMARY KEY AUTO_INCREMENT,
    groupname VARCHAR(50) NOT NULL UNIQUE,
    groupdesc VARCHAR(200) DEFAULT ''
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================
-- 群成员表
-- =====================================
CREATE TABLE IF NOT EXISTS GroupUser (
    groupid INT NOT NULL,
    userid INT NOT NULL,
    grouprole ENUM('creator', 'normal') DEFAULT 'normal',
    PRIMARY KEY(groupid, userid),
    FOREIGN KEY(groupid) REFERENCES AllGroup(id) ON DELETE CASCADE,
    FOREIGN KEY(userid) REFERENCES User(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- =====================================
-- 离线消息表
-- =====================================
CREATE TABLE IF NOT EXISTS OfflineMessage (
    id INT PRIMARY KEY AUTO_INCREMENT,
    userid INT NOT NULL,
    message VARCHAR(500) NOT NULL,
    FOREIGN KEY(userid) REFERENCES User(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
