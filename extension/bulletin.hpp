
#pragma once

#include <algorithm>
#include <boost/locale.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <avhttp.hpp>

#include "extension.hpp"

class bulletin : avbotextension
{
public:
	template<class MsgSender>
	bulletin( boost::asio::io_service &_io_service,  MsgSender sender, std::string channel_name )
	  : avbotextension(_io_service, sender, channel_name)
	{
		// 读取公告配置.
		// 公告配置分两个部分, 一个是公告文件  $qqlog/$channel_name/bulletin.txt
		// 公告文件每次发送的时候读取,  而不是一次性读取.
		// 另一个是公告频次配置
		// 里面是一个时间的数组
		// 时间是 YY-MM-DD-HH-MM 这个的格式
		// 留 * 表示 每
		// 比如 *-*-*-08-00 表示每天早上 8 点
		

	}

	// on_message 回调.
	void operator()( boost::property_tree::ptree message ) const;
};
