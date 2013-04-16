
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <boost/property_tree/ptree.hpp>

#include "libwebqq/webqq.h"
#include "libirc/irc.h"
#include "libxmpp/xmpp.h"
#include "libmailexchange/mx.hpp"

class avbot : boost::noncopyable{
	enum {
		avbot_account_qq,
		avbot_account_irc,
		avbot_account_xmpp,
		avbot_account_mail,
	}avbot_account_type;

	boost::asio::io_service & m_io_service;

	boost::scoped_ptr<webqq> m_qq_account;
	boost::scoped_ptr<irc::IrcClient> m_irc_account;
	boost::scoped_ptr<xmpp> m_xmpp_account;
	boost::scoped_ptr<mx::mx> m_mail_account;

	std::string base_image_url;
	bool fetch_img;
public:
	avbot(boost::asio::io_service & io_service);
public:
	boost::signals2::signal< void (boost::property_tree::ptree) > on_message;
	typedef boost::function<void (const boost::asio::const_buffer &) > need_verify_image;

public:
	// 调用这个添加 QQ 帐号. need_verify_image 会在需要登录验证码的时候调用，buffer 里包含了验证码图片.
	void set_qq_account(std::string qqnumber, std::string password, need_verify_image cb);
	// 如风发生了 需要验证码 这样的事情，就麻烦调用这个把识别后的验证码喂进去.
	void feed_login_verify_code(std::string vcode);


	// 调用这个添加 IRC 帐号.
	void set_irc_account(std::string nick = autonick(), std::string password = "" , std::string server = "irc.freenode.net:6667", bool use_ssl = false);
	// 调用这个加入 IRC 频道.
	void irc_join_room(std::string room_name);

	// 调用这个设置 XMPP 账户.
	void set_xmpp_account(std::string user, std::string password, std::string nick="avbot", std::string server="");
	// 调用这个加入 XMPP 聊天室.
	// root 的格式为 roomname@server
	void xmpp_join_room(std::string room);

	// 调用这个设置邮件账户.
	void set_mail_account(std::string mailaddr, std::string password, std::string pop3server = "", std::string smtpserver = "");

	// NOTE: webqq will create a channel_name name after qq group number automantically
	void add_to_channel(std::string channel_name, std::string room_name);

private:
	void callback_on_irc_message(irc::IrcMsg pMsg);
	void callback_on_qq_group_message(std::string group_code, std::string who, const std::vector<qqMsg> & msg);
	void callback_on_xmpp_group_message(std::string xmpproom, std::string who, std::string message);
	void callback_on_mail(mailcontent mail, mx::pop3::call_to_continue_function call_to_contiune);
private:
	void callback_save_qq_image(const boost::system::error_code & ec, boost::asio::streambuf & buf, std::string cface);
public:
	// auto pick an nick name for IRC
	static std::string autonick();
};
