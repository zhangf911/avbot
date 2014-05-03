
#ifndef iplocation_h__
#define iplocation_h__

#include <boost/regex.hpp>
#include <boost/make_shared.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>

#include <avhttp.hpp>
#include <avhttp/async_read_body.hpp>

#include "extension.hpp"
#include "qqwry/ipdb.hpp"

class iplocation : avbotextension
{
	// 这么多 extension 的 qqwry 数据库当然得共享啦！ 共享那肯定就是用的共享指针.
	boost::shared_ptr<QQWry::ipdb> m_ipdb;
public:
	template<class MsgSender>
	iplocation(boost::asio::io_service & _io_service, MsgSender sender, std::string channel_name, boost::shared_ptr<QQWry::ipdb> ipdb)
		: avbotextension(_io_service, sender, channel_name)
		, m_ipdb(ipdb)
	{
	}

	void operator()(boost::property_tree::ptree msg)
	{
 		if (!m_ipdb)
		{
			return;
		}

		if (msg.get<std::string>("channel") != m_channel_name)
			return;

		std::string textmsg = boost::trim_copy(msg.get<std::string>("message.text"));

		in_addr ipaddr;

		boost::cmatch what;

		if (
			boost::regex_search(
				textmsg.c_str(),
				what, 
				boost::regex(
					"((((\\d{1,2})|(1\\d{2})|(2[0-4]\\d)|(25[0-5]))\\.){3}((\\d{1,2})|(1\\d{2})|(2[0-4]\\d)|(25[0-5])))"
				)
			)
		)
		{
			std::string matchedip = what[1];
			ipaddr.s_addr = ::inet_addr(matchedip.c_str());
			// ip 地址是这样查询的 .qqbot locate 8.8.8.8
			// 或者直接聊天内容就是完整的一个 ip 地址
			QQWry::IPLocation l = m_ipdb->GetIPLocation(ipaddr);
			
			// 找到后，发给聊天窗口.

			m_sender(boost::str(
				boost::format("你在聊天中提到的 %s 地址，经过 avbot 仔细的考察，发现它在 %s %s")
				% matchedip
				% local_encode_to_utf8(l.country)
				% local_encode_to_utf8(l.area)
			));

		}

		// 或者通过其他方式激活

	}

};

#endif // iplocation_h__