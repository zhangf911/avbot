
#pragma once

#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include "boost/coro/coro.hpp"
#include "boost/coro/yield.hpp"

#include "boost/resolver.hpp"
#include "boost/timedcall.hpp"

class pop3 : boost::coro::coroutine {
public:
	pop3(boost::asio::io_service & _io_service, std::string user, std::string passwd)
		:io_service(_io_service), m_socket(new boost::asio::ip::tcp::socket(io_service)),
		i(0), m_user(user), m_passwd(passwd)
	{
		io_service.post(*this);
	}

	// connect to pop.qq.com
	void operator()(const boost::system::error_code & ec = boost::system::error_code(), std::size_t length = 0)
	{
		using namespace boost::asio::ip;
		tcp::endpoint endpoint;
		reenter(this)
		{
			do{
				hosts.clear();
				// 延时 100ms
				coyield ::boost::delayedcallms(io_service,100, *this);
				// dns 解析.
				coyield ::boost::resolver<tcp>(io_service,tcp::resolver::query("pop.qq.com","110"),hosts, *this);

				// 失败了延时 10s
				if(ec)
					coyield ::boost::delayedcallsec(io_service,10, *this);
			}while(ec);// dns解析到成功为止!

			i = 0;

			do{
				// 一个一个尝试链接.
				endpoint = hosts[i++];
				coyield m_socket->async_connect(endpoint, *this);
			}while(ec && i < hosts.size());

			// 没连接上？　重试不　？
			if(ec){
				pop3 p(io_service, m_user, m_passwd);
				return;
			}
			// 好了，连接上了.
			coyield m_socket->async_write_some(boost::asio::buffer(std::string("user ")+ m_user), *this);
		}
	}
private:
	int i;
	boost::asio::io_service & io_service;
	
	std::string m_user,m_passwd;
	// 必须是可拷贝的，所以只能用共享指针.
	boost::shared_ptr<boost::asio::ip::tcp::socket>	m_socket;
	// resolved hosts to connect
	std::vector<boost::asio::ip::tcp::endpoint> hosts;
};

#include  "boost/coro/unyield.hpp"
