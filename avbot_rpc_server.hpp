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
#include <boost/enable_shared_from_this.hpp>
#include <boost/async_coro_queue.hpp>

#include "avhttpd.hpp"

#include "botctl.hpp"

#include <boost/asio/yield.hpp>

namespace detail
{

// avbot_rpc_server 由 acceptor_server 这个辅助类调用
// 为其构造函数传入一个 m_socket, 是 shared_ptr 的.
class avbot_rpc_server : boost::asio::coroutine
{
public:
	typedef boost::signals2::signal <
	void( boost::property_tree::ptree )
	> on_message_signal_type;
	on_message_signal_type & on_message;

	typedef boost::asio::ip::tcp Protocol;
	typedef boost::asio::basic_stream_socket<Protocol> socket_type;

	avbot_rpc_server( boost::shared_ptr<socket_type> _socket, on_message_signal_type & _on_message )
		: m_socket( _socket )
		, m_streambuf( new boost::asio::streambuf )
		, m_request( new avhttpd::request_opts )
		, on_message( _on_message )
	{
		m_responses = boost::make_shared<
				boost::async_coro_queue<
					boost::circular_buffer_space_optimized<
						boost::shared_ptr<boost::asio::streambuf>
					>
				>
			>(boost::ref(_socket->get_io_service()), 20 );

		m_connect = boost::make_shared<boost::signals2::connection>(
						on_message.connect(*this));

		m_socket->get_io_service().post(
			boost::asio::detail::bind_handler( *this, boost::system::error_code(), 0 )
		);
	}

	void operator()(boost::asio::coroutine coro, boost::system::error_code ec, boost::shared_ptr<boost::asio::streambuf> v);

	// 数据操作跑这里，嘻嘻.
	void operator()(boost::system::error_code ec, std::size_t bytestransfered);

	// signal 的回调到了这里, 这里我们要区分对方是不是用了 keep-alive 呢.
	void operator()(const boost::property_tree::ptree & jsonmessage )
	{
		boost::shared_ptr<boost::asio::streambuf> buf( new boost::asio::streambuf );
		std::ostream	stream( buf.get() );
		std::stringstream	teststream;

		js::write_json( teststream,  jsonmessage );

		// 直接写入 json 格式的消息吧!
		stream << "HTTP/1.1 200 OK\r\n" <<  "Content-type: application/json\r\n";
		stream << "connection: keep-alive\r\n" <<  "Content-length: ";
		stream << teststream.str().length() <<  "\r\n\r\n";

		js::write_json( stream, jsonmessage );

		m_responses->push(buf);
	}
private:
	boost::shared_ptr<socket_type> m_socket;
	boost::shared_ptr<boost::signals2::connection> m_connect;

	boost::shared_ptr<boost::asio::streambuf>	m_streambuf;
	boost::shared_ptr<avhttpd::request_opts>	m_request;

	boost::shared_ptr<
		boost::async_coro_queue<
			boost::circular_buffer_space_optimized<
				boost::shared_ptr<boost::asio::streambuf>
			>
		>
	> m_responses;

	void process_post( std::size_t bytestransfered );
};
}
