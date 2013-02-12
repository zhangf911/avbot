/**
 * @file   main.cpp
 * @author microcai <microcaicai@gmail.com>
 * 
 */
#include <string>
#include <algorithm>
#include <vector>

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/program_options.hpp>
namespace po = boost::program_options;
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/noncopyable.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/locale.hpp>

#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>

#include "libirc/irc.h"
#include "libwebqq/webqq.h"
#include "libwebqq/url.hpp"
#include "utf8/utf8.h"
#include "libxmpp/xmpp.h"

#include "counter.hpp"
#include "logger.hpp"

#include "auto_question.hpp"

#define QQBOT_VERSION "0.0.1"

static void qq_msg_sended(const boost::system::error_code& ec)
{

}

static qqlog logfile;			// 用于记录日志文件.
static counter cnt;				// 用于统计发言信息.
static bool resend_img = false;	// 用于标识是否转发图片url.
static bool qqneedvc = false;	// 用于在irc中验证qq登陆.
static auto_question question;	// 自动问问题.
static std::string progname;
static std::string ircvercodechannel;
class messagegroup;
static std::vector<messagegroup>	messagegroups;

class messagegroup {
	webqq*		qq_;
	xmpp*		xmpp_;
	IrcClient*	irc_;
	// 组号.
	std::vector<std::string> channels;

public:
	messagegroup(webqq * _qq, xmpp * _xmpp, IrcClient * _irc)
	:qq_(_qq),xmpp_(_xmpp),irc_(_irc){}

	void add_channel(std::string);
	void add_channel(std::vector<std::string> groups){
		//std::copy(channels,groups);
		channels = groups;
	}

	bool in_group(std::string ch){
		return std::find(channels.begin(),channels.end(),ch)!=channels.end();
	}
	void broadcast(std::string message);
	void forwardmessage(std::string from, std::string message){
		BOOST_FOREACH(std::string chatgroupmember, channels)
	 	{
			if (chatgroupmember == from)
				continue;

			if (chatgroupmember.substr(0,3) == "irc")
			{
				irc_->chat(std::string("#") + chatgroupmember.substr(4), message);
			}
			else if (chatgroupmember.substr(0,4) == "xmpp")
			{
				//XMPP
				xmpp_->send_room_message(chatgroupmember.substr(5), message);
			}else if (chatgroupmember.substr(0,2)=="qq" )
			{
				
				std::wstring qqnum = utf8_wide(chatgroupmember.substr(3));
				logfile.add_log(qqnum, message);
				if(qq_->get_Group_by_qq(qqnum))
					qq_->send_group_message(*qq_->get_Group_by_qq(qqnum), message, qq_msg_sended);
			}
		}
	}
};

/*
 * 查找同组的其他聊天室和qq群.
 */
static messagegroup * find_group(std::string id)
{
	BOOST_FOREACH(messagegroup & g ,  messagegroups)
	{
		if (g.in_group(id))
			return &g;
	}
	return NULL;
}

//从命令行或者配置文件读取组信息.
static void build_group(std::string chanelmapstring, webqq & qqclient, xmpp& xmppclient, IrcClient &ircclient)
{
	std::vector<std::string> gs;
	boost::split(gs, chanelmapstring, boost::is_any_of(";"));
	BOOST_FOREACH(std::string  pergroup, gs)
	{
		std::vector<std::string> group;
	 	boost::split(group, pergroup, boost::is_any_of(","));
		messagegroup g(&qqclient,&xmppclient,&ircclient);
		g.add_channel(group);
		messagegroups.push_back(g);
	}
}

