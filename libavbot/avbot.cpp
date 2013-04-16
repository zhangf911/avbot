#include <boost/regex.hpp>
#include <boost/throw_exception.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/format.hpp>
#include "avbot.hpp"
#include "libwebqq/url.hpp"

avbot::avbot( boost::asio::io_service& io_service )
  : m_io_service(io_service), fetch_img(false)
{
	base_image_url = "http://w.qq.com/cgi-bin/get_group_pic?pic=";

}

void avbot::callback_on_irc_message( irc::IrcMsg pMsg )
{
	using boost::property_tree::ptree;
	// formate irc message to JSON and call on_message

	ptree message;
	message.put("protocol", "irc");
	message.put("room", pMsg.from);
	message.put("who", pMsg.whom);

	ptree textmsg;
	textmsg.add("text", pMsg.msg);
	message.add_child("message", textmsg);

	// TODO 将 识别的 URL 进行转化.

	on_message(message);
}

void avbot::callback_on_qq_group_message( std::string group_code, std::string who, const std::vector< qqMsg >& msg )
{
	using boost::property_tree::ptree;
	ptree message;
	message.put("protocol", "qq");
	ptree ptreee_group;
	ptreee_group.add("code", group_code);

	qqGroup_ptr group = m_qq_account->get_Group_by_gid( group_code );
	std::string groupname = group_code;

	if( group ){
		groupname = group->name;
		ptreee_group.add("groupnumber", group->qqnum);
		ptreee_group.add("name", group->name);
	}

	message.add_child("room", ptreee_group);

	ptree ptree_who;
	ptree_who.add("code", who);

	qqBuddy *buddy = NULL;

	buddy = group ? group->get_Buddy_by_uin( who ) : NULL;
	if (buddy){
		ptree_who.add("name", buddy->nick);
		ptree_who.add("qqnumber", buddy->qqnum);
		ptree_who.add("card", buddy->card);
	}

	message.add_child("who", ptree_who);
	ptree textmsg;

	// 解析 qqMsg
	BOOST_FOREACH( qqMsg qqmsg, msg )
	{
		std::string buf;

		switch( qqmsg.type ) {
			case qqMsg::LWQQ_MSG_TEXT:
			{
				textmsg.add("text", qqmsg.text);
			}
			break;
			case qqMsg::LWQQ_MSG_CFACE:
			{
				if (fetch_img){
					// save to disk
					// 先检查这样的图片本地有没有，有你fetch的P啊.
					if (!fs::exists(std::string("images/") + qqmsg.cface))
					{
						if (!fs::exists("images"))
							fs::create_directories("images");
						m_qq_account->async_fetch_cface(qqmsg.cface, boost::bind(&avbot::callback_save_qq_image, this, _1, _2, qqmsg.cface));
					}
				}
				// 接收方，需要把 cfage 格式化为 url , loger 格式化为 ../images/XX ,
				// 而 forwarder 则格式化为 http://http://w.qq.com/cgi-bin/get_group_pic?pic=XXX
				textmsg.add("cface", qqmsg.cface);
			}
			break;
			case qqMsg::LWQQ_MSG_FACE:
			{
 				std::string url = boost::str( boost::format("http://0.web.qstatic.com/webqqpic/style/face/%d.gif" ) % qqmsg.face );
 				textmsg.add("url", url);
			} break;
		}
	}
	message.add_child("message", textmsg);
	on_message(message);
}

void avbot::callback_on_xmpp_group_message( std::string xmpproom, std::string who, std::string textmsg )
{
	using boost::property_tree::ptree;
	ptree message;

	message.add("protocol", "xmpp");
	message.add("room", xmpproom);
	message.add("who", who);
	message.add_child("message", ptree().add("text", textmsg));

	on_message(message);
}

void avbot::callback_on_mail( mailcontent mail, mx::pop3::call_to_continue_function call_to_contiune )
{

}


void avbot::set_qq_account( std::string qqnumber, std::string password, avbot::need_verify_image cb )
{
	m_qq_account.reset(new webqq(m_io_service, qqnumber, password));
	m_qq_account->on_verify_code(cb);
	m_qq_account->login();
	m_qq_account->on_group_msg(boost::bind(&avbot::callback_on_qq_group_message, this, _1, _2, _3));
}

void avbot::feed_login_verify_code( std::string vcode )
{
	m_qq_account->login_withvc(vcode);
}

void avbot::set_irc_account( std::string nick, std::string password, std::string server, bool use_ssl)
{
	boost::cmatch what;
	if (use_ssl){
		boost::throw_exception(std::invalid_argument("ssl is currently not supported"));
	}

	std::string server_host, server_port;
	if (boost::regex_search(server.c_str(), what, boost::regex("(.*):([0-9]+)?")))
	{
		server_host = what[1];
		server_port = what[2];
	}
	else
	{
		server_host = server;
		server_port = 6667;
	}
	m_irc_account.reset(new irc::IrcClient(m_io_service, nick, password, server_host, server_port));
	m_irc_account->login(boost::bind(&avbot::callback_on_irc_message, this, _1));
}

void avbot::irc_join_room( std::string room_name )
{
	m_irc_account->join(room_name);
}

void avbot::set_xmpp_account( std::string user, std::string password, std::string nick, std::string server )
{
	m_xmpp_account.reset(new xmpp(m_io_service, user, password, server, nick));
	m_xmpp_account->on_room_message(boost::bind(&avbot::callback_on_xmpp_group_message, this, _1, _2, _3));
}

void avbot::xmpp_join_room( std::string room )
{
	m_xmpp_account->join(room);
}

void avbot::set_mail_account( std::string mailaddr, std::string password, std::string pop3server, std::string smtpserver )
{
	// 开启 pop3 收邮件.
	m_mail_account.reset(new mx::mx(m_io_service, mailaddr, password, pop3server, smtpserver));

	m_mail_account->async_fetch_mail(boost::bind(&avbot::callback_on_mail, this, _1, _2));
}
