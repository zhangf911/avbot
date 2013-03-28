#pragma once

#include <string>
#include <boost/asio/io_service.hpp>

class smtp{
	boost::asio::io_service & io_service;
	std::string m_mailaddr,m_passwd,m_mailserver;

public:
	smtp(::boost::asio::io_service & _io_service, std::string user, std::string passwd, std::string _mailserver="");
};

