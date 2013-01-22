/**
 * @file   main.cpp
 * @author microcai <microcaicai@gmail.com>
 * @origal_author mathslinux <riegamaths@gmail.com>
 * 
 */
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
#include "counter.h"

#include "logger.hpp"

#define QQBOT_VERSION "0.0.1"


static qqlog logfile;
static bool resend_img = false;
static bool qqneedvc = false;
static std::string progname;
static std::string ircvercodechannel;

counter cnt;

/*
 * 用来配置是否是在一个组里的，一个组里的群和irc频道相互转发.
 */
static std::vector< std::vector<std::string> > channelgroups;

static bool in_group(const std::vector<std::string> & group, std::string id)
{
	BOOST_FOREACH(const std::string & i ,  group)
	{
		if (i == id )
			return true;
	}
	return false;
}

/*
 * 查找同组的其他聊天室和qq群.
 */
static std::vector<std::string> & find_group(std::string id)
{
	static std::vector<std::string> empty;
	BOOST_FOREACH(std::vector<std::string> & g ,  channelgroups)
	{
		if (in_group(g, id))
			return g;
	}
	return empty;
}

static bool same_group(std::string id1, std::string id2)
{
	std::vector< std::string > g1 = find_group(id1);
	if (g1.empty())
		return false;
	return in_group(g1, id2);
}

//从命令行或者配置文件读取组信息.
static void build_group(std::string chanelmapstring)
{
	std::vector<std::string> gs;
	boost::split(gs, chanelmapstring, boost::is_any_of(";"));
	BOOST_FOREACH(std::string  pergroup, gs)
	{
		std::vector<std::string> group;
	 	boost::split(group, pergroup, boost::is_any_of(","));
	 	channelgroups.push_back(group);
	}
}

static void qq_msg_sended(const boost::system::error_code& ec)
{
	
}


// 简单的消息命令控制.
static void qqbot_control(webqq & qqclient, qqGroup & group, qqBuddy &who, std::string cmd)
{
	
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
		boost::regex ex(".qqbot begin class ?\"(.*)?\"");
		boost::cmatch what;
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
	
	BOOST_FOREACH(std::string groupmember, find_group(from))
	{
		if (groupmember != from){
			if (groupmember[0]=='q' && groupmember[1]=='q')
			{
				qqGroup* group = qqclient.get_Group_by_qq(utf8_wide(groupmember.substr(3)));
				if (group){
					std::string forwarder = boost::str(boost::format("%s 说：%s") % pMsg.whom % pMsg.msg);
					qqclient.send_group_message(*group, forwarder , qq_msg_sended);
					// log into
					logfile.add_log(group->qqnum, std::string("[irc]") + forwarder );
				}
			}else if (groupmember[0]=='i' && groupmember[1]=='r'&&groupmember[2]=='c'){
				//TODO, irc频道之间转发.
				
			}else if (groupmember[0]=='x' && groupmember[1]=='m'&&groupmember[2]=='p'&&groupmember[3]=='p')
			{
				std::string forwarder = boost::str(boost::format("irc(%s)说：%s") % pMsg.whom % pMsg.msg);
				//XMPP
				xmppclient.send_room_message(groupmember.substr(5), forwarder);
			}
		}
	}
}

static void om_xmpp_message(std::string xmpproom, std::string who, std::string message, webqq & qqclient, IrcClient & ircclient, xmpp& xmppclient)
{
	std::string from = std::string("xmpp:") + xmpproom;
	//log to logfile?
	BOOST_FOREACH(std::string groupmember, find_group(from))
	{
		if (groupmember != from){
			if (groupmember[0]=='q' && groupmember[1]=='q')
			{
				qqGroup* group = qqclient.get_Group_by_qq(utf8_wide(groupmember.substr(3)));
				if (group){
					std::string forwarder = boost::str(boost::format("(%s)说：%s") % who % message);
					qqclient.send_group_message(*group, forwarder , qq_msg_sended);
					// log into
					logfile.add_log(group->qqnum, std::string("[xmpp]") + forwarder );
				}
			}else if (groupmember[0]=='i' && groupmember[1]=='r'&&groupmember[2]=='c'){
				//IRC
				std::string formatedmessage = boost::str(boost::format("xmpp(%s)说: %s") % who % message);
				ircclient.chat( std::string("#") + groupmember.substr(4), formatedmessage ); ;

			}else if (groupmember[0]=='x' && groupmember[1]=='m'&&groupmember[2]=='p'&&groupmember[3]=='p')
			{
				//XMPP
			}
		}
	}	
}

static void on_group_msg(std::wstring group_code, std::wstring who, const std::vector<qqMsg> & msg, webqq & qqclient, IrcClient & ircclient, xmpp& xmppclient)
{
    { // count the message.
        std::string w_ = boost::locale::conv::utf_to_utf<char>(who);
        cnt.increace(w_);
        cnt.save();
    }
	qqBuddy * buddy = NULL;
	qqGroup * group = qqclient.get_Group_by_gid(group_code);
	std::wstring	groupname = group_code;
	if (group)
		groupname = group->name;
	buddy = group? group->get_Buddy_by_uin(who):NULL;
	std::wstring nick = who;
	if (buddy){
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

	BOOST_FOREACH(std::string groupmember, find_group(from))
	{
		if (groupmember == from)
			continue;

		if (groupmember[0]=='i' && groupmember[1]=='r'&&groupmember[2]=='c'){
			ircclient.chat(std::string("#") + groupmember.substr(4), ircmsg);
		}else if (groupmember[0]=='x' && groupmember[1]=='m'&&groupmember[2]=='p'&&groupmember[3]=='p')
		{
			//XMPP
			xmppclient.send_room_message(groupmember.substr(5), ircmsg);
		}
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
	if (fs::exists(fs::path(progname) / "qqbotrc"))
		return fs::path(progname) / "qqbotrc";
	if (getenv("USERPROFILE"))
	{
		if (fs::exists(fs::path(getenv("USERPROFILE")) / ".qqbotrc"))
			return fs::path(getenv("USERPROFILE")) / ".qqbotrc";
	}
	if (getenv("HOME")){
		if (fs::exists(fs::path(getenv("HOME")) / ".qqbotrc"))
			return fs::path(getenv("HOME")) / ".qqbotrc";
	}
        if (fs::exists("./qqbotrc/.qqbotrc"))
                return fs::path("./qqbotrc/.qqbotrc");
	if (fs::exists("/etc/qqbotrc"))
		return fs::path("/etc/qqbotrc");
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
	if (vm.size() ==0 ){
        try {
            fs::path p = configfilepath();
            po::store(po::parse_config_file<char>(p.string().c_str(), desc), vm);
            po::notify(vm);
        } catch(char* e) {
            std::cerr << e << std::endl;
        }
	}
	if (vm.count("version"))
	{
		printf("qqbot version %s \n", QQBOT_VERSION);
	}
	
	if (!logdir.empty()){
		if (!fs::exists(logdir))
			fs::create_directory(logdir);
	}

	// 设置日志自动记录目录.
	if (! logdir.empty())
		logfile.log_path(logdir);

	build_group(chanelmap);

    if (isdaemon)
		daemon(0, 0);

	boost::asio::io_service asio;
   


	xmpp		xmppclient(asio, xmppuser, xmpppwd);
	webqq		qqclient(asio, qqnumber, qqpwd);
	IrcClient	ircclient(asio, ircnick, ircpwd);

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
