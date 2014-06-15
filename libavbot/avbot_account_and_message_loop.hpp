#pragma once

#include <boost/noncopyable.hpp>
#include <boost/atomic.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/scope_exit.hpp>
#include <boost/signals2.hpp>
#include "avbot_accounts.hpp"

// 这个类用来执行
class avbot_account_and_message_loop : boost::noncopyable
{
public:
	avbot_account_and_message_loop();

	~avbot_account_and_message_loop();

	void add_account(const concepts::avbot_account & avbot_account)
	{
		// 为新帐号创建新的协程
		boost::asio::spawn(m_io_service, boost::bind(&avbot_account_and_message_loop::message_loop_coro, this, m_quit, avbot_account, _1));
	}
private:

	void message_loop_coro(boost::shared_ptr<boost::atomic<bool> > flag_quit, concepts::avbot_account accounts, boost::asio::yield_context yield);

private:
	boost::shared_ptr< boost::atomic<bool> > m_quit;
	std::vector<concepts::avbot_account*> m_accouts;

	boost::asio::io_service m_io_service;
public:
	typedef boost::property_tree::ptree av_message_tree;
	typedef boost::signals2::signal<void(av_message_tree) > on_message_type;
	// 每当有消息的时候激发.
	on_message_type on_message;
};
