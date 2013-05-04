
extern "C"{
#include <luajit-2.0/luajit.h>
#include <luajit-2.0/lualib.h>
#include <luajit-2.0/lauxlib.h>
}

#include "luabind/object.hpp"
#include "luabind/luabind.hpp"
#include "luabind/tag_function.hpp"

#include "luascript.hpp"

struct lua_sender{
	boost::function<void ( std::string ) > m_sender;

	template<class F>
	lua_sender(F f):m_sender(f){}

	void operator()(const char * str) const
	{
		std::cerr <<  "send::" << str << std::endl;
		m_sender(str);
	}
};

callluascript::callluascript( boost::asio::io_service &_io_service,  boost::function<void ( std::string ) >  sender, std::string channel_name )
	: io_service( _io_service ), m_sender( sender ), m_channel_name( channel_name )
{
	lua_State* L = luaL_newstate();
	m_lua_State.reset(L, lua_close);
    luaL_openlibs(L);

	luabind::open(L);

	// 准备调用 LUA 脚本.
	luabind::module(L)[
		luabind::def("send_channel_message", luabind::tag_function<void(const char *)>(lua_sender(m_sender)) )
	];

	luaL_dofile(L, "main.lua");

}

callluascript::~callluascript()
{

}

void callluascript::operator()( boost::property_tree::ptree message ) const
{
	if( message.get<std::string>( "channel" ) != m_channel_name )
	{
		return;
	}
	// m_sender 这个函数就可以用来发消息到群里.

}
