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

#ifndef QQBOT_VERSION
#define QQBOT_VERSION "unknow"
#endif

static qqlog logfile;			// 用于记录日志文件.
static counter cnt;				// 用于统计发言信息.
static bool resend_img = false;	// 用于标识是否转发图片url.
static bool qqneedvc = false;	// 用于在irc中验证qq登陆.
static auto_question question;	// 自动问问题.
static std::string progname;
static std::string ircvercodechannel;

class messagegroup;
static std::vector<messagegroup>	messagegroups;

struct messagegroup {
	webqq*		qq_;
	xmpp*		xmpp_;
	IrcClient*	irc_;
	// 组号.
	std::vector<std::string> channels;

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

	void broadcast(std::string message){
		forwardmessage("",message);
	}

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
				
				std::string qqnum = chatgroupmember.substr(3);
				logfile.add_log(qqnum, message);
				if(qq_->get_Group_by_qq(qqnum))
					qq_->send_group_message(*qq_->get_Group_by_qq(qqnum), message, boost::lambda::constant(0));
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

enum sender_flags{
	sender_is_op, // 管理员, 群管理员或者频道OP .
	sender_is_normal, // 普通用户.
};

//-------------

// 命令控制, 所有的协议都能享受的命令控制在这里实现.
// msg_sender 是一个函数, on_command 用它发送消息.
static void on_bot_command(boost::asio::io_service& io_service, std::string message, std::string from_channel, std::string sender, sender_flags sender_flag, boost::function<void(std::string)> msg_sender)
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
						"\t.qqbot relogin\n\t.qqbot reload\n"
                        "\t.qqbot start image\t\n.qqbot stop image\n"
                        "\t.qqbot begin class XXX\t\n\t.qqbot end class\n"
                        "\t.qqbot newbee SB")
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

// 简单的消息命令控制.
static void qqbot_control(webqq & qqclient, qqGroup & group, qqBuddy &who, std::string cmd)
{
    boost::trim(cmd);

    messagegroup* chanelgroup = find_group(std::string("qq:") + group.qqnum);
    boost::function<void(std::string)> msg_sender;

    if (chanelgroup){
		msg_sender = boost::bind(&messagegroup::broadcast, chanelgroup,  _1);
	}else{
		msg_sender = boost::bind(static_cast<void (webqq::*)(std::string, std::string, boost::function<void (const boost::system::error_code& ec)>)>(& webqq::send_group_message), &qqclient, group.gid, _1, boost::lambda::constant(0));
	}

	sender_flags sender_flag;

	if ((who.mflag & 21) == 21 || who.uin == group.owner )
		sender_flag = sender_is_op;
	else
		sender_flag = sender_is_normal;

	on_bot_command(qqclient.get_ioservice(), cmd, std::string("qq:") + group.qqnum, who.nick, sender_flag, msg_sender);
}

static void on_irc_message(const IrcMsg pMsg, IrcClient & ircclient, webqq & qqclient)
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
    boost::function<void(std::string)> msg_sender;

    if (groups){
		msg_sender = boost::bind(&messagegroup::broadcast, groups,  _1);
	}else{
		msg_sender = boost::bind(&IrcClient::chat, &ircclient, pMsg.from, _1);
	}
	
	on_bot_command(qqclient.get_ioservice(), pMsg.msg, from, pMsg.whom, sender_is_normal, msg_sender);
}

static void om_xmpp_message(std::string xmpproom, std::string who, std::string message)
{
	std::string from = std::string("xmpp:") + xmpproom;
	//log to logfile?
	messagegroup* groups =  find_group(from);
	if(groups){
		std::string forwarder = boost::str(boost::format("(%s)说：%s") % who % message);
		groups->forwardmessage(from,forwarder);
	}
}

static bool logqqnumber = false;

