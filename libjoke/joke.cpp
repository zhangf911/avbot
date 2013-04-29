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

#include <boost/random.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/coro/coro.hpp>

#include "joke.hpp"

#include <boost/coro/yield.hpp>

class jokefecher{
	boost::asio::io_service & io_service;
	boost::function<void (const boost::system::error_code &, std::string)>	m_handler;
public:
	jokefecher(boost::asio::io_service &_io_service)
	  : io_service(_io_service)
	{

	}

	template<class Handler>
	void operator()(Handler handler)
	{
		m_handler = handler;

		using namespace boost::system;
		using namespace boost::asio::detail;
		// 第一步，构造一个 URL, 然后调用 avhttp 去读取.
		boost::mt19937 rannum(time(0));
		int num = 1 + rannum() % 20000;
		int numcl = num / 1000 + 1;

		std::string url = boost::str(boost::format("http://xiaohua.zol.com.cn/detail/%d/%d.html") % numcl % num);

		std::string jokestr = boost::str(boost::format("到 %s 看笑话 ") % url);

		io_service.post( bind_handler(m_handler, error_code(), jokestr) );
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
