/*
 * Copyright (C) 2013  microcai <microcai@fedoraproject.org>
 * Copyright (C) 2013  ericsimith <ericsimith@hotmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * libjoke 使用 avhttp 从笑话网站获取笑话，然后贴到群里.
 */

#include <boost/shared_ptr.hpp>
#include <boost/random.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include <avhttp.hpp>
#include <avhttp/async_read_body.hpp>
#include <limits>

#include "joke.hpp"
#include "html.hpp"

static std::string get_joke_content(std::istream &response_stream )
{
	std::string joketitlestart = "lastT lan14b";
	std::string jokemessagestart = "class=\"lastC\"";

	std::string message;
	std::string jokestring;
	std::string charset;

	while( std::getline( response_stream, message ) )
	{
		if (charset.empty() && message.find("charset=") != std::string::npos)
		{
			std::string tmp =  message.substr(message.find("charset="));

			// 从 charset=gb2312" /> 中获取 gb2312
			char _charset[256] = {0};

			std::sscanf(tmp.c_str(), "charset=%[^\"]", _charset);
			charset = _charset;
			continue;
		}else if( message.find( joketitlestart ) != std::string::npos )
		{
			jokestring.append( html_unescape(message.substr( message.find( ">" ), ( message.rfind( "<" ) - message.find( ">" ) - 1 ) ) ) );
			jokestring.append( "\r\n" );
			continue;
		}

		if( message.find( jokemessagestart ) != std::string::npos )
		{
			do{
				std::getline( response_stream, message );
				if ( message.find( "</div>" ) != std::string::npos )
					break;

				while( message.find( "\r" ) != std::string::npos )
					message.erase( message.find( "\r" ), 1 );

				while( message.find( "\t" ) != std::string::npos )
					message.erase( message.find( "\t" ), 1 );

				while( message.find( " " ) != std::string::npos )
					message.erase( message.find( " " ), 1 );

				while( message.find( "\n" ) != std::string::npos )
					message.erase( message.find( "\n" ), 1 );

				while( message.find( "<br><br>" ) != std::string::npos )
					message.replace( message.find( "<br><br>" ), 8, "\r\n" );

				while( message.find( "<br>" ) != std::string::npos )
					message.replace( message.find( "<br>" ), 4, "\r\n" );

				jokestring.append( html_unescape(message) );
			} while ( message.find( "</div>" ) == std::string::npos );
			// 笑话内容已完
			break;
		}
	}
	
	// 去除多余的空行
	while( jokestring.find( "\x20\r\n" ) != std::string::npos )
		jokestring.replace( jokestring.find( "\x20\r\n" ), 3, "\r\n" );
	while( jokestring.find( "\r\n\r\n" ) != std::string::npos )
		jokestring.replace( jokestring.find( "\r\n\r\n" ), 4, "\r\n" );

	if (charset.empty())
		charset = "7bit";
	if (charset == "gb2132"){
		charset = "gb18030";
	}
	return boost::locale::conv::between(jokestring, "UTF-8", charset);
}

class jokefecher{
	boost::asio::io_service & io_service;
	boost::shared_ptr<avhttp::http_stream>	m_http_stream;
	boost::shared_ptr<boost::asio::streambuf> m_read_buf;
public:
	typedef void result_type;

	jokefecher(boost::asio::io_service &_io_service)
	  : io_service(_io_service)
	{
	}

	template<class Handler>
	void operator()(Handler handler)
	{
		// 第一步，构造一个 URL, 然后调用 avhttp 去读取.
		static boost::mt19937 rannum(std::time(0));
		int num = 1 + rannum() % 20000;
		int numcl = num / 1000 + 1;

		std::string url = boost::str(boost::format("http://xiaohua.zol.com.cn/detail%d/%d.html") % numcl % num);

		m_http_stream.reset(new avhttp::http_stream(io_service));
		m_read_buf.reset(new boost::asio::streambuf);

		avhttp::async_read_body(*m_http_stream, url, *m_read_buf, boost::bind(*this, _1, _2, handler));
	}

