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

namespace detail
{

// avbot_rpc_server 由 acceptor_server 这个辅助类调用
// 为其构造函数传入一个 m_socket, 是 shared_ptr 的.
class avbot_rpc_server
{
	enum http_request
	{
		HTTP_GET = 1,
		HTTP_POST = 1,
	};
public:
	typedef boost::signals2::signal <
	void( boost::property_tree::ptree )
	> on_message_signal_type;
	on_message_signal_type & on_message;

	typedef boost::asio::ip::tcp Protocol;
	typedef boost::asio::basic_stream_socket<Protocol> socket_type;
	typedef void result_type;

	avbot_rpc_server( boost::shared_ptr<socket_type> _socket, on_message_signal_type & _on_message )
		: m_socket( _socket )
		, m_request( new boost::asio::streambuf )
		, m_responses( new boost::circular_buffer_space_optimized<boost::shared_ptr<boost::asio::streambuf> >( 20 ) )
		, on_message( _on_message )
	{
		m_socket->get_io_service().post(
			boost::asio::detail::bind_handler( *this, boost::coro::coroutine(), boost::system::error_code(), 0 )
		);
	}

	// 数据操作跑这里，嘻嘻.
	void operator()( boost::coro::coroutine coro, boost::system::error_code ec, std::size_t bytestransfered );

	// signal 的回调到了这里, 这里我们要区分对方是不是用了 keep-alive 呢.
	void operator()( boost::coro::coroutine coro, const boost::property_tree::ptree & jsonmessage )
	{
		boost::shared_ptr<boost::asio::streambuf> buf( new boost::asio::streambuf );
		std::ostream	stream( buf.get() );
		std::stringstream	teststream;

		js::write_json( teststream,  jsonmessage );

		// 直接写入 json 格式的消息吧!
		stream <<  "HTTP/1.1 200 OK\r\n" <<  "Content-type: application/json\r\n";
		stream <<  "connection: keep-alive\r\n" <<  "Content-length: ";
		stream << teststream.str().length() <<  "\r\n\r\n";

		js::write_json( stream, jsonmessage );

		// 检查 发送缓冲区.
		if( m_responses->empty() )
		{
			// 打通仁督脉.
			m_socket->get_io_service().post( boost::asio::detail::bind_handler( *this, coro, boost::system::error_code(), 0 ) );
		}

		// 写入 m_responses
		m_responses->push_back( buf );
	}
private:
	boost::shared_ptr<socket_type> m_socket;
	boost::shared_ptr<boost::signals2::connection> m_connect;

	boost::shared_ptr<boost::asio::streambuf>	m_request;
	std::size_t									m_request_content_length;
	std::string									m_request_content_type;

	boost::shared_ptr< boost::circular_buffer_space_optimized<boost::shared_ptr<boost::asio::streambuf> > >	m_responses;

	http_request parse_http( std::size_t );
	void process_post( std::size_t bytestransfered );
};
}

#include "boost/coro/unyield.hpp"
