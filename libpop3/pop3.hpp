
#pragma once


#include <boost/signal.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include "boost/coro/coro.hpp"

#include "boost/timedcall.hpp"

struct mailcontent{
	std::string		from;
	std::string		to;
	std::string		subject;
	std::string		content_encoding; // base64 or 7bit
	// (content-type)/(base64 encoded content) pair of content
	std::vector< std::pair<std::string, std::string> > contents;
	// the matched or the so called, the best selected content type
	std::string		content;
};

class pop3 : boost::coro::coroutine {
public:
	typedef void result_type;
	typedef boost::signal< void (mailcontent thismail) > on_gotmail_signal;
public:
	pop3(::boost::asio::io_service & _io_service, std::string user, std::string passwd, std::string _mailserver="")
		:io_service(_io_service),
		m_mailaddr(user), m_passwd(passwd),
		m_mailserver(_mailserver),
		m_sig_gotmail(new on_gotmail_signal())
	{
		if(m_mailserver.empty()) // 自动从　mailaddress 获得.
		{
			if( m_mailaddr.find("@") == std::string::npos)
				m_mailserver = "pop.qq.com"; // 如果　邮箱是 qq 号码（没@），就默认使用 pop.qq.com .
			else
				m_mailserver =  std::string("pop.") + m_mailaddr.substr(m_mailaddr.find_last_of("@")+1);
		}
		io_service.post(boost::asio::detail::bind_handler(*this, boost::system::error_code(), 0));
	}

	void operator()(const boost::system::error_code & ec, std::size_t length =0);

	void connect_gotmail(const on_gotmail_signal::slot_type& slot)
	{
		m_sig_gotmail->connect(slot);
	}
private:
	void process_mail(std::istream &mail);
private:
	::boost::asio::io_service & io_service;

	std::string m_mailaddr,m_passwd,m_mailserver;
	// 必须是可拷贝的，所以只能用共享指针.
	boost::shared_ptr<boost::asio::ip::tcp::socket>	m_socket;
	boost::shared_ptr<boost::asio::streambuf>	m_streambuf;
	boost::shared_ptr<on_gotmail_signal>		m_sig_gotmail;
	std::vector<std::string>	maillist;
};
