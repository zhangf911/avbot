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
#include <boost/lexical_cast.hpp>

#include "boost/avloop.hpp"
#include "boost/coro/coro.hpp"

#include "avbot_rpc_server.hpp"

#include "boost/coro/yield.hpp"

namespace detail{

static const char hex_chars[] = "0123456789abcdef";

inline bool is_char(int c)
{
	return c >= 0 && c <= 127;
}

inline bool is_digit(char c)
{
	return c >= '0' && c <= '9';
}

inline bool is_ctl(int c)
{
	return (c >= 0 && c <= 31) || c == 127;
}

inline bool is_tspecial(int c)
{
	switch (c)
	{
	case '(': case ')': case '<': case '>': case '@':
	case ',': case ';': case ':': case '\\': case '"':
	case '/': case '[': case ']': case '?': case '=':
	case '{': case '}': case ' ': case '\t':
		return true;
	default:
		return false;
	}
}

inline bool tolower_compare(char a, char b)
{
	return std::tolower(a) == std::tolower(b);
}

inline bool headers_equal(const std::string &a, const std::string &b)
{
	if (a.length() != b.length())
		return false;
	return std::equal(a.begin(), a.end(), b.begin(), tolower_compare);
}

inline void check_header(const std::string &name, const std::string &value,
	std::string &content_type, std::size_t &content_length,
	std::string &location)
{
	if (headers_equal(name, "Content-Type"))
		content_type = value;
	else if (headers_equal(name, "Content-Length"))
		content_length = boost::lexical_cast<std::size_t>(value);
	else if (headers_equal(name, "Location"))
		location = value;
}

template <typename Iterator>
bool parse_http_headers(Iterator begin, Iterator end,
	std::string &content_type, std::size_t &content_length,
	std::string &location)
{
	enum
	{
		first_header_line_start,
		header_line_start,
		header_lws,
		header_name,
		space_before_header_value,
		header_value,
		linefeed,
		final_linefeed,
		fail
	} state = first_header_line_start;

	Iterator iter = begin;
	std::string reason;
	std::string name;
	std::string value;
	while (iter != end && state != fail)
	{
		char c = *iter++;
		switch (state)
		{
		case first_header_line_start:
			if (c == '\r')
				state = final_linefeed;
			else if (!is_char(c) || is_ctl(c) || is_tspecial(c))
				state = fail;
			else
			{
				name.push_back(c);
				state = header_name;
			}
			break;
		case header_line_start:
			if (c == '\r')
			{
				boost::trim(name);
				boost::trim(value);
				check_header(name, value, content_type, content_length, location);
				name.clear();
				value.clear();
				state = final_linefeed;
			}
			else if (c == ' ' || c == '\t')
				state = header_lws;
			else if (!is_char(c) || is_ctl(c) || is_tspecial(c))
				state = fail;
			else
			{
				boost::trim(name);
				boost::trim(value);
				check_header(name, value, content_type, content_length, location);
				name.clear();
				value.clear();
				name.push_back(c);
				state = header_name;
			}
			break;
		case header_lws:
			if (c == '\r')
				state = linefeed;
			else if (c == ' ' || c == '\t')
				; // Discard character.
			else if (is_ctl(c))
				state = fail;
			else
			{
				state = header_value;
				value.push_back(c);
			}
			break;
		case header_name:
			if (c == ':')
				state = space_before_header_value;
			else if (!is_char(c) || is_ctl(c) || is_tspecial(c))
				state = fail;
			else
				name.push_back(c);
			break;
		case space_before_header_value:
			if (c == ' ')
				state = header_value;
			else if (is_ctl(c))
				state = fail;
			else
			{
				value.push_back(c);
				state = header_value;
			}
			break;
		case header_value:
			if (c == '\r')
				state = linefeed;
			else if (is_ctl(c))
				state = fail;
			else
				value.push_back(c);
			break;
		case linefeed:
			state = (c == '\n') ? header_line_start : fail;
			break;
		case final_linefeed:
			return (c == '\n');
		default:
			return false;
		}
	}
	return false;
}


avbot_rpc_server::http_request avbot_rpc_server::parse_http(std::size_t bytestransfered)
{
	std::string request_header;
	request_header.resize(bytestransfered);
	m_request->sgetn(&request_header[0], bytestransfered);

	std::stringstream strstream(request_header);
	std::string http_status_line;
	std::getline(strstream, http_status_line);

	std::stringstream strstream2(http_status_line);
	// 解析 HTTP status line
	std::string http_request_cmd, http_request_path;
	strstream2 >> http_request_cmd >> http_request_path;

	boost::to_upper(http_request_cmd);
	// 检查是 GET 还是 PUSH
	if ( http_request_cmd == "POST" ){

		// 继续解析头部，最重要的是 content_length :)
// 		// strstream
		std::string headers = strstream.str();
		std::string location;
		parse_http_headers(headers.begin(), headers.end(), m_request_content_type, m_request_content_length, location);
		return HTTP_POST;
	}else{
		return HTTP_GET;
	}
}

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
	m_request->sgetn( &messagebody[0], bytestransfered );
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

struct readbody_completion_condition{

	boost::shared_ptr<boost::asio::streambuf> _buf;
	std::size_t _s;

	readbody_completion_condition(boost::shared_ptr<boost::asio::streambuf> buf, std::size_t s)
	  : _buf(buf), _s(s){}

	std::size_t operator()(const boost::system::error_code &, std::size_t)
	{
		return _s -  _buf->size();
	}
};

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

	//for (;;)
	reenter ( &coro )
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
			// 解析 body, 不过其实最重要的是 content_length
			// 有了 content_length 才能知道消息有多长啊!
			if (m_request_content_length ==  0)
			{
				// 没有 content_length 的 POST! 不支持！ 哼
				using namespace boost::system::errc;
				_yield avloop_idle_post(m_socket->get_io_service(), boost::bind( *this, coro, make_error_code(protocol_error), 0 ));
				m_connect->disconnect();
				return;
			}
			// 读取 body

			boost::asio::async_read(*m_socket, *m_request,
				readbody_completion_condition(m_request, m_request_content_length),
				boost::bind( *this, coro,  boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
			);
			// body 必须是合法有效的 JSON 格式
			process_post(bytestransfered);
		}

		// 这个步骤重新创建了一个新的 coro 对象，导致 reenter 会重新执行.
		_yield avloop_idle_post(m_socket->get_io_service(), boost::bind( *this, boost::coro::coroutine(), boost::system::error_code(), 0 ));
	}
}

}
