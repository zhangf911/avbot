/*
 * Copyright (C) 2013  microcai <microcai@fedoraproject.org>
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

#ifndef __GOLDPRICE_HPP_
#define __GOLDPRICE_HPP_

#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>

#include <avhttp.hpp>

#include "extension.hpp"
#include "httpagent.hpp"

namespace gold{

// 向 http://hq.sinajs.cn/?_=1369230050901/&list=hf_GC 查询金价.
template<class MsgSender>
struct goldprice_fetcher_op{

	goldprice_fetcher_op(boost::asio::io_service & _io_service, MsgSender _sender)
	  : io_service(_io_service), sender(_sender), stream(new avhttp::http_stream(_io_service))
	{
		async_http_download(stream, "http://hq.sinajs.cn/?_=1369230050901/&list=hf_GC", *this);
	}

	void operator()(boost::system::error_code ec, read_streamptr stream,  boost::asio::streambuf & buf)
	{

		if (!ec || ec == boost::asio::error::eof){
			// 读取 buf 内容, 执行 regex 匹配, 获取金价.
			std::string jscript;
			jscript.resize(buf.size());
			buf.sgetn(&jscript[0], buf.size());

			boost::cmatch what;
			boost::regex ex("var hq_str_hf_GC=\"([0-9.\\-]*),(.*)");
			if (boost::regex_search(jscript.c_str(), what, ex))
			{
				std::string price = what[1];
				std::string msg = boost::str(boost::format("当前价 %s") % price );

				sender(msg);
			}
		}
	}

	boost::asio::io_service & io_service;
	MsgSender sender;
	boost::shared_ptr<avhttp::http_stream> stream;

};

template<class MsgSender>
void goldprice_fetcher(boost::asio::io_service & io_service, MsgSender sender)
{
	goldprice_fetcher_op<MsgSender>(io_service, sender);
}

}

class goldprice : avbotextension
{
private:

public:
	template<class MsgSender>
	goldprice(boost::asio::io_service & _io_service, MsgSender sender, std::string channel_name)
	  : avbotextension(_io_service, sender, channel_name)
	{
	}

	void operator()(const boost::system::error_code& error);

	void operator()(boost::property_tree::ptree msg)
	{
 		if( msg.get<std::string>( "channel" ) != m_channel_name )
			return;

		std::string textmsg = boost::trim_copy( msg.get<std::string>( "message.text" ) );

		if (textmsg == ".qqbot 黄金报价"){
			gold::goldprice_fetcher(io_service, m_sender);
		}
	}
};

#endif // __GOLDPRICE_HPP_
