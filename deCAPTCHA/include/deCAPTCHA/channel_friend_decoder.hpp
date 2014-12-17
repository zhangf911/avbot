﻿/*
 * Copyright (C) 2013  微蔡 <microcai@fedoraproject.org>
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
#include <string>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/regex.hpp>

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

namespace decaptcha{
namespace decoder{

namespace detail{

inline bool is_vc(std::string str)
{
	boost::cmatch what;
	boost::regex ex("[a-z0-9A-Z][a-z0-9A-Z][a-z0-9A-Z][a-z0-9A-Z]");
	return boost::regex_match(str.c_str(), what, ex);
}

template<class Sender, class AsyncInputer, class Handler>
class channel_friend_decoder_op : boost::asio::coroutine
{
public:
	channel_friend_decoder_op(boost::asio::io_service & io_service,
			Sender sender, AsyncInputer async_inputer,
			const std::string & buffer, Handler handler)
		: m_io_service(io_service)
		, m_sender(sender)
		, m_async_inputer(async_inputer)
		, m_handler(handler)
		, m_buffer(buffer)
	{
		m_io_service.post(
			boost::asio::detail::bind_handler(*this, boost::system::error_code(), std::string())
		);
	}

	void operator()(boost::system::error_code ec, std::string str)
	{
		std::string tmp;
		if (ec)
		{
			m_io_service.post(
				boost::asio::detail::bind_handler(
					m_handler,
					ec,
					std::string("IRC/XMPP 好友辅助验证码解码器"),
					std::string(),
					boost::function<void()>()
				)
			);
			return;
		}

		BOOST_ASIO_CORO_REENTER(this)
		{
			// send to xmpp and irc.
			// 向 频道广播消息.
			m_sender("请查看qqlog目录下的vercode.jpeg 然后用\".qqbot vc XXX\"输入验证码:");

			// 同时向命令行也广播
			std::cerr << literal_to_localstr("请查看qqlog目录下的vercode.jpeg 然后输入验证码: ");
			std::cerr.flush();

			// 等待输入
			BOOST_ASIO_CORO_YIELD m_async_inputer(m_buffer, *this);
			if (ec == boost::asio::error::timed_out)
			{
				// 控制台输入超时，转向irc
			}
			str = boost::trim_copy(str);
			// 检查 str
			if(str.length() == 4 && is_vc(str))
			{
				// 是 vc 的话就调用 handler
				m_io_service.post(
					boost::asio::detail::bind_handler(
						m_handler, ec, std::string("IRC/XMPP 好友辅助验证码解码器"), str, boost::function<void()>()
					)
				);
				return;
			}

			if ( check_qqbot_vc(str, tmp)){
				m_io_service.post(
					boost::asio::detail::bind_handler(
						m_handler, ec, std::string("IRC/XMPP 好友辅助验证码解码器"), tmp, boost::function<void()>()
					)
				);
				return;
			}
			m_io_service.post(
				boost::asio::detail::bind_handler(
					m_handler,
					ec,
					std::string("IRC/XMPP 好友辅助验证码解码器"),
					std::string(),
					boost::function<void()>()
				)
			);
		}
	}
private:
	bool check_qqbot_vc(std::string message, std::string & out)
	{
		boost::cmatch what;
		static boost::regex ex(".qqbot vc ([^ ]*)");
		std::string _vccode;

		if(boost::regex_match(message.c_str(), what, ex))
		{
			_vccode = what[1];

			if(_vccode.length() == 4)
			{
				out = _vccode;
				return true;
			}
		}

		return false;
	}
private:
	boost::asio::io_service & m_io_service;
	Sender m_sender;
	std::string m_buffer;
	AsyncInputer m_async_inputer;
	Handler m_handler;
};

}

template<class Sender, class AsyncInputer>
class channel_friend_decoder_t{
public:
	channel_friend_decoder_t(boost::asio::io_service & io_service, Sender sender, AsyncInputer async_inputer)
	  : m_io_service(io_service), m_sender(sender), m_async_inputer(async_inputer)
	{
	}

	template <class Handler>
	void operator()(const std::string &buffer, Handler handler)
	{
		detail::channel_friend_decoder_op<Sender, AsyncInputer, Handler>
				op(m_io_service, m_sender, m_async_inputer, buffer, handler);
	}

private:
	boost::asio::io_service & m_io_service;
	Sender m_sender;
	AsyncInputer m_async_inputer;
};

template<class Sender, class AsyncInputer> channel_friend_decoder_t<Sender, AsyncInputer>
channel_friend_decoder(boost::asio::io_service & io_service, Sender sender, AsyncInputer async_inputer)
{
	return channel_friend_decoder_t<Sender, AsyncInputer>(io_service, sender, async_inputer);
}

} // namespace decoder
} // namespace decaptcha
