
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
		m_sender(str);
	}
};

callluascript::callluascript( boost::asio::io_service &_io_service,  boost::function<void ( std::string ) >  sender, std::string channel_name )
	: io_service( _io_service ), m_sender( sender ), m_channel_name( channel_name )
{
	m_lua_State.reset(luaL_newstate(), lua_close);

	luaopen_base(m_lua_State.get());
	luaopen_string(m_lua_State.get());
	luaopen_table(m_lua_State.get());
	luaopen_math(m_lua_State.get());
	luaopen_io(m_lua_State.get());
	luaopen_debug(m_lua_State.get());


	using namespace luabind;
	// 准备调用 LUA 脚本.
 	module(this->m_lua_State.get())[
 		def("send_channel_message", tag_function<void(const char *)>(lua_sender(m_sender)) )
 	];
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
