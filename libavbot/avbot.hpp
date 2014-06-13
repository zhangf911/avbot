
#pragma once

#include <boost/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/signals2.hpp>
#include <boost/property_tree/ptree.hpp>

#include "libwebqq/webqq.hpp"
#include "libirc/irc.hpp"
#include "libxmpp/xmpp.hpp"
#include "libmailexchange/mx.hpp"

namespace concepts{

class avbot_account;

namespace implementation{

class avbot_account_indrector : boost::noncopyable
{
	friend class concepts::avbot_account;
	virtual void async_login(boost::function<void(boost::system::error_code)> handler);
	virtual void async_recv_message(boost::function<void(boost::system::error_code, boost::property_tree::ptree)> handler);
	virtual void async_send_message(std::string target, std::string message, boost::function<void(boost::system::error_code)> handler);
public:
	virtual ~avbot_account_indrector(){}
};

template<typename T>
class avbot_account_adapter : public avbot_account_indrector
{
	friend class avbot_account;

	typename boost::remove_reference<T>::type m_real_avbot_account;
public:
	virtual ~avbot_account_adapter(){}
#ifdef BOOST_ASIO_HAS_MOVE
	avbot_account_adapter(T && wrapee)
		: m_real_avbot_account(wrapee)
	{
	}
#endif // !BOOST_ASIO_HAS_MOVE
	avbot_account_adapter(const T &wrapee)
		: m_real_avbot_account(wrapee)
	{
	}
private:
	void async_login(boost::function<void(boost::system::error_code)> handler)
	{
		m_real_avbot_account.async_login(handler);
	}

	void async_recv_message(boost::function<void(boost::system::error_code, boost::property_tree::ptree)> handler)
	{
		m_real_avbot_account.async_recv_message(handler);
	}

	void async_send_message(std::string target, std::string message, boost::function<void(boost::system::error_code)> handler)
	{
		m_real_avbot_account.async_send_message(target, message, handler);
	}
};

}

// copyable and movable, so it can be put into STL container
class avbot_account
{
	boost::shared_ptr<implementation::avbot_account_indrector> _impl;
public:
	template<typename Handler>
	inline BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code))
		async_login(BOOST_ASIO_MOVE_ARG(Handler) handler)
	{
		using namespace boost::asio;
		boost::asio::detail::async_result_init<Handler, void(boost::system::error_code)>
			init(BOOST_ASIO_MOVE_CAST(Handler)(handler));

		_impl->async_login(init.handler);

		return init.result.get();
	}

	template<typename Handler>
	inline BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code, boost::property_tree::ptree))
		async_recv_message(BOOST_ASIO_MOVE_ARG(Handler) handler)
	{
		using namespace boost::asio;
		boost::asio::detail::async_result_init<Handler, void(boost::system::error_code, boost::property_tree::ptree)>
			init(BOOST_ASIO_MOVE_CAST(Handler)(handler));

		_impl->async_recv_message(init.handler);

		return init.result.get();
	}

	template<typename Handler>
	inline BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code))
		async_send_message(std::string target, std::string message, BOOST_ASIO_MOVE_ARG(Handler) handler)
	{
		using namespace boost::asio;
		boost::asio::detail::async_result_init<Handler, void(boost::system::error_code)>
			init(BOOST_ASIO_MOVE_CAST(Handler)(handler));

		_impl->async_send_message(target, message, init.handler);

		return init.result.get();
	}

	avbot_account()
	{}

#ifdef BOOST_NO_RVALUE_REFERENCES
	template<typename T>
	avbot_account(const T & wrapee)
	{
		_impl.reset(new implementation::avbot_account_adapter<boost::remove_reference<T>::type>(wrapee));
	}

	avbot_account(const avbot_account & other)
	{
		_impl = other._impl;
	}
#else
	template<typename T>
	avbot_account(T && wrapee)
	{
		_impl.reset(new implementation::avbot_account_adapter<boost::remove_reference<T>::type>(wrapee));
	}

	avbot_account(avbot_account && other)
	{
		_impl = std::move(other._impl);
	}
#endif
};

}

class BOOST_SYMBOL_VISIBLE avbot : boost::noncopyable
{
public:
	typedef std::vector<std::string> av_chanels_t;
private:
	boost::asio::io_service & m_io_service;

	// 把帐户放到 STL 容器里
	std::vector<concepts::avbot_account> m_accouts;

