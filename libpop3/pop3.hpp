
#pragma once


#include <boost/signal.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include "boost/coro/coro.hpp"

#include "boost/resolver.hpp"
#include "boost/timedcall.hpp"

struct mailcontent{
	std::string		from;
	std::string		to;
	std::string		subject;
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
	pop3(::boost::asio::io_service & _io_service, std::string user, std::string passwd)
		:io_service(_io_service),
		m_user(user), m_passwd(passwd),
		hosts( new std::vector<boost::asio::ip::tcp::endpoint>() ),
		m_sig_gotmail(new on_gotmail_signal())
	{
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
	int i;
	::boost::asio::io_service & io_service;

	std::string m_user,m_passwd;
	// 必须是可拷贝的，所以只能用共享指针.
	boost::shared_ptr<boost::asio::ip::tcp::socket>	m_socket;
	boost::shared_ptr<boost::asio::streambuf>	m_streambuf;
	boost::shared_ptr<on_gotmail_signal>		m_sig_gotmail;
	std::vector<std::string>	maillist;
	
	// resolved hosts to connect
	boost::shared_ptr<std::vector<boost::asio::ip::tcp::endpoint> > hosts;
};