// 简单的消息命令控制.
static void qqbot_control(webqq & qqclient, qqGroup & group, qqBuddy &who, std::string cmd)
{
	boost::regex ex;
	boost::cmatch what;
    boost::trim(cmd);

    if (who.nick == L"水手(Jack)" || who.nick == L"Cai==天马博士")
	{
		// 转发图片处理.
		if (cmd == ".qqbot start image")
		{
			resend_img = true;
		}

		if (cmd == ".qqbot stop image")
		{
			resend_img = false;
		}

		// 重新加载群成员列表.
		if (cmd == ".qqbot reload")
		{
			qqclient.update_group_member(group);
			qqclient.send_group_message(group, "群成员列表重加载", qq_msg_sended);
		}

		// 开始讲座记录.	
		ex.set_expression(".qqbot begin class ?\"(.*)?\"");
		
		if(boost::regex_match(cmd.c_str(), what, ex))
		{
			std::string title = what[1];
			if (title.empty()) return ;
			if (!logfile.begin_lecture(group.qqnum, title))
			{
				printf("lecture failed!\n");
			}
		}

		// 停止讲座记录.
		if (cmd == ".qqbot end class")
		{
			logfile.end_lecture();
		}
		
		// 向新人问候.
		ex.set_expression(".qqbot newbee ?(.*)?");
		if(boost::regex_match(cmd.c_str(), what, ex))
		{
			std::string nick = what[1];
			
			if (nick.empty())
				return;
				
			auto_question::value_qq_list list;			
			list.push_back(nick);
			
			question.add_to_list(list);
			question.on_handle_message(group, qqclient);
		}
	}
}

static void irc_message_got(const IrcMsg pMsg,  webqq & qqclient, IrcClient &ircclient, xmpp& xmppclient)
{
	std::cout <<  pMsg.msg<< std::endl;

	std::string from = std::string("irc:") + pMsg.from.substr(1);

	//验证码check
	if(qqneedvc){
		std::string vc = boost::trim_copy(pMsg.msg);
		if(vc[0] == '.' && vc[1]=='v' && vc[2]=='c' && vc[3] == ' ')
			qqclient.login_withvc(vc.substr(4));
		qqneedvc = false;
		return;
	}

	messagegroup* groups =  find_group(from);
	if(groups){
		std::string forwarder = boost::str(boost::format("%s 说：%s") % pMsg.whom % pMsg.msg);
		groups->forwardmessage(from,forwarder);
	}
}

static void om_xmpp_message(std::string xmpproom, std::string who, std::string message, webqq & qqclient, IrcClient & ircclient, xmpp& xmppclient)
{
	std::string from = std::string("xmpp:") + xmpproom;
	//log to logfile?
	messagegroup* groups =  find_group(from);
	if(groups){
		std::string forwarder = boost::str(boost::format("(%s)说：%s") % who % message);
		groups->forwardmessage(from,forwarder);
	}
}

