#pragma once

#include <string>
#include <boost/function.hpp>
#include <boost/asio/io_service.hpp>

class webqq;

enum sender_flags{
	sender_is_op, // 管理员, 群管理员或者频道OP .
	sender_is_normal, // 普通用户.
};

//-------------

// 命令控制, 所有的协议都能享受的命令控制在这里实现.
// msg_sender 是一个函数, on_command 用它发送消息.
void on_bot_command(boost::asio::io_service& io_service, std::string message, std::string from_channel, std::string sender, sender_flags sender_flag, boost::function<void(std::string)> msg_sender, webqq * qqclient = NULL);
