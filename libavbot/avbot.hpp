
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <boost/property_tree/ptree.hpp>

#include "libwebqq/webqq.h"
#include "libirc/irc.h"
#include "libxmpp/xmpp.h"

class avbot : boost::noncopyable{
	enum {
		avbot_account_qq,
		avbot_account_irc,
		avbot_account_xmpp,
		avbot_account_mail,
	}avbot_account_type;

	boost::asio::io_service & m_io_service;

	boost::shared_ptr<webqq> m_qq_account;
	boost::shared_ptr<irc::IrcClient> m_irc_account;
	boost::shared_ptr<xmpp> m_xmpp_account;

	std::string base_image_url;
	bool fetch_img;
public:
	boost::signals2::signal< void (boost::property_tree::ptree) > on_message;

	avbot(boost::asio::io_service & io_service);
	void add_qq_account(boost::shared_ptr<webqq> qqclient);
	void add_irc_account(boost::shared_ptr<irc::IrcClient> irc_client);
	void add_xmpp_account(boost::shared_ptr<xmpp> xmppclient);
	void add_mail_account(std::string mailaddr, std::string password, std::string pop3server = "", std::string smtpserver = "");

	// NOTE: webqq will create a channel_name name after qq group number automantically
	void add_to_channel(std::string channel_name, std::string room_name);

private:
	void callback_on_irc_message(irc::IrcMsg pMsg);
	void callback_on_qq_group_message(std::string group_code, std::string who, const std::vector<qqMsg> & msg);

	void callback_save_qq_image(const boost::system::error_code & ec, boost::asio::streambuf & buf, std::string cface);
};
