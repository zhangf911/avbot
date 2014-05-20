/*
 * Copyright (C) 2014  microcai <microcai@fedoraproject.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#ifndef __DLL_EXTENSION_HPP_
#define __DLL_EXTENSION_HPP_

#include <boost/make_shared.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread/thread.hpp>

#include <boost/cfunction.hpp>
#include "extension.hpp"

template<class MsgSender>
class dllextention_caller
{
private:
	boost::asio::io_service &io_service;
	std::string m_channel;
	typename boost::remove_reference<MsgSender>::type m_sender;
	HMODULE m_dll_module;
public:
	dllextention_caller(boost::asio::io_service & _io_service, std::string _channel, MsgSender sender, HMODULE dll_module)
		: m_sender(sender)
		, io_service(_io_service)
		, m_channel(_channel)
		, m_dll_module(dll_module)
	{
	}

	typedef void (*message_sender_t)(const char * message, void* _apitag);

	typedef void (*avbot_on_message_t)(const char * message, const char * channel, message_sender_t sender, void* _apitag);

	void operator()(boost::property_tree::ptree msg)
	{
		if (m_dll_module)
		{
			std::string textmsg = boost::trim_copy(msg.get<std::string>("message.text"));

			boost::thread t(boost::bind(&dllextention_caller<MsgSender>::call_dll_message, this, textmsg, m_channel, m_sender));
			t.detach();
		}
	}

	void call_dll_message(std::string textmsg, std::string channel, boost::function<void(std::string)> sender)
	{
		
		auto p = GetProcAddress(m_dll_module, "avbot_on_message");
		avbot_on_message_t f = reinterpret_cast<avbot_on_message_t>(p);

		if (!f)
		{
			MessageBoxA(0, "avbot_on_message 函数没找到", "avbotdllextension.dll", MB_OK);
		}

		// 调用 f 吧！
		boost::cfunction<message_sender_t, void(std::string)> sender_for_c = sender;
		f(textmsg.c_str(), channel.c_str(), sender_for_c.c_func_ptr(), sender_for_c.c_void_ptr());

		std::cout << "called avbot_on_message()" << std::endl;
	}
};

static void once_run_dllextention(HMODULE & dll_module)
{
	dll_module = LoadLibrary("avbotextension.dll");
	if (!dll_module)
	{
		std::cerr << literal_to_localstr("没有找到 avbotextension.dll!!!") << std::endl;
		std::cerr << literal_to_localstr("禁用 dll 扩展！") << std::endl;
	}	
}

template<class MsgSender>
dllextention_caller<MsgSender>
make_dllextention(boost::asio::io_service & io_service, std::string _channel, const MsgSender & sender)
{
#ifdef BOOST_THREAD_PROVIDES_ONCE_CXX11
	static boost::once_flag once;
	//static boost::once_flag once2 = BOOST_ONCE_INIT;
#else
	static boost::once_flag once = BOOST_ONCE_INIT;
	//static boost::once_flag once2 = once;
#endif
	// 在这里加载 DLL 并发送消息
	// 加载 DLL 然后执行，然后将 DLL 释放
	static HMODULE dll_module;

	boost::call_once(boost::bind(&once_run_dllextention, boost::ref(dll_module)), once);

	return dllextention_caller<MsgSender>(io_service, _channel, sender, dll_module);
}

#endif // __DLL_EXTENSION_HPP_
