/*
 * <one line to give the program's name and a brief idea of what it does.>
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

#include <boost/json_create_escapes_utf8.hpp>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <avhttp/detail/parsers.hpp>

#include "boost/avloop.hpp"
// #include <boost/asio/yield.hpp>

#include "avbot_rpc_server.hpp"

namespace detail{

/**
 * avbot rpc 接受的 JSON 格式为
 *
 * 	{
		"protocol":"rpc",
		"channel":"",  // 留空表示所有频道广播
		"message":"text message"
	}
 */
void avbot_rpc_server::process_post( std::size_t bytestransfered )
{
	pt::ptree msg;
	std::string messagebody;
	messagebody.resize( bytestransfered );
	m_streambuf->sgetn( &messagebody[0], bytestransfered );
	std::stringstream jsonpostdata( messagebody );

	try
	{
		// 读取 json
		js::read_json( jsonpostdata, msg );
	}
	catch( const js::json_parser_error & err )
	{
		// 数据不是 json 格式，视作 纯 TEXT 格式.
		msg.put( "protocol", "rpc" );
		msg.put( "channel", "" );
		msg.put( "message", messagebody );
	}
	catch( const pt::ptree_error &err )
	{
		// 其他错误，忽略.
	}

	try
	{
		on_message( msg );
	}
	catch( const pt::ptree_error &err )
	{
		// 忽略.
	}

}

// 发送数据在这里
void avbot_rpc_server::operator()(boost::asio::coroutine coro, boost::system::error_code ec, boost::shared_ptr< boost::asio::streambuf > v)
{
	reenter(coro){
		yield boost::asio::async_write(*m_socket, *v, boost::bind<void>(*this, coro, _1, v));
		(*this)(ec, 0);
	}
}

// 数据操作跑这里，嘻嘻.
void avbot_rpc_server::operator()(boost::system::error_code ec, std::size_t bytestransfered )
{
	boost::shared_ptr<boost::asio::streambuf> sendbuf;

	if( ec )
	{
		m_socket->close( ec );

		// 看来不是 HTTP 客户端，诶，滚蛋啊！
		// 沉默，直接关闭链接. 取消信号注册.
		if( m_connect && m_connect->connected() )
			m_connect->disconnect();

		return;
	}

	//for (;;)
	reenter (this)
	{for (;;){
		// 发起 HTTP 处理操作.
		yield avhttpd::async_read_request(*m_socket, *m_streambuf, *m_request, boost::bind<void>(*this, _1, 0));

		// 解析 HTTP
		if(m_request->find(avhttpd::http_options::request_method) == "GET" )
		{
			// 等待消息, 并发送.
			yield m_responses->async_pop(boost::bind<void>(*this, boost::asio::coroutine(), ec, _1));
		}
		else if( m_request->find(avhttpd::http_options::request_method) == "POST")
		{
			// 这里进入 POST 处理.
			// 读取 body
			yield boost::asio::async_read(
				*m_socket, *m_streambuf,
				boost::asio::transfer_exactly(
					boost::lexical_cast<std::size_t>(m_request->find(avhttpd::http_options::content_length))
				),
				*this
			);
			// body 必须是合法有效的 JSON 格式
			process_post(bytestransfered);
		}

		// 继续
		yield avloop_idle_post(m_socket->get_io_service(), boost::bind<void>( *this, ec, 0 ));
	}}
}

}
