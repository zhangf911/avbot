/*
 * Copyright (C) 2013  microcai <microcai@fedoraproject.org>
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
#include <boost/asio/streambuf.hpp>
#include <avhttp.hpp>

#include <boost/coro/coro.hpp>

#include "joke.hpp"

#include <boost/coro/yield.hpp>

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
			jokestring.append( message.substr( message.find( ">" ), ( message.rfind( "<" ) - message.find( ">" ) - 1 ) ) );
			jokestring.append( "\r\n" );
			continue;
		}

		if( message.find( jokemessagestart ) != std::string::npos )
		{
			std::getline( response_stream, message );

			while( message.find( "\r" ) != std::string::npos )
				message.erase( message.find( "\r" ), 1 );

			while( message.find( "&nbsp;" ) != std::string::npos )
				message.erase( message.find( "&nbsp;" ), 6 );

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

			jokestring.append( message );
		}
	}
	if (charset.empty())
		charset = "7bit";
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
		boost::mt19937 rannum(time(0));
		int num = 1 + rannum() % 20000;
		int numcl = num / 1000 + 1;

		std::string url = boost::str(boost::format("http://xiaohua.zol.com.cn/detail%d/%d.html") % numcl % num);

		m_http_stream.reset(new avhttp::http_stream(io_service));

		m_http_stream->async_open(url,  boost::bind(*this, _1, handler));
	}

	template<class Handler>
	void operator()(const boost::system::error_code &ec, Handler handler)
	{
		using namespace boost::system;
		using namespace boost::asio::detail;

		if (ec){
			io_service.post( bind_handler(handler, ec, std::string("获取笑话出错")) );
		}else{
			m_read_buf.reset(new boost::asio::streambuf);
			boost::asio::async_read(*m_http_stream, *m_read_buf, boost::asio::transfer_all(), boost::bind(*this, _1, _2, handler));
		}
	}

	template<class Handler>
	void operator()(const boost::system::error_code &ec, std::size_t bytes_transferred, Handler handler)
	{
		using namespace boost::system;
		using namespace boost::asio::detail;
		std::istream htmlstream(m_read_buf.get());

		std::string jokestr = get_joke_content(htmlstream);

		io_service.post( bind_handler(handler, error_code(), jokestr) );
	}


};

void joke::set_joke_fecher()
{
	m_async_jokefecher = jokefecher(io_service);
}

void joke::operator()( const boost::system::error_code& error, std::string joke )
{
	reenter( this )
	{
		// 发送 joke
		m_sender(joke);//"到时了，发送 joke" );
		// 发一个 joke,  heh
	}
}

void joke::operator()(const boost::system::error_code& error )
{
	if (error){
		// timer cancled, give up
		return;
	}

	m_async_jokefecher( *this );
	// 重启自己.
	start();
}


void joke::operator()( boost::property_tree::ptree msg )
{
	try
	{
		if( msg.get<std::string>( "channel" ) == m_channel_name )
		{
			start();
		}// else ignore other channel message
	}
	catch( const boost::property_tree::ptree_error & err )
	{

	}
}

void joke::start()
{
	m_timer->expires_from_now(m_interval);
	m_timer->async_wait(*this);
}
