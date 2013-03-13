
#pragma once

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/concept_check.hpp>
#include "boost/coro/coro.hpp"
#include "boost/coro/yield.hpp"

#include "boost/resolver.hpp"
#include "boost/timedcall.hpp"

class pop3 : boost::coro::coroutine {
public:
	typedef void result_type;
public:	
	pop3(boost::asio::io_service & _io_service, std::string user, std::string passwd)
		:io_service(_io_service),
		m_user(user), m_passwd(passwd),
		hosts( new std::vector<boost::asio::ip::tcp::endpoint>() )
	{
		io_service.post(*this);
	}
	// connect to pop.qq.com
	void operator()(const boost::system::error_code & ec = boost::system::error_code(), std::size_t length = 0)
	{
		using namespace boost::asio::ip;
		tcp::endpoint endpoint;
		std::string		line;
		std::istream	inbuffer(m_streambuf.get());
		reenter(this)
		{
restart:
			m_socket.reset();

			do{
				hosts->clear();
				// 延时 100ms
				_yield ::boost::delayedcallms(io_service,100, *this);
				// dns 解析.
				_yield ::boost::resolver<tcp>(io_service,tcp::resolver::query("pop.qq.com","110"),hosts, *this);

				// 失败了延时 10s
				if(ec)
					_yield ::boost::delayedcallsec(io_service,10, boost::bind(*this, ec, 0));
			}while(ec);// dns解析到成功为止!

			i = 0;

			do{
				// 一个一个尝试链接.
				endpoint = (*hosts)[i++];
				m_socket.reset(new boost::asio::ip::tcp::socket(io_service));
				_yield m_socket->async_connect(endpoint, *this);
			}while(ec && i < hosts->size());

			// 没连接上？　重试不　？
			if(ec){
				goto restart;
			}
			// 好了，连接上了.
			m_streambuf.reset(new boost::asio::streambuf);			
			// "+OK QQMail POP3 Server v1.0 Service Ready(QQMail v2.0)"
			_yield	boost::asio::async_read_until(*m_socket,*m_streambuf,"\n",*this);
			inbuffer >> line;
			if(line != "+OK"){
				// 失败，重试.
				goto restart;
			}

			// 发送用户名.
			_yield m_socket->async_write_some(boost::asio::buffer(std::string("user ")+ m_user +"\n"), *this);
			// 接受返回状态.
			m_streambuf.reset(new boost::asio::streambuf);
			_yield	boost::asio::async_read_until(*m_socket,*m_streambuf,"\n",*this);
			inbuffer >> line;

			// 解析是不是　OK.
			if(line != "+OK"){
				// 失败，重试.
				goto restart;
			}

			// 发送密码.
			_yield m_socket->async_write_some(boost::asio::buffer(std::string("pass ")+ m_passwd+"\n"), *this);
			// 接受返回状态.
			m_streambuf.reset(new boost::asio::streambuf);
			_yield	boost::asio::async_read_until(*m_socket,*m_streambuf,"\n",*this);
			inbuffer >> line;
			// 解析是不是　OK.
			if(line != "+OK"){
				// 失败，重试.
				goto restart;
			}

			// 完成登录. 开始接收邮件.
			
			// 发送　list 命令.
			
			

		}
	}
private:
	int i;
	boost::asio::io_service & io_service;
	
	std::string m_user,m_passwd;
	// 必须是可拷贝的，所以只能用共享指针.
	boost::shared_ptr<boost::asio::ip::tcp::socket>	m_socket;
	boost::shared_ptr<boost::asio::streambuf>	m_streambuf;
	// resolved hosts to connect
	boost::shared_ptr<std::vector<boost::asio::ip::tcp::endpoint> > hosts;
};

#include  "boost/coro/unyield.hpp"
