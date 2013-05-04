
#include "luascript.hpp"

void callluascript::operator()( boost::property_tree::ptree message ) const
{
	if( message.get<std::string>( "channel" ) != m_channel_name )
	{
		return;
	}

	// 准备调用 LUA 脚本.
}
