
#pragma once

#include <algorithm>
#include <boost/locale.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>

struct lua_State;

class callluascript
{
	boost::asio::io_service &io_service;
	boost::function<void ( std::string ) > m_sender;
	std::string m_channel_name;
	lua_State * lua_State;

public:
	callluascript( boost::asio::io_service &_io_service,  boost::function<void ( std::string ) >  sender, std::string channel_name );

	// on_message 回调.
	void operator()( boost::property_tree::ptree message ) const;
};
