/**
 * botcmd.cpp 实现 .qqbot 命令控制
 */

#include <string>
#include <algorithm>
#include <vector>

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/noncopyable.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/locale.hpp>
#include <boost/lambda/lambda.hpp>
#include <locale.h>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>
#if defined(_MSC_VER)
#include <direct.h>
#endif
#include "libirc/irc.h"
#include "libwebqq/webqq.h"
#include "libwebqq/url.hpp"
#include "libxmpp/xmpp.h"
#include "libmailexchange/mx.hpp"

#include "counter.hpp"
#include "logger.hpp"

#include "auto_question.hpp"
#include "messagegroup.hpp"


#ifndef QQBOT_VERSION
#define QQBOT_VERSION "unknow"
#endif

static auto_question question;	// 自动问问题.
static bool resend_img = false;	// 用于标识是否转发图片url.

extern qqlog logfile;			// 用于记录日志文件.

enum sender_flags{
	sender_is_op, // 管理员, 群管理员或者频道OP .
	sender_is_normal, // 普通用户.
};

//-------------

// 命令控制, 所有的协议都能享受的命令控制在这里实现.
// msg_sender 是一个函数, on_command 用它发送消息.
void on_bot_command(boost::asio::io_service& io_service, std::string message, std::string from_channel, std::string sender, sender_flags sender_flag, boost::function<void(std::string)> msg_sender)
{
	boost::regex ex;
	boost::cmatch what;

    messagegroup* chanelgroup = find_group(from_channel);
	qqGroup* group =  chanelgroup->qq_->get_Group_by_qq(from_channel.substr(3));

	if( message == ".qqbot help")
	{
        io_service.post(
            boost::bind(msg_sender, "可用的命令\n"
						"\t.qqbot help\n"
						"\t.qqbot version\n"
						"\t.qqbot ping\n"
						"\t.qqbot mailto [emailaddress]\n"
						"\t 将命令中间的聊天内容发送到邮件 emailaddress\n"
						"\t.qqbot mailend\n"
						"== 以下命令需要管理员才能使用==\n"
						"\t.qqbot relogin 强制重新登录qq\n\t.qqbot reload 重新加载群成员列表\n"
                        "\t.qqbot start image \t.qqbot stop image 开启关闭群图片的URL转发\n"
                        "\t.qqbot begin class XXX\t\n\t.qqbot end class\n"
                        "\t.qqbot newbee SB\n"
                        "以上! (别吐嘈, 我是武藏舰长)")
        );
	}

	if( message == ".qqbot ping")
	{
		io_service.post(boost::bind(msg_sender,"我还活着"));
	}
	
	if( message == ".qqbot version")
	{
		io_service.post(boost::bind(msg_sender,boost::str(boost::format("我的版本是 %s (%s %s)") % QQBOT_VERSION %__DATE__% __TIME__)));
	}

	if ( message == ".qqbot mailend")
	{
		// 开始发送 !
	}
	ex.set_expression(".qqbot mailto ?\"(.*)?\"");

	if(boost::regex_match(message.c_str(), what, ex))
	{
		// 进入邮件记录模式.
	}

	if ( sender_flag == sender_is_op )
	{
		if (message == ".qqbot relogin")
		{
			io_service.post(
				boost::bind(&webqq::login,chanelgroup->qq_)
			);
		}

		if ( message == ".qqbot exit")
		{
			exit(0);
		}
		// 转发图片处理.
		if (message == ".qqbot start image")
		{
			resend_img = true;
		}

		if (message == ".qqbot stop image")
		{
			resend_img = false;
		}

		// 重新加载群成员列表.
		if (message == ".qqbot reload")
		{

			io_service.post(boost::bind(&webqq::update_group_member, chanelgroup->qq_ , boost::ref(*group)));
			msg_sender("群成员列表重加载");
		}

		// 开始讲座记录.	
		ex.set_expression(".qqbot begin class ?\"(.*)?\"");
		
		if(boost::regex_match(message.c_str(), what, ex))
		{
			std::string title = what[1];
			if (title.empty()) return ;
			if (!logfile.begin_lecture(group->qqnum, title))
			{
				printf("lecture failed!\n");
			}
		}

		// 停止讲座记录.
		if (message == ".qqbot end class")
		{
			logfile.end_lecture();
		}
		
		// 向新人问候.
		ex.set_expression(".qqbot newbee ?(.*)?");
		if(boost::regex_match(message.c_str(), what, ex))
		{
			std::string nick = what[1];
			
			if (nick.empty())
				return;
				
			auto_question::value_qq_list list;			
			list.push_back(nick);
			
			question.add_to_list(list);
			question.on_handle_message( msg_sender );
		}
	}	
}
