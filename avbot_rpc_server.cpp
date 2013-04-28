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

#include <boost/concept_check.hpp>
#include <boost/asio.hpp>

#include "boost/coro/coro.hpp"

#include "avbot_rpc_server.hpp"

#include "boost/coro/yield.hpp"

namespace detail{

avbot_rpc_server::http_request avbot_rpc_server::parse_http(std::size_t bytestransfered)
{
	std::string request_header;
	request_header.resize(bytestransfered);
	m_request->sgetn(&request_header[0], bytestransfered);

	std::stringstream strstream(request_header);
	std::string http_status_line;
	std::getline(strstream, http_status_line);

	strstream.str(http_status_line);
	// 解析 HTTP status line
	std::string http_request_cmd, http_request_path;
	strstream >> http_request_cmd >> http_request_path;

	boost::to_upper(http_request_cmd);
	// 检查是 GET 还是 PUSH
	if ( http_request_cmd == "POST" ){

		// 继续解析头部，最重要的是 content_length :)



		return HTTP_POST;
	}else{
		return HTTP_GET;
	}
}

// 数据操作跑这里，嘻嘻.
void avbot_rpc_server::operator()( boost::coro::coroutine coro, boost::system::error_code ec, std::size_t bytestransfered )
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

	http_request req ;

	reenter( &coro )
	{
		do
		{
			// 发起 HTTP 处理操作.
			_yield boost::asio::async_read_until( *m_socket, *m_request, "\r\n\r\n", boost::bind( *this, coro, _1, _2 ) );

			// 解析 HTTP
			req = parse_http( bytestransfered );

			if( req == HTTP_GET )
			{

				// 等待消息.
				if( m_responses->empty() )
				{
					if( !m_connect )
					{
						// 将自己注册到 avbot 的 signal 去
						// 等 有消息的时候，on_message 被调用，也就是下面的 operator() 被调用.
						_yield m_connect = boost::make_shared<boost::signals2::connection>
										   ( on_message.connect( boost::bind( *this, coro, _1 ) ) );
						// 就这么退出了，但是消息来的时候，om_message 被调用，然后下面的那个
						// operator() 就被调用了，那个 operator() 接着就会重新回调本 operator()
						// 结果就是随着 coroutine 的作用，代码进入这一行，然后退出  if 判定
						// 然后进入发送过程.
					}
					else
					{
						// 如果已经注册，直接返回。时候如果消息来了，on_message 被调用，也就
						// 是下面的 operator() 被调用. 结果就是随着 coroutine 的作用，代码
						// 进入上面那行，然后退出  if 判定。然后进入发送过程.
						return;
					}

					// signals2 回调的时候会进入到这一行.
				}

				// 进入发送过程
				sendbuf = m_responses->front();
				_yield boost::asio::async_write( *m_socket, *sendbuf, boost::bind( *this, coro, _1, _2 ) );
				m_responses->pop_front();
			}
			else if( req == HTTP_POST )
			{
				// 这里进入 POST 处理.
				// TODO
				// 解析 body, 不过其实最重要的是 content_length
				// 有了 content_length 才能知道消息有多长啊!

			}

			// 写好了，重新开始我们的处理吧!
		}
		while( 1 );
	}
}

}
