
#pragma once


#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include "boost/coro/yield.hpp"

#include "boost/timedcall.hpp"
#include "avproxy.hpp"

#include "internet_mail_format.hpp"

#include "pop3.hpp"
#include "smtp.hpp"

namespace mx{

// -------------------------

// 用于邮件的发送 & 接收.
class mx{
	pop3 m_pop3;
	smtp m_smtp;
public:
	mx(::boost::asio::io_service & _io_service, std::string user, std::string passwd, std::string _pop3server="", std::string _smtpserver="")
		:m_pop3(_io_service, user, passwd, _pop3server), 
		m_smtp(_io_service, user, passwd, _smtpserver)
	{
	}

};

}