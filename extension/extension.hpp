
#pragma once

#include <string>
#include <boost/asio/io_service.hpp>

#include "libavbot/avbot.hpp"

class avbotextension
{
protected:
	boost::asio::io_service &io_service;
	boost::function<void ( std::string ) > m_sender;
	std::string m_channel_name;

	template<class MsgSender>
	avbotextension( boost::asio::io_service &_io_service,  MsgSender sender, std::string channel_name )
		: io_service( _io_service ), m_sender( sender ), m_channel_name( channel_name )
	{
	}
};


void new_channel_set_extension(boost::asio::io_service &io_service, avbot & mybot , std::string channel_name);
