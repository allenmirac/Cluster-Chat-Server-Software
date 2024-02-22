#ifndef PUBLIC_H
#define PUBLIC_H

/*
 server 和 client 公共头文件
*/
enum
{
    LOGIN_MSG = 1, // 登陆消息
    LOGIN_MSG_ACK,
    
    REG_MSG, // 注册消息
    REG_MSG_ACK,

    ONE_CHAT_MSG,
    ADD_FRIEND_MSG,

    CREATE_GROUP_MSG,
    JOIN_GROUP_MSG,
    GROUP_CHAT_MSG
};

#endif // PUBLIC_H