static void on_group_msg(std::string group_code, std::string who, const std::vector<qqMsg> & msg, webqq & qqclient)
{
	qqBuddy *buddy = NULL;
	qqGroup *group = qqclient.get_Group_by_gid(group_code);
	std::string groupname = group_code;
	if (group)
		groupname = group->name;
	buddy = group ? group->get_Buddy_by_uin(who) : NULL;
	std::string nick = who;
	if (buddy)
	{
		if (buddy->card.empty())
			nick = buddy->nick;
		else
			nick = buddy->card;
	}

	std::string message_nick, message;
	std::string ircmsg;

	message_nick += nick;
	if (logqqnumber)
	{
		message_nick += buddy? std::string(boost::str(boost::format("[%s]") % buddy->qqnum)):"";
	}
	message_nick += " 说：";
	
	ircmsg = boost::str(boost::format("qq(%s): ") % nick);

	BOOST_FOREACH(qqMsg qqmsg, msg)
	{
      	std::string buf;
		switch (qqmsg.type)
		{
			case qqMsg::LWQQ_MSG_TEXT:
			{
				buf = qqmsg.text;
				ircmsg += buf;
				if (!buf.empty()) {
					boost::replace_all(buf, "&", "&amp;");
					boost::replace_all(buf, "<", "&lt;");
					boost::replace_all(buf, ">", "&gt;");
					boost::replace_all(buf, "  ", "&nbsp;");
				}
			}
			break;
			case qqMsg::LWQQ_MSG_CFACE:			
			{
				buf = boost::str(boost::format(
				"<img src=\"http://w.qq.com/cgi-bin/get_group_pic?pic=%s\" > ")
				% qqmsg.cface);
				std::string imgurl = boost::str(
					boost::format(" http://w.qq.com/cgi-bin/get_group_pic?pic=%s ")
						% url_encode(qqmsg.cface)
				);
				if (resend_img){
					//TODO send it
					qqclient.send_group_message(group_code, imgurl, boost::lambda::constant(0) == 0);
				}
				ircmsg += imgurl;
			}break;
			case qqMsg::LWQQ_MSG_FACE:
			{
				buf = boost::str(boost::format(
					"<img src=\"http://0.web.qstatic.com/webqqpic/style/face/%d.gif\" >") % qqmsg.face);
				ircmsg += buf;
			}break;
		}
		message += buf;
	}

	// 统计发言.
	cnt.increace(nick);
	cnt.save();

	// 记录.
	std::printf("%s%s\n", message_nick.c_str(),  message.c_str());
	if (!group)
		return;
	// qq消息控制.
	if (buddy)
		qqbot_control(qqclient, *group, *buddy, message);

	logfile.add_log(group->qqnum, message_nick + message);
	// send to irc

	std::string from = std::string("qq:") + group->qqnum;

	messagegroup* groups =  find_group(from);
	
	if(groups){
		groups->forwardmessage(from,ircmsg);
	}
}

static void on_mail(mailcontent mail, mx::pop3::call_to_continue_function call_to_contiune, webqq & qqclient)
{
	if (qqclient.is_online()){
		BOOST_FOREACH(messagegroup & g ,  messagegroups)
		{
			if (g.in_group("mail"))
			{
				g.broadcast(boost::str(
					boost::format("[QQ邮件]\n发件人:%s\n收件人:%s\n主题:%s\n\n%s")
					% mail.from % mail.to % mail.subject % mail.content
				));
			}
		}
	}
	qqclient.get_ioservice().post(boost::bind(call_to_contiune, qqclient.is_online()));
}

static void on_verify_code(const boost::asio::const_buffer & imgbuf,webqq & qqclient, IrcClient & ircclient, xmpp& xmppclient)
{
	const char * data = boost::asio::buffer_cast<const char*>(imgbuf);
	size_t	imgsize = boost::asio::buffer_size(imgbuf);
	fs::path imgpath = fs::path(logfile.log_path()) / "vercode.jpeg";
	std::ofstream	img(imgpath.c_str(),std::ios::binary|std::ios::out);
	img.write(data,imgsize);
	qqneedvc = true;
	// send to xmpp and irc
	ircclient.chat(boost::str(boost::format("#%s") % ircvercodechannel),"输入qq验证码");
	std::cerr << "请输入验证码" ;
}

#ifdef WIN32
int daemon(int nochdir, int noclose)
{
	// nothing...
	return -1;
}
#endif // WIN32

#include "input.ipp"
#include "fsconfig.ipp"

