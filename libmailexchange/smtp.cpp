
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/base64.hpp>

#include "smtp.hpp"
namespace mx{
namespace detail{
smtp::smtp(boost::asio::io_service& _io_service, std::string user, std::string passwd, std::string _mailserver)
	:io_service(_io_service),
	m_mailaddr(user), m_passwd(passwd),
	m_mailserver(_mailserver)
{
	// 计算 m_AUTH
	std::vector<char> authtoken(user.length() + passwd.length() + 2);
	std::copy(user.begin(), user.end(), &authtoken[1]);
	std::copy(passwd.begin(), passwd.end(), &authtoken[user.length()+2]);

	//ADQ2NDg5MzQ5MAB0ZXN0cGFzc3dk
	m_AUTH = boost::base64_encode(std::string(authtoken.data(), authtoken.size()));

    if(m_mailserver.empty()) // 自动从　mailaddress 获得.
    {
        if( m_mailaddr.find("@") == std::string::npos)
            m_mailserver = "smtp.qq.com"; // 如果　邮箱是 qq 号码（没@），就默认使用 smtp.qq.com .
        else
            m_mailserver =  std::string("smtp.") + m_mailaddr.substr(m_mailaddr.find_last_of("@")+1);
    }	
}

}

smtp::smtp(boost::asio::io_service& _io_service, std::string user, std::string passwd, std::string _mailserver)
  : impl_smtp(_io_service, user, passwd, _mailserver)
{
}

}