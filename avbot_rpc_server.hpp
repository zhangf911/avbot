/*
 * Copyright (C) 2013  microcai <microcai@fedoraproject.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include "botctl.hpp"

class avbot_rpc_server
{

public:
	typedef boost::signals2::signal< void( std::string protocol, std::string room, std::string who, std::string message, sender_flags ) > on_message_signal_type;
	static on_message_signal_type on_message;

	typedef boost::asio::ip::tcp Protocol;
	typedef boost::shared_ptr<boost::asio::basic_stream_socket<Protocol> > socket_type;
	socket_type m_socket;

	boost::shared_ptr<boost::signals2::scoped_connection> m_connect;
	avbot_rpc_server(socket_type _socket)
	  : m_socket(_socket), m_connect(new boost::signals2::scoped_connection)
	{
		// 将自己注册到 avbot 的 signal 去
		// 等 有消息的时候，on_message 被调用，也就是下面的 operator() 被调用.
		*m_connect = on_message.connect(*this);
	}

	// signal 的回调到了这里, 这里我们要区分对方是不是用了 keep-alive 呢.
	void operator()(std::string protocol, std::string room, std::string who, std::string message, sender_flags)
	{
		// 直接写入 json 格式的消息吧!
	}
};