int main(int argc, char *argv[])
{
    std::string qqnumber, qqpwd;
    std::string ircnick, ircroom, ircpwd;
    std::string xmppuser, xmppserver, xmpppwd, xmpproom, xmppnick;
    std::string cfgfile;
	std::string logdir;
	std::string chanelmap;
	std::string mailaddr,mailpasswd,pop3server, smtpserver;

    progname = fs::basename(argv[0]);

    setlocale(LC_ALL, "");

	po::options_description desc("qqbot options");
	desc.add_options()
	    ( "version,v",										"output version" )
		( "help,h",											"produce help message" )
		( "daemon,d",										"go to background" )
		( "logqqnumber",po::value<bool>(&logqqnumber),		"let qqlog contain qqnumber")
		( "qqnum,u",	po::value<std::string>(&qqnumber),	"QQ number" )
		( "qqpwd,p",	po::value<std::string>(&qqpwd),		"QQ password" )
		( "logdir",		po::value<std::string>(&logdir),	"dir for logfile" )
		( "ircnick",	po::value<std::string>(&ircnick),	"irc nick" )
		( "ircpwd",		po::value<std::string>(&ircpwd),	"irc password" )
		( "ircrooms",	po::value<std::string>(&ircroom),	"irc room" )
		( "xmppuser",	po::value<std::string>(&xmppuser),	"id for XMPP,  eg: (microcaicai@gmail.com)" )
		( "xmppserver",	po::value<std::string>(&xmppserver),"server to connect for XMPP,  eg: (xmpp.l.google.com)" )
		( "xmpppwd",	po::value<std::string>(&xmpppwd),	"password for XMPP" )
		( "xmpprooms",	po::value<std::string>(&xmpproom),	"xmpp rooms" )
		( "xmppnick",	po::value<std::string>(&xmppnick),	"nick in xmpp rooms" )	
		( "map",		po::value<std::string>(&chanelmap),	"map channels. eg: --map=qq:12345,irc:avplayer;qq:56789,irc:ubuntu-cn" )
		( "mail",		po::value<std::string>(&mailaddr),	"fetch mail from this address")
		( "mailpasswd",	po::value<std::string>(&mailpasswd),"password of mail")
		( "pop3server",	po::value<std::string>(&pop3server),"pop server of mail,  default to pop.[domain]")
		( "smtpserver",	po::value<std::string>(&smtpserver),"smtp server of mail,  default to smtp.[domain]")
		;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		std::cerr <<  desc <<  std::endl;
		return 1;
	}
	if (vm.size() ==0 || (vm.size() ==1 && vm.count("daemon")))
	{
		try
		{
			fs::path p = configfilepath();
			po::store(po::parse_config_file<char>(p.string().c_str(), desc), vm);
			po::notify(vm);
		}
		catch(char* e)
		{
			std::cout <<  "no command line arg and config file not found neither." <<  std::endl;
			std::cout <<  "try to add command line arg or put config file in /etc/qqbotrc or ~/.qqbotrc" <<  std::endl;
		}
	}

	if (vm.count("daemon"))
		daemon(0, 1);
		
	if (vm.count("version"))
	{
		printf("qqbot version %s (%s %s) \n", QQBOT_VERSION, __DATE__, __TIME__);
		exit(EXIT_SUCCESS);
	}
	
	if (!logdir.empty())
	{
		if (!fs::exists(logdir))
			fs::create_directory(logdir);
	}
	// 设置到中国的时区，否则 qq 消息时间不对啊.
	putenv((char*)"TZ=Asia/Shanghai");

	// 设置日志自动记录目录.
	if (! logdir.empty()){
		logfile.log_path(logdir);
		chdir(logdir.c_str());
	}

	if( qqnumber.empty()|| qqpwd.empty() )
	{
		std::cerr << "请设置qq号码和密码" << std::endl;
		exit(1);
	}

	if( ircnick.empty() )
	{
		std::cerr << "请设置irc昵称" << std::endl;
		exit(1);
	}

	if( ircroom.empty() )
	{
		std::cerr << "请设置irc频道" << std::endl;
		exit(1);
	}

	boost::asio::io_service asio;

	xmpp		xmppclient(asio, xmppuser, xmpppwd, xmppserver, xmppnick);
	webqq		qqclient(asio, qqnumber, qqpwd);
	IrcClient	ircclient(asio, ircnick, ircpwd);
	mx::mx		mx(asio, mailaddr, mailpasswd, pop3server, smtpserver);

	build_group(chanelmap,qqclient,xmppclient,ircclient);

	xmppclient.on_room_message(boost::bind(&om_xmpp_message, _1, _2, _3));
	ircclient.login(boost::bind(&on_irc_message, _1, boost::ref(ircclient), boost::ref(qqclient)));

	qqclient.on_verify_code(boost::bind(on_verify_code,_1, boost::ref(qqclient), boost::ref(ircclient), boost::ref(xmppclient)));
	qqclient.login();
	qqclient.on_group_msg(boost::bind(on_group_msg, _1, _2, _3, boost::ref(qqclient)));

	mx.async_fetch_mail(boost::bind(on_mail,_1, _2, boost::ref(qqclient)));

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
	if (!vm.count("daemon")){
#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR
		boost::shared_ptr<boost::asio::posix::stream_descriptor> stdin(new boost::asio::posix::stream_descriptor(asio, 0));
		boost::shared_ptr<boost::asio::streambuf> inputbuffer(new boost::asio::streambuf);
		boost::asio::async_read_until(*stdin, *inputbuffer, '\n', boost::bind(inputread , _1,_2, stdin, inputbuffer, boost::ref(qqclient)));
#else
		boost::thread(boost::bind(input_thread, boost::ref(asio), boost::ref(qqclient)));
#endif
	}
    asio.run();
    return 0;
}