static void on_group_msg(std::wstring group_code, std::wstring who, const std::vector<qqMsg> & msg, webqq & qqclient, IrcClient & ircclient, xmpp& xmppclient)
{
	qqBuddy *buddy = NULL;
	qqGroup *group = qqclient.get_Group_by_gid(group_code);
	std::wstring groupname = group_code;
	if (group)
		groupname = group->name;
	buddy = group ? group->get_Buddy_by_uin(who) : NULL;
	std::wstring nick = who;
	if (buddy)
	{
		if (buddy->card.empty())
			nick = buddy->nick;
		else
			nick = buddy->card;
	}

	std::wstring message_nick, message;
	std::string ircmsg;

	message_nick += nick;
	message_nick += L" 说：";
	
	ircmsg = boost::str(boost::format("qq(%s): ") % wide_utf8(nick));

	BOOST_FOREACH(qqMsg qqmsg, msg)
	{
      	std::wstring buf;
		switch (qqmsg.type)
		{
			case qqMsg::LWQQ_MSG_TEXT:
			{
				buf = qqmsg.text;
				ircmsg += wide_utf8(buf);
				if (!buf.empty()) {
					boost::replace_all(buf, L"&", L"&amp;");
					boost::replace_all(buf, L"<", L"&lt;");
					boost::replace_all(buf, L">", L"&gt;");
					boost::replace_all(buf, L"  ", L"&nbsp;");
				}
			}
			break;
			case qqMsg::LWQQ_MSG_CFACE:			
			{
				buf = boost::str(boost::wformat(
				L"<img src=\"http://w.qq.com/cgi-bin/get_group_pic?pic=%s\" > ")
				% qqmsg.cface);
				std::string imgurl = boost::str(
					boost::format(" http://w.qq.com/cgi-bin/get_group_pic?pic=%s ")
						% url_encode(wide_utf8(qqmsg.cface))
				);
				if (resend_img){
					//TODO send it
					qqclient.send_group_message(group_code, imgurl, qq_msg_sended);
				}
				ircmsg += imgurl;
			}break;
			case qqMsg::LWQQ_MSG_FACE:
			{
				buf = boost::str(boost::wformat(
					L"<img src=\"http://0.web.qstatic.com/webqqpic/style/face/%d.gif\" >") % qqmsg.face);
				ircmsg += wide_utf8(buf);
			}break;
		}
		message += buf;
	}

	// 统计发言.
	cnt.increace(wide_utf8(nick));
	cnt.save();

	// 记录.
	printf("%ls%ls\n", message_nick.c_str(),  message.c_str());
	if (!group)
		return;
	// qq消息控制.
	if (buddy)
		qqbot_control(qqclient, *group, *buddy, wide_utf8(message));

	logfile.add_log(group->qqnum, wide_utf8(message_nick + message));
	// send to irc

	std::string from = std::string("qq:") + wide_utf8(group->qqnum);

	messagegroup* groups =  find_group(from);
	
	if(groups){
		groups->forwardmessage(from,ircmsg);
	}
}

void on_verify_code(const boost::asio::const_buffer & imgbuf,webqq & qqclient, IrcClient & ircclient, xmpp& xmppclient)
{
	const char * data = boost::asio::buffer_cast<const char*>(imgbuf);
	size_t	imgsize = boost::asio::buffer_size(imgbuf);
	fs::path imgpath = fs::path(logfile.log_path()) / "vercode.jpeg";
	std::ofstream	img(imgpath.c_str(),std::ios::binary|std::ios::out);
	img.write(data,imgsize);
	qqneedvc = true;
	// send to xmpp and irc
	ircclient.chat(std::string("#") + ircvercodechannel,"输入qq验证码");
}

static fs::path configfilepath()
{
	if ( fs::exists ( fs::path ( progname ) / "qqbotrc" ) )
		return fs::path ( progname ) / "qqbotrc";

	if ( getenv ( "USERPROFILE" ) ) {
		if ( fs::exists ( fs::path ( getenv ( "USERPROFILE" ) ) / ".qqbotrc" ) )
			return fs::path ( getenv ( "USERPROFILE" ) ) / ".qqbotrc";
	}

	if ( getenv ( "HOME" ) ) {
		if ( fs::exists ( fs::path ( getenv ( "HOME" ) ) / ".qqbotrc" ) )
			return fs::path ( getenv ( "HOME" ) ) / ".qqbotrc";
	}

	if ( fs::exists ( "./qqbotrc/.qqbotrc" ) )
		return fs::path ( "./qqbotrc/.qqbotrc" );

	if ( fs::exists ( "/etc/qqbotrc" ) )
		return fs::path ( "/etc/qqbotrc" );

	throw "not configfileexit";
}

#ifdef WIN32
int daemon(int nochdir, int noclose)
{
	// nothing...
	return -1;
}
#endif // WIN32

