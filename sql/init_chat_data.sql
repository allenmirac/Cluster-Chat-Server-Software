-- ==============================
-- 初始化数据
-- ==============================

USE chat;

-- 插入用户
INSERT INTO User (name, password, state) VALUES
('alice', '123456', 'offline'),
('bob', '123456', 'offline'),
('charlie', '123456', 'online'),
('david', '123456', 'offline'),
('eva', '123456', 'online');

-- 插入好友关系
-- 注意：userid < friendid，避免重复
INSERT INTO Friend (userid, friendid) VALUES
(1, 2),   -- alice & bob
(1, 3),   -- alice & charlie
(2, 4),   -- bob & david
(3, 5);   -- charlie & eva

-- 插入群组
INSERT INTO AllGroup (groupname, groupdesc) VALUES
('TechTalk', '技术讨论群'),
('GamingClub', '游戏爱好者聚集地');

-- 插入群成员
-- TechTalk 群（id=1）
INSERT INTO GroupUser (groupid, userid, grouprole) VALUES
(1, 1, 'creator'),   -- alice 创建
(1, 2, 'normal'),    -- bob 加入
(1, 3, 'normal');    -- charlie 加入

-- GamingClub 群（id=2）
INSERT INTO GroupUser (groupid, userid, grouprole) VALUES
(2, 4, 'creator'),   -- david 创建
(2, 5, 'normal');    -- eva 加入

-- 插入离线消息
INSERT INTO OfflineMessage (userid, message) VALUES
(2, 'Hi Bob, this is Alice!'),
(4, 'David, are you coming to the meeting?'),
(5, 'Eva, welcome to the GamingClub!');