	template<class Handler>
	void operator()(const boost::system::error_code &ec, std::size_t bytes_transferred, Handler handler)
	{
		using namespace boost::system;
		using namespace boost::asio::detail;

		if (ec){
			io_service.post( bind_handler(handler, ec, std::string("获取笑话出错")) );
			return;
		}

		std::istream htmlstream(m_read_buf.get());

		try{
			std::string jokestr = get_joke_content(htmlstream);
			io_service.post( bind_handler(handler, error_code(), jokestr) );
		}catch (const boost::locale::conv::invalid_charset_error &err){
			// 应该重新开始, 而不是把这个错误的编码帖上去.
			(*this)(handler);
		}
	}
};

void joke::set_joke_fecher()
{
	m_async_jokefecher = jokefecher(io_service);
}

void joke::operator()( const boost::system::error_code& error, std::string joke )
{
	// 发送 joke
	m_sender(joke);

	// 重启自己.
	start();
}

void joke::operator()(const boost::system::error_code& error )
{
	if (error){
		// timer cancled, give up
		return;
	}

	m_async_jokefecher( *this );
}

static bool can_joke(std::string msg)
{
	if (msg == ".qqbot joke")
		return true;
#ifndef _MSC_VER
	if (msg == ".qqbot 给大爷来一个笑话")
		return true;
	if ( (msg.find("大爷") != std::string::npos) && (msg.find("笑话")!= std::string::npos) )
		return true;
#endif
	return false;
}

void joke::operator()( boost::property_tree::ptree msg )
{
	try
	{
		if( msg.get<std::string>( "channel" ) == m_channel_name )
		{
			// do joke control here
			std::string textmsg = boost::trim_copy( msg.get<std::string>( "message.text" ) );

			if( textmsg ==  ".qqbot joke off" )
			{
				// 其实关闭不掉的, 就是延长到 24 个小时了, 嘻嘻.
				* m_interval = boost::posix_time::seconds( 3600 * 24 );
				m_sender( "笑话关闭." );
				save_setting();
			}
			else if( textmsg == ".qqbot joke on" )
			{

				* m_interval = boost::posix_time::seconds( 600 );

				m_sender( "笑话开启." );
				save_setting();
			}
			else if (can_joke(textmsg)){
				m_timer->expires_from_now(boost::posix_time::seconds(2));
				m_timer->async_wait(*this);
				return;
			}
			else
			{
				// .qqbot joke interval XXX
				boost::cmatch what;
				boost::regex ex( "\\.qqbot joke interval (.*)" );

				if( boost::regex_match( textmsg.c_str(), what, ex ) )
				{
					try
					{
						int sec =  boost::lexical_cast<int>( what[1] );

						if( sec < 10 )
						{
							m_sender( boost::str( boost::format( "混蛋, %d 秒太短了!" ) % sec ) );
						}
						else
						{
							* m_interval = boost::posix_time::seconds( sec );
							m_sender( boost::str( boost::format( "笑话间隔为 %d 秒." ) % sec ) );
							save_setting();
						}
					}
					catch( const boost::bad_lexical_cast & err ) {}
				}
			}
		}// else ignore other channel message
	}
	catch( const boost::property_tree::ptree_error & err )
	{

	}
	boost::system::error_code ec;
	// 如果已经超时了, 只不过是在 fetch 笑话网页, 就不要在这里无意间重启了 timer.
	if (m_timer->cancel(ec)>0)
		start();
}

void joke::start()
{
	m_timer->expires_from_now(*m_interval);
	m_timer->async_wait(*this);
}

void joke::load_setting()
{
	boost::property_tree::ptree set;

	try
	{
		std::ifstream settings( (std::string( "joke_setting_" ) + m_channel_name).c_str() );

		boost::property_tree::json_parser::read_json( settings, set );

		*m_interval = boost::posix_time::seconds( set.get<int>( "interval" ) );
	}
	catch( const std::exception & err )
	{
	}
}

void joke::save_setting()
{
	boost::property_tree::ptree set;
	set.put<std::size_t>("interval", m_interval->total_seconds());

	try
	{
		std::ofstream settings( (std::string( "joke_setting_" ) + m_channel_name).c_str() );

		boost::property_tree::json_parser::write_json( settings, set );
	}
	catch( const std::exception & err )
	{

	}
}


