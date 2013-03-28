#pragma once

#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include "boost/coro/coro.hpp"
#include "boost/coro/yield.hpp"
#include "boost/timedcall.hpp"
#include "avproxy.hpp"

#include "internet_mail_format.hpp"

class smtp : private boost::coro::coroutine {
	boost::asio::io_service & io_service;
	std::string m_mailaddr,m_passwd,m_mailserver;
	std::string m_AUTH;
	
	// 必须是可拷贝的，所以只能用共享指针.
	boost::shared_ptr<boost::asio::ip::tcp::socket>	m_socket;

	// for operator()()
	int retry_count;

public:
	smtp(::boost::asio::io_service & _io_service, std::string user, std::string passwd, std::string _mailserver="");

	// ---------------------------------------------

	// 使用 async_sendmail 异步发送一份邮件.邮件的内容由 InternetMailFormat 指定.
	// 请不要对 InternetMailFormat 执行 base64 编码, async_sendmail 发现里面包含了
	// 非ASCII编码的时候, 就会对内容执行base64编码.
	// 所以也请不要自行设置 content-transfer-encoding
	template<class Handler>
	void async_sendmail(InternetMailFormat imf, Handler handler)
	{
 		io_service.post(boost::bind(*this, boost::system::error_code(), 0, handler));
	}

	template<class Handler>
	void operator()(boost::system::error_code ec, std::size_t bytetransfered, Handler handler)
	{
 		using namespace boost::asio;

		reenter(this)
		{
			retry_count = 0;
  			m_socket.reset( new ip::tcp::socket(io_service));

  			do{
				// 首先链接到服务器. dns 解析并连接.
				_yield avproxy::async_proxy_connect(
						avproxy::autoproxychain(*m_socket, ip::tcp::resolver::query(m_mailserver, "25")),
						boost::bind(*this, _1, 0, handler));
					if (ec){
						// 连接失败, 重试次数到达极限 ...
						retry_count ++;
						if (retry_count >= 20)
						{
							// 到极限了, 返回错误好了.
							io_service.post(boost::asio::detail::bind_handler(handler, ec));
							return ;
						}
						// 延时 retry_count * 10 s
						_yield ::boost::delayedcallsec( io_service, retry_count * 10, boost::bind(*this, ec, 0, handler) );
					}
			}while (ec);

			// 发送 EHLO 执行登录.
			_yield m_socket->async_write_some ( buffer ( std::string ( "EHLO " ) + m_mailserver  + "\r\n" ), boost::bind(*this, _1, _2, handler) );

			// 发送 AUTH PLAIN XXX 登录认证.

			// 进入邮件发送过程.
			
			// 如果失败, 等待 retry_count * 40 + 60 s 后, 重试
		}
	}
	typedef void result_type;
};

