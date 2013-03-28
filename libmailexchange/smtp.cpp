
#include <boost/asio.hpp>

#include "smtp.hpp"

smtp::smtp(boost::asio::io_service& _io_service, std::string user, std::string passwd, std::string _mailserver)
	:io_service(_io_service),
	m_mailaddr(user), m_passwd(passwd),
	m_mailserver(_mailserver)
{

}
