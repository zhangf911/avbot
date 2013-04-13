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
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <boost/property_tree/ptree.hpp>
namespace pt = boost::property_tree;
#include <boost/property_tree/json_parser.hpp>
namespace js = boost::property_tree::json_parser;
#include <boost/circular_buffer.hpp>

#include "boost/coro/coro.hpp"

#include "botctl.hpp"

#include "boost/coro/yield.hpp"

namespace detail {

// avbot_rpc_server 由 acceptor_server 这个辅助类调用
// 为其构造函数传入一个 m_socket, 是 shared_ptr 的.
class avbot_rpc_server
{
public:
	typedef boost::signals2::signal<
		void( std::string protocol, std::string room, std::string who,
				std::string message, sender_flags )
	> on_message_signal_type;
	static on_message_signal_type on_message;

	typedef boost::asio::ip::tcp Protocol;
	typedef boost::asio::basic_stream_socket<Protocol> socket_type;
	typedef void result_type;

	avbot_rpc_server(boost::shared_ptr<socket_type> _socket)
	  : m_socket(_socket)
	  , m_request(new boost::asio::streambuf)
	  , m_responses(new boost::circular_buffer_space_optimized<boost::shared_ptr<boost::asio::streambuf> >(20) )
	{
		m_socket->get_io_service().post(
			boost::asio::detail::bind_handler(*this, boost::coro::coroutine(), boost::system::error_code(), 0)
		);
	}

	// 数据操作跑这里，嘻嘻.
	void operator()(boost::coro::coroutine coro, boost::system::error_code ec, std::size_t bytestransfered)
	{
		boost::shared_ptr<boost::asio::streambuf>	sendbuf;

		if (ec){
			m_socket->close(ec);
			// 看来不是 HTTP 客户端，诶，滚蛋啊！
			// 沉默，直接关闭链接. 取消信号注册.
		 	if (m_connect && m_connect->connected())
				m_connect->disconnect();
			return;
		}

		CORO_REENTER(&coro)
		{
		do{
			// 发起 HTTP 处理操作.
			_yield boost::asio::async_read_until(*m_socket, *m_request, "\r\n\r\n", boost::bind(*this, coro, _1, _2));
			m_request->consume(bytestransfered);

			// 解析 HTTP

			// 等待消息.
			if (m_responses->empty())
			{
				if (!m_connect){
					// 将自己注册到 avbot 的 signal 去
					// 等 有消息的时候，on_message 被调用，也就是下面的 operator() 被调用.
					_yield m_connect = boost::make_shared<boost::signals2::connection>
						(on_message.connect(boost::bind(*this, coro, _1, _2, _3, _4, _5)));

				}else{
					return;
				}
				// signals2 回调的时候会进入到这一行.
			}
			// 进入发送过程
			sendbuf = m_responses->front();
			_yield boost::asio::async_write(*m_socket, *sendbuf, boost::bind(*this, coro, _1, _2) );
			m_responses->pop_front();
			// 写好了，重新开始我们的处理吧!
		}while(1);
		}
	}

	// signal 的回调到了这里, 这里我们要区分对方是不是用了 keep-alive 呢.
	void operator()(boost::coro::coroutine coro, std::string protocol, std::string room, std::string who, std::string message, sender_flags)
	{
		pt::ptree jsonmessage;
		boost::shared_ptr<boost::asio::streambuf> buf(new boost::asio::streambuf);
		std::ostream	stream(buf.get());
		std::stringstream	teststream;
		jsonmessage.put("protocol", protocol);
		jsonmessage.put("root", room);
		jsonmessage.put("who", who);
		jsonmessage.put("msg", message);

		js::write_json(teststream,  jsonmessage);

		// 直接写入 json 格式的消息吧!
		stream <<  "HTTP/1.1 200 OK\r\n" <<  "Content-type: application/json\r\n";
		stream <<  "connection: keep-alive\r\n" <<  "Content-length: ";
		stream << teststream.str().length() <<  "\r\n\r\n";

		js::write_json(stream, jsonmessage);

		// 检查 发送缓冲区.
		if (m_responses->empty()){
			// 打通仁督脉.
			m_socket->get_io_service().post(boost::asio::detail::bind_handler(*this, coro, boost::system::error_code(), 0));
		}
		// 写入 m_responses
		m_responses->push_back(buf);
	}
private:
	boost::shared_ptr<socket_type> m_socket;
	boost::shared_ptr<boost::signals2::connection> m_connect;

	boost::shared_ptr<boost::asio::streambuf>	m_request;
	boost::shared_ptr< boost::circular_buffer_space_optimized<boost::shared_ptr<boost::asio::streambuf> > >	m_responses;
};
}

using detail::avbot_rpc_server;

#include "boost/coro/unyield.hpp"