	boost::shared_ptr<webqq::webqq> m_qq_account;
	boost::shared_ptr<irc::client> m_irc_account;
	boost::shared_ptr<xmpp> m_xmpp_account;
	boost::shared_ptr<mx::mx> m_mail_account;
	// channel have a name :)
	std::map<std::string, av_chanels_t> m_channel_map;

public:
	avbot(boost::asio::io_service & io_service);


public:
	// 这里是一些公开的成员变量.
	typedef boost::function<void (std::string) > need_verify_image;
	typedef boost::property_tree::ptree av_message_tree;
	typedef boost::signals2::signal<void (av_message_tree) > on_message_type;

	// 用了传入一个 url 生成器，这样把 qq 的消息里的图片地址转换为 vps 上跑的 http 服务的地址。
	boost::function<std::string(av_message_tree)> m_urlformater;

	// 每当有消息的时候激发.
	on_message_type on_message;
	// 每当有新的频道被创建的时候激发
	// 还记得吗？如果libweqq找到了一个QQ群，会自动创建和QQ群同名的频道的（如果不存在的话）
	// 这样就保证没有被加入 map= 的群也能及时的被知道其存在.
	// 呵呵，主要是 libjoke 用来知道都有那些频道存在
	boost::signals2::signal< void (std::string) > signal_new_channel;

	std::string preamble_qq_fmt, preamble_irc_fmt, preamble_xmpp_fmt;
	bool fetch_img;

public:
	boost::shared_ptr<webqq::webqq> get_qq(){return m_qq_account;}
	boost::shared_ptr<xmpp> get_xmpp(){return m_xmpp_account;}
	boost::shared_ptr<mx::mx> get_mx(){return m_mail_account;}
	boost::shared_ptr<irc::client> get_irc(){return m_irc_account;}
public:

	// 调用这个接口添加受 avbot 控制的账户。
	void add_account(BOOST_ASIO_MOVE_ARG(concepts::avbot_account) accounts);

	// 调用这个添加 QQ 帐号. need_verify_image 会在需要登录验证码的时候调用，buffer 里包含了验证码图片.
	void set_qq_account(std::string qqnumber, std::string password, need_verify_image cb, bool no_persistent_db = false);
	// 调用这个重新登陆 QQ
	void relogin_qq_account();

	// 如风发生了 需要验证码 这样的事情，就麻烦调用这个把识别后的验证码喂进去.
	void feed_login_verify_code(std::string vcode, boost::function<void()> badvcreporter = boost::function<void()>());

	// 调用这个添加 IRC 帐号.
	void set_irc_account(std::string nick = autonick(), std::string password = "" , std::string server = "irc.freenode.net:6667", bool use_ssl = false);
	// 调用这个加入 IRC 频道.
	void irc_join_room(std::string room_name);
	// 调用这个加入 IRC 频道, 并提供频道密码.
	void irc_join_room(std::string room_name, std::string room_passwd);

	// 调用这个设置 XMPP 账户.
	void set_xmpp_account(std::string user, std::string password, std::string nick="avbot", std::string server="");
	// 调用这个加入 XMPP 聊天室.
	// root 的格式为 roomname@server
	void xmpp_join_room(std::string room);

	// 调用这个设置邮件账户.
	void set_mail_account(std::string mailaddr, std::string password, std::string pop3server = "", std::string smtpserver = "");


public:
	// NOTE: webqq will create a channel_name name after qq group number automantically
	void add_to_channel(std::string channel_name, std::string room_name);
	av_chanels_t get_channel_map(std::string channel_name){
		return m_channel_map[channel_name];
	}
	// 从 "irc:avplayer" 这样的名字获得组合频道的名字.
	std::string get_channel_name(std::string room_name);
	boost::asio::io_service & get_io_service(){return m_io_service;}
public:
	void broadcast_message(std::string);
	void broadcast_message(std::string channel_name, std::string msg);
	void broadcast_message(std::string channel_name, std::string exclude_room, std::string msg);
private:
	void accountsroutine(concepts::avbot_account accounts, boost::asio::yield_context yield);

	void callback_on_irc_message(irc::irc_msg pMsg);
	void callback_on_qq_group_message(std::string group_code, std::string who, const std::vector<webqq::qqMsg> & msg);
	void callback_on_xmpp_group_message(std::string xmpproom, std::string who, std::string message);
	void callback_on_mail(mailcontent mail, mx::pop3::call_to_continue_function call_to_contiune);

private:
	void callback_on_qq_group_found(webqq::qqGroup_ptr);
	void callback_on_qq_group_newbee(webqq::qqGroup_ptr, webqq::qqBuddy_ptr);

private:
	void forward_message(const av_message_tree &message);
public:
	// auto pick an nick name for IRC
	static std::string autonick();
	std::string format_message( const avbot::av_message_tree& message );
	static std::string image_subdir_name(std::string cface);
};
