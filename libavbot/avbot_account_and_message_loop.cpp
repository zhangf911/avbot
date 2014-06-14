#include "avbot_account_and_message_loop.hpp"

void avbot_account_and_message_loop::message_loop_coro(boost::shared_ptr<boost::atomic<bool> > flag_quit, concepts::avbot_account accounts, boost::asio::yield_context yield)
{
#define  flag_check() do { if (*flag_quit){return;} } while(false)

	boost::system::error_code ec;
	// 添加到 m_accounts 里.
	m_accouts.push_back(&accounts);

	BOOST_SCOPE_EXIT(&m_accouts, &accounts, flag_quit)
	{
		// 如果 flag_quit是真的，那么 this 其实是不能访问的，因为已经析构了。
		if (!*flag_quit)
		{
			std::remove(m_accouts.begin(), m_accouts.end(), &accounts);
		}
	}BOOST_SCOPE_EXIT_END;

	// 登录执行完成！
	// 开始读取消息
	for (;;)
	{
		// 执行登录!
		flag_check();
		do{
			accounts.async_login(yield[ec]);
			flag_check();
			if (accounts.is_error_fatal(ec))
			{
				// 帐号有严重的问题，只能删除这一帐号，没有别的选择
				return;
			}
		} while (ec);

		do
		{
			// 等待并解析协议的消息
			boost::property_tree::ptree message = accounts.async_recv_message(yield[ec]);
			flag_check();

			// 调用 broadcast message, 如果没要求退出的话
			if (!ec)
				on_message(message);
			// 掉线了？重登录！
			// 只有遇到了必须要重登录的错误才重登录
			// 一时半会的网络错误可没事，再获取一下就可以了
		} while (!accounts.is_error_fatal(ec));
	}
#undef  flag_check
}

avbot_account_and_message_loop::~avbot_account_and_message_loop()
{

}

avbot_account_and_message_loop::avbot_account_and_message_loop()
{

}
