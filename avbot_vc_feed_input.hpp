
#pragma once

#include <algorithm>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>

class avbot_vc_feed_input{

public:
	void async_input_read(boost::function<void (boost::system::error_code, std::string)> handler)
	{
		// 将 handler 进入 列队.
		m_input_wait_handlers.push_back(handler);
	}

	void call_this_to_feed_line(std::string line)
	{
		BOOST_FOREACH(boost::function<void (boost::system::error_code, std::string)> handler ,  m_input_wait_handlers)
		{
			handler(boost::system::error_code(), line);
		}
	}

	void call_this_to_feed_message(boost::property_tree::ptree message)
	{
		// format 后调用 call_this_to_feed_line
	}

private:
	std::vector<boost::function<void (boost::system::error_code, std::string)> >
			m_input_wait_handlers;
};