int main(int argc, char *argv[])
{
    std::string qqnumber, qqpwd;
    std::string ircnick, ircroom, ircpwd;
    std::string xmppuser, xmpppwd, xmpproom;
    std::string cfgfile;
	std::string logdir;
	std::string chanelmap;

    bool isdaemon=false;

    progname = fs::basename(argv[0]);

    setlocale(LC_ALL, "");

	po::options_description desc("qqbot options");
	desc.add_options()
	    ( "version,v",										"output version" )
		( "help,h",											"produce help message" )
		( "qqnum,u",	po::value<std::string>(&qqnumber),	"QQ number" )
		( "qqpwd,p",	po::value<std::string>(&qqpwd),		"QQ password" )
		( "logdir",		po::value<std::string>(&logdir),	"dir for logfile" )
		( "daemon,d",	po::value<bool>       (&isdaemon),	"go to background" )
		( "ircnick",	po::value<std::string>(&ircnick),	"irc nick" )
		( "ircpwd",		po::value<std::string>(&ircpwd),	"irc password" )
		( "ircrooms",	po::value<std::string>(&ircroom),	"irc room" )
		( "xmppuser",	po::value<std::string>(&xmppuser),	"id for XMPP,  eg: (microcaicai@gmail.com)" )
		( "xmpppwd",	po::value<std::string>(&xmpppwd),	"password for XMPP" )
		( "xmpprooms",	po::value<std::string>(&xmpproom),	"xmpp rooms" )
		( "map",		po::value<std::string>(&chanelmap),	"map qqgroup to irc channel. eg: --map:qq:12345,irc:avplayer;qq:56789,irc:ubuntu-cn" )
		;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		std::cerr <<  desc <<  std::endl;
		return 1;
	}
	if (vm.size() ==0 )
	{
		try
		{
			fs::path p = configfilepath();
			po::store(po::parse_config_file<char>(p.string().c_str(), desc), vm);
			po::notify(vm);
		}
		catch(char* e)
		{
			std::cerr << e << std::endl;
		}
	}
	if (vm.count("version"))
	{
		printf("qqbot version %s \n", QQBOT_VERSION);
	}
	
	if (!logdir.empty())
	{
		if (!fs::exists(logdir))
			fs::create_directory(logdir);
	}
	// 设置到中国的时区，否则 qq 消息时间不对啊.
	setenv("TZ","Asia/Shanghai",1);

	// 设置日志自动记录目录.
	if (! logdir.empty()){
		logfile.log_path(logdir);
		chdir(logdir.c_str());
	}

    if (isdaemon)
		daemon(0, 0);

	boost::asio::io_service asio;

	xmpp		xmppclient(asio, xmppuser, xmpppwd);
	webqq		qqclient(asio, qqnumber, qqpwd);
	IrcClient	ircclient(asio, ircnick, ircpwd);

	build_group(chanelmap,qqclient,xmppclient,ircclient);
		
	xmppclient.on_room_message(boost::bind(&om_xmpp_message, _1, _2, _3, boost::ref(qqclient), boost::ref(ircclient), boost::ref(xmppclient)));
	ircclient.login(boost::bind(&irc_message_got, _1, boost::ref(qqclient), boost::ref(ircclient), boost::ref(xmppclient)));

	qqclient.on_verify_code(boost::bind(on_verify_code,_1, boost::ref(qqclient), boost::ref(ircclient), boost::ref(xmppclient)));
	qqclient.start();
	qqclient.on_group_msg(boost::bind(on_group_msg, _1, _2, _3, boost::ref(qqclient), boost::ref(ircclient), boost::ref(xmppclient)));

	std::vector<std::string> ircrooms;
	boost::split(ircrooms, ircroom, boost::is_any_of(","));
	ircvercodechannel = ircrooms[0];
	BOOST_FOREACH( std::string room , ircrooms)
	{
		ircclient.join(std::string("#") + room);
	}

	std::vector<std::string> xmpprooms;
	boost::split(xmpprooms, xmpproom, boost::is_any_of(","));
	BOOST_FOREACH( std::string room , xmpprooms)
	{
		xmppclient.join(room);
	}

    boost::asio::io_service::work work(asio);
    asio.run();
    return 0;
}
