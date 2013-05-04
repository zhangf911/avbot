
#include <lua.h>

#include "luascript.hpp"

callluascript::callluascript( boost::asio::io_service &_io_service,  boost::function<void ( std::string ) >  sender, std::string channel_name )
	: io_service( _io_service ), m_sender( sender ), m_channel_name( channel_name )
{
}

void callluascript::operator()( boost::property_tree::ptree message ) const
{
	if( message.get<std::string>( "channel" ) != m_channel_name )
	{
		return;
	}

	// 准备调用 LUA 脚本.

}
