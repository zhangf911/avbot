/**
 * @file   main.cpp
 * @author microcai <microcaicai@gmail.com>
 *
 */
#include <string>
#include <algorithm>
#include <vector>
#include <signal.h>
#include <fstream>

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
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
#include <cstring>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>
#if defined(_WIN32)
#include <direct.h>

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"

// 据说mingw32不支持这个
//#pragma comment(lib, "Comctl32.lib")
//#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#ifndef WINVER
#define WINVER 0x0501
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif	

#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410
#endif

#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif

#define WIN32_LEAN_AND_MEAN
#endif

#include "libirc/irc.h"
#include "libwebqq/webqq.h"
#include "libwebqq/url.hpp"
#include "libxmpp/xmpp.h"
#include "libmailexchange/mx.hpp"

#include "counter.hpp"
#include "logger.hpp"

#include "messagegroup.hpp"
#include "botctl.hpp"

#include "boost/consolestr.hpp"

#ifndef QQBOT_VERSION
#define QQBOT_VERSION "unknow"
#endif

#if defined(_WIN32)

// 重启qqbot
#define WM_RESTART_AV_BOT WM_USER + 5

// 选项设置框框的消息回调函数
BOOL CALLBACK DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	BOOL fError; 

	switch (message) 
	{ 
	case WM_INITDIALOG: 
		if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_XMPP) != BST_CHECKED) {
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_CHANNEL), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_NICK), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_PWD), FALSE);
		}
		return TRUE; 
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		case IDOK:
			// 通知主函数写入数据并重启
			PostMessage(NULL, WM_RESTART_AV_BOT, 0, 0);
			return TRUE; 
		case IDCANCEL: 
			DestroyWindow(hwndDlg); 
			// 退出消息循环
			PostMessage(NULL, WM_QUIT, 0, 0);
			return TRUE; 
		case IDC_CHECK_XMPP:
			BOOL enable = FALSE;
			if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_XMPP) == BST_CHECKED) enable = TRUE;
			
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_CHANNEL), enable);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_NICK), enable);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_PWD), enable);
			return TRUE;
		} 
	} 

	return FALSE; 
} 

#endif

char * execpath;
qqlog logfile;			// 用于记录日志文件.

static counter cnt;				// 用于统计发言信息.
static bool qqneedvc = false;	// 用于在irc中验证qq登陆.
static std::string progname;
static std::string ircvercodechannel;

static std::string preamble_qq_fmt, preamble_irc_fmt, preamble_xmpp_fmt;

// 简单的消息命令控制.
static void qqbot_control( webqq & qqclient, qqGroup & group, qqBuddy &who, std::string cmd )
{
	boost::trim( cmd );

	messagegroup* chanelgroup = find_group( std::string( "qq:" ) + group.qqnum );
	boost::function<void( std::string )> msg_sender;

	if( chanelgroup ) {
		msg_sender = boost::bind( &messagegroup::broadcast, chanelgroup,  _1 );
	} else {

		typedef void ( webqq::*webqq_send_group_message )
			( std::string, std::string, boost::function<void ( const boost::system::error_code & ec )> );

		msg_sender = boost::bind( static_cast<webqq_send_group_message>( & webqq::send_group_message ),
									&qqclient, group.gid, _1, boost::lambda::constant( 0 )
								);
	}

	sender_flags sender_flag;

	if( ( who.mflag & 21 ) == 21 || who.uin == group.owner )
		sender_flag = sender_is_op;
	else
		sender_flag = sender_is_normal;

	on_bot_command( qqclient.get_ioservice(), cmd, std::string( "qq:" ) + group.qqnum, who.nick, sender_flag, msg_sender, &qqclient );
}


static std::string	preamble_formater( qqBuddy *buddy, std::string falbacknick, qqGroup * grpup = NULL )
{
	static qqBuddy _buddy;
	std::string preamble;
	// 格式化神器, 哦耶.
	// 获取格式化描述字符串.
	std::string preamblefmt = preamble_qq_fmt;
	// 支持的格式化类型有 %u UID,  %q QQ号, %n 昵称,  %c 群名片 %a 自动.

	preamble = preamblefmt;
	std::string autonick = "";

	if( !buddy ) {
		autonick = falbacknick;
		buddy = & _buddy;
	} else {
		autonick = buddy->card;

		if( autonick.empty() ) {
			autonick = buddy->nick;
		}

		if( autonick.empty() ) {
			autonick = buddy->qqnum;
		}

		if( autonick.empty() ) {
			autonick = buddy->uin;
		}
	}

	boost::replace_all( preamble, "%a", autonick );
	boost::replace_all( preamble, "%n", buddy->nick );
	boost::replace_all( preamble, "%u", buddy->uin );
	boost::replace_all( preamble, "%q", buddy->qqnum );
	boost::replace_all( preamble, "%c", buddy->card );

	if( grpup )
		boost::replace_all( preamble, "%r", grpup->qqnum );

	return preamble;
}

static std::string	preamble_formater( IrcMsg pmsg )
{
	// 格式化神器, 哦耶.
	// 获取格式化描述字符串.
	std::string preamble = preamble_irc_fmt;

	// 支持的格式化类型有 %u UID,  %q QQ号, %n 昵称,  %c 群名片 %a 自动 %r irc 房间.
	// 默认为 qq(%a) 说:.
	boost::replace_all( preamble, "%a", pmsg.whom );
	boost::replace_all( preamble, "%r", pmsg.from );
	boost::replace_all( preamble, "%n", pmsg.whom );
	return preamble;
}

static std::string	preamble_formater( std::string who, std::string room )
{
	// 格式化神器, 哦耶.
	// 获取格式化描述字符串.
	std::string preamble = preamble_xmpp_fmt;
	// 支持的格式化类型有 %u UID,  %q QQ号, %n 昵称,  %c 群名片 %a 自动 %r irc 房间.

	boost::replace_all( preamble, "%a", who );
	boost::replace_all( preamble, "%r", room );
	boost::replace_all( preamble, "%n", who );
	return preamble;
}

static void on_irc_message( IrcMsg pMsg, IrcClient & ircclient, webqq & qqclient )
{
	std::cout << console_out_str(pMsg.msg) << std::endl;

	boost::trim( pMsg.msg );

	std::string from = std::string( "irc:" ) + pMsg.from.substr( 1 );

	//验证码check.
	if( qqneedvc ) {
		std::string vc = boost::trim_copy( pMsg.msg );

		if( vc[0] == '.' && vc[1] == 'v' && vc[2] == 'c' && vc[3] == ' ' )
			qqclient.login_withvc( vc.substr( 4 ) );

		qqneedvc = false;
		return;
	}

	std::string forwarder = boost::str( boost::format( "%s %s" ) % preamble_formater( pMsg ) % pMsg.msg );
	forwardmessage( from, forwarder );

	boost::function<void( std::string )> msg_sender;
	messagegroup* groups =  find_group( from );

	if( groups ) {
		msg_sender = boost::bind( &messagegroup::broadcast, groups,  _1 );
	} else {
		msg_sender = boost::bind( &IrcClient::chat, &ircclient, pMsg.from, _1 );
	}

	sender_flags sender_flag = sender_is_normal;

	// a hack, later should be fixed to fetch channel op list.
	if( pMsg.whom == "microcai" )
		sender_flag = sender_is_op;

	on_bot_command( qqclient.get_ioservice(), pMsg.msg, from, pMsg.whom, sender_flag, msg_sender, &qqclient );
}

static void om_xmpp_message( xmpp & xmppclient, std::string xmpproom, std::string who, std::string message )
{
	std::string from = std::string( "xmpp:" ) + xmpproom;

	std::string forwarder = boost::str( boost::format( "%s %s" ) % preamble_formater( who, xmpproom ) % message );
	forwardmessage( from, forwarder );

	boost::function<void( std::string )> msg_sender;
	messagegroup* groups =  find_group( from );

	if( groups ) {
		msg_sender = boost::bind( &messagegroup::broadcast, groups,  _1 );
	} else {
		msg_sender = boost::bind( &xmpp::send_room_message, &xmppclient, xmpproom, _1 );
	}

	on_bot_command( xmppclient.get_ioservice(), message, from, who, sender_is_normal, msg_sender );
}

static std::string 	base_image_url = "http://w.qq.com/cgi-bin/get_group_pic?pic=";

static void save_image(const boost::system::error_code & ec, boost::asio::streambuf & buf, std::string cface)
{
	if (!ec || ec == boost::asio::error::eof){
		std::ofstream cfaceimg((std::string("images/") + cface).c_str(), std::ofstream::binary|std::ofstream::out);
		cfaceimg.write(boost::asio::buffer_cast<const char*>(buf.data()), boost::asio::buffer_size(buf.data()));
	}
}

static void on_group_msg( std::string group_code, std::string who, const std::vector<qqMsg> & msg, webqq & qqclient )
{
	qqBuddy *buddy = NULL;
	qqGroup_ptr group = qqclient.get_Group_by_gid( group_code );
	std::string groupname = group_code;

	if( group )
		groupname = group->name;

	buddy = group ? group->get_Buddy_by_uin( who ) : NULL;

	std::string message_nick = preamble_formater( buddy, who, group.get() );

	std::string htmlmsg;
	std::string textmsg;

	BOOST_FOREACH( qqMsg qqmsg, msg ) {
		std::string buf;

		switch( qqmsg.type ) {
			case qqMsg::LWQQ_MSG_TEXT: {
				buf = qqmsg.text;
				textmsg += buf;

				if( !buf.empty() ) {
					boost::replace_all( buf, "&", "&amp;" );
					boost::replace_all( buf, "<", "&lt;" );
					boost::replace_all( buf, ">", "&gt;" );
					boost::replace_all( buf, "  ", "&nbsp;" );
				}
			}
			break;
			case qqMsg::LWQQ_MSG_CFACE: {
				buf = boost::str( boost::format(
									  "<img src=\"%s%s\" > " ) % base_image_url
								  % url_encode( qqmsg.cface ) );
				std::string imgurl = boost::str(
										boost::format( " http://w.qq.com/cgi-bin/get_group_pic?pic=%s " )
										% url_encode( qqmsg.cface )
									);
				textmsg += imgurl;
				if (base_image_url != "http://w.qq.com/cgi-bin/get_group_pic?pic="){
					// save to disk
					// 先检查这样的图片本地有没有，有你fetch的P啊.
					if (!fs::exists(std::string("images/") + qqmsg.cface))
					{
						if (!fs::exists("images"))
							fs::create_directories("images");
						qqclient.async_fetch_cface(qqmsg.cface, boost::bind(save_image, _1, _2, qqmsg.cface));
					}
				}
			} break;
			case qqMsg::LWQQ_MSG_FACE: {
				buf = boost::str( boost::format(
									  "<img src=\"http://0.web.qstatic.com/webqqpic/style/face/%d.gif\" >" ) % qqmsg.face );
				textmsg += buf;
			} break;
		}

		htmlmsg += buf;
	}

	// 统计发言.
	if( buddy && buddy->qqnum.size() )
		cnt.increace( buddy->qqnum );

	cnt.save();

	// 记录.
	std::cout << console_out_str(message_nick) <<  console_out_str(htmlmsg) <<  std::endl;

	if( !group )
		return;

	logfile.add_log( group->qqnum, message_nick + htmlmsg );
	// send to irc

	std::string from = std::string( "qq:" ) + group->qqnum;

	forwardmessage( from, message_nick + textmsg );

	// qq消息控制.
	if( buddy )
		qqbot_control( qqclient, *group, *buddy, htmlmsg );
}

static void on_mail( mailcontent mail, mx::pop3::call_to_continue_function call_to_contiune, webqq & qqclient )
{
	if( qqclient.is_online() ) {

		forwardmessage( "mail",
						boost::str(
							boost::format( "[QQ邮件]\n发件人:%s\n收件人:%s\n主题:%s\n\n%s" )
							% mail.from % mail.to % mail.subject % mail.content
						)
					  );
	}

	qqclient.get_ioservice().post( boost::bind( call_to_contiune, qqclient.is_online() ) );
}

static void on_verify_code( const boost::asio::const_buffer & imgbuf, webqq & qqclient, IrcClient & ircclient, xmpp& xmppclient )
{
	const char * data = boost::asio::buffer_cast<const char*>( imgbuf );
	size_t	imgsize = boost::asio::buffer_size( imgbuf );
	fs::path imgpath = fs::path( logfile.log_path() ) / "vercode.jpeg";
	std::ofstream	img( imgpath.string().c_str(), std::ofstream::openmode(std::ofstream::binary | std::ofstream::out) );
	img.write( data, imgsize );
	qqneedvc = true;
	// send to xmpp and irc
	ircclient.chat( boost::str( boost::format( "#%s" ) % ircvercodechannel ), "输入qq验证码：" );
	std::cerr << console_out_str("请查看qqlog目录下的vercode.jpeg 然后输入验证码:") ;
}

#ifdef WIN32
int daemon( int nochdir, int noclose )
{
	// nothing...
	return -1;
}
#endif // WIN32

#include "input.ipp"
#include "fsconfig.ipp"

int main( int argc, char *argv[] )
{
	std::string qqnumber, qqpwd;
	std::string ircnick, ircroom, ircpwd;
	std::string xmppuser, xmppserver, xmpppwd, xmpproom, xmppnick;
	std::string cfgfile;
	std::string logdir;
	std::string chanelmap;
	std::string mailaddr, mailpasswd, pop3server, smtpserver;

	bool localimage;

	progname = fs::basename( argv[0] );

	setlocale( LC_ALL, "" );
	po::variables_map vm;
	po::options_description desc( "qqbot options" );
	desc.add_options()
	( "version,v", 	"output version" )
	( "help,h", 	"produce help message" )
	( "daemon,d", 	"go to background" )

	( "qqnum,u",	po::value<std::string>( &qqnumber ), 	"QQ number" )
	( "qqpwd,p",	po::value<std::string>( &qqpwd ), 		"QQ password" )
	( "logdir",		po::value<std::string>( &logdir ), 		"dir for logfile" )
	( "ircnick",	po::value<std::string>( &ircnick ), 	"irc nick" )
	( "ircpwd",		po::value<std::string>( &ircpwd ), 		"irc password" )
	( "ircrooms",	po::value<std::string>( &ircroom ), 	"irc room" )
	( "xmppuser",	po::value<std::string>( &xmppuser ), 	"id for XMPP,  eg: (microcaicai@gmail.com)" )
	( "xmppserver",	po::value<std::string>( &xmppserver ), 	 "server to connect for XMPP,  eg: (xmpp.l.google.com)" )
	( "xmpppwd",	po::value<std::string>( &xmpppwd ), 	"password for XMPP" )
	( "xmpprooms",	po::value<std::string>( &xmpproom ), 	"xmpp rooms" )
	( "xmppnick",	po::value<std::string>( &xmppnick ), 	"nick in xmpp rooms" )
	( "map",		po::value<std::string>( &chanelmap ), 	"map channels. eg: --map=qq:12345,irc:avplayer;qq:56789,irc:ubuntu-cn" )
	( "mail",		po::value<std::string>( &mailaddr ), 	"fetch mail from this address" )
	( "mailpasswd",	po::value<std::string>( &mailpasswd ), 	"password of mail" )
	( "pop3server",	po::value<std::string>( &pop3server ), 	"pop server of mail,  default to pop.[domain]" )
	( "smtpserver",	po::value<std::string>( &smtpserver ), 	"smtp server of mail,  default to smtp.[domain]" )

	( "localimage", po::value<bool>( &localimage)->default_value(false),	"fetch qq image to local disk and store it there")

	( "preambleqq",		po::value<std::string>( &preamble_qq_fmt )->default_value( console_out_str("qq(%a)：") ),
		console_out_str("为QQ设置的发言前缀, 默认是 qq(%a):").c_str() )
	( "preambleirc",	po::value<std::string>( &preamble_irc_fmt )->default_value( console_out_str("%a 说：") ),
	  console_out_str("为IRC设置的发言前缀, 默认是 %a 说: ").c_str() )
	( "preamblexmpp",	po::value<std::string>( &preamble_xmpp_fmt )->default_value( console_out_str("(%a)：") ),
		console_out_str(
	  "为XMPP设置的发言前缀, 默认是 (%a): \n\n "
	  "前缀里的含义 \n"
	  "\t %a 为自动选择\n\t %q 为QQ号码\n\t %n 为昵称\n\t %c 为群名片 \n"
	  "\t %r为房间名(群号, XMPP房名, IRC频道名) \n"
	  "可以包含多个, 例如想记录QQ号码的可以使用 qq(%a, %q)说: \n"
	  "注意在shell下可能需要使用\\(来转义(\n配置文件无此问题 \n\n").c_str()	)
	;

	po::store( po::parse_command_line( argc, argv, desc ), vm );
	po::notify( vm );

	if( vm.count( "help" ) ) {
		std::cerr <<  desc <<  std::endl;
		return 1;
	}
#ifdef WIN32
	// 从 windows 控制台输入的能有啥好编码，转到utf8吧.
	preamble_qq_fmt = ansi_utf8(preamble_qq_fmt);
	preamble_irc_fmt = ansi_utf8(preamble_irc_fmt);
	preamble_xmpp_fmt = ansi_utf8(preamble_xmpp_fmt);
#endif

	if( qqnumber.empty() ) {
		try {
			fs::path p = configfilepath();
			po::store( po::parse_config_file<char>( p.string().c_str(), desc ), vm );
			po::notify( vm );
		} catch( char const* e ) {
			std::cout <<  "no command line arg and config file not found neither." <<  std::endl;
			std::cout <<  "try to add command line arg or put config file in /etc/qqbotrc or ~/.qqbotrc" <<  std::endl;
		}
	}


	if( vm.count( "daemon" ) ) {
		daemon( 0, 0 );
	}

	if( vm.count( "version" ) ) {
		printf( "qqbot version %s (%s %s) \n", QQBOT_VERSION, __DATE__, __TIME__ );
		exit( EXIT_SUCCESS );
	}

	if( !logdir.empty() ) {
		if( !fs::exists( logdir ) )
			fs::create_directory( logdir );
	}

	if (localimage)
		base_image_url = "../images/";

	// 设置到中国的时区，否则 qq 消息时间不对啊.
	putenv( ( char* )"TZ=Asia/Shanghai" );

	// 设置日志自动记录目录.
	if( ! logdir.empty() ) {
		logfile.log_path( logdir );
		chdir( logdir.c_str() );
	}
	
#ifdef WIN32
	::InitCommonControls();
	// windows下面弹出选项设置框框
	if( qqnumber.empty() || qqpwd.empty() || ircnick.empty() ) {
		HMODULE hIns = GetModuleHandle(NULL);
		HWND hDlg = NULL;

		hDlg = CreateDialog(hIns, MAKEINTRESOURCE(IDD_AVSETTINGS_DIALOG), NULL, (DLGPROC)DlgProc);

		ShowWindow(hDlg, SW_SHOW);

		// 启动windows消息循环
		MSG msg;
		while (true) {
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) exit(1);
				
				if (msg.message == WM_RESTART_AV_BOT) {
					// 看起来avbot用的是多字节,std::string not std::wstring
					TCHAR temp[MAX_PATH];
					bool use_xmpp = false;

					// qq setting				
					GetDlgItemText(hDlg, IDC_EDIT_USER_NAME, temp, MAX_PATH);
					qqnumber = temp;
					
					GetDlgItemText(hDlg, IDC_EDIT_PWD, temp, MAX_PATH);
					qqpwd = temp;

					// irc setting
					GetDlgItemText(hDlg, IDC_EDIT_IRC_CHANNEL, temp, MAX_PATH);
					ircroom = temp;
					
					GetDlgItemText(hDlg, IDC_EDIT_IRC_NICK, temp, MAX_PATH);
					ircnick = temp;
					
					GetDlgItemText(hDlg, IDC_EDIT_IRC_PWD, temp, MAX_PATH);
					ircpwd = temp;

					// xmpp setting
					if (IsDlgButtonChecked(hDlg, IDC_CHECK_XMPP) == BST_CHECKED) {
						GetDlgItemText(hDlg, IDC_EDIT_XMPP_CHANNEL, temp, MAX_PATH);
						xmpproom = temp;
						
						GetDlgItemText(hDlg, IDC_EDIT_XMPP_NICK, temp, MAX_PATH);
						xmppnick = temp;
						
						GetDlgItemText(hDlg, IDC_EDIT_XMPP_PWD, temp, MAX_PATH);
						xmpppwd = temp;
						
						use_xmpp = true;
					}

					// save data to config file					
					try {						
						fs::path config_file;
						if (exist_config_file()) {
							 config_file = configfilepath();
						}
						else {
							std::cerr << "config file not exist." << std::endl;
							TCHAR file_path[MAX_PATH];
							GetModuleFileName(NULL, file_path, MAX_PATH);
							config_file = fs::path(file_path).parent_path();
							config_file += "\\qqbotrc";
						}
						
						fs::ofstream file(config_file);
						
						file << "# qq config" << std::endl;
						file << "qqnum=" << qqnumber << std::endl;
						file << "qqpwd=" << qqpwd << std::endl;
						file << std::endl;
						
						file << "# irc config" << std::endl;
						file << "ircnick=" << ircnick << std::endl;
						file << "ircpwd=" << ircpwd << std::endl;
						file << "ircrooms=" << ircroom << std::endl;
						file << std::endl;
						
						if (use_xmpp) {
							file << "# xmpp config" << std::endl;
							file << "xmppuser=" << xmppnick << std::endl;
							file << "xmpppwd=" << xmpppwd << std::endl;
							file << "xmpproom=" << xmpproom << std::endl;
							file << std::endl;
						}
					}
					catch (...) {
						std::cerr << "error while handle config file" << std::endl;
					}

					TCHAR file_path[MAX_PATH];
					GetModuleFileName(NULL, file_path, MAX_PATH);
					// now, create process
					STARTUPINFO si;
					PROCESS_INFORMATION pi;

					ZeroMemory(&si, sizeof(si));
					si.cb = sizeof(si);
					ZeroMemory(&pi, sizeof(pi));

					CreateProcess(file_path, NULL, NULL, NULL,
						FALSE, 0, NULL, NULL,
						&si, &pi);
					
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
					// now,parent process exit.
					exit(1);					
				}

				TranslateMessage(&msg);	
				DispatchMessage(&msg);	
			}		
		}
	}
		
#endif

	if( qqnumber.empty() || qqpwd.empty() ) {
		std::cerr << console_out_str("请设置qq号码和密码") << std::endl;
		exit( 1 );
	}

	if( ircnick.empty() ) {
		std::cerr << console_out_str("请设置irc昵称") << std::endl;
		exit( 1 );
	}

	boost::asio::io_service asio;

	xmpp		xmppclient( asio, xmppuser, xmpppwd, xmppserver, xmppnick );
	webqq		qqclient( asio, qqnumber, qqpwd );
	IrcClient	ircclient( asio, ircnick, ircpwd );
	mx::mx		mx( asio, mailaddr, mailpasswd, pop3server, smtpserver );

	build_group( chanelmap, qqclient, xmppclient, ircclient, mx );

	xmppclient.on_room_message( boost::bind( &om_xmpp_message, boost::ref( xmppclient ), _1, _2, _3 ) );
	ircclient.login( boost::bind( &on_irc_message, _1, boost::ref( ircclient ), boost::ref( qqclient ) ) );

	qqclient.on_verify_code( boost::bind( on_verify_code, _1, boost::ref( qqclient ), boost::ref( ircclient ), boost::ref( xmppclient ) ) );
	qqclient.login();
	qqclient.on_group_msg( boost::bind( on_group_msg, _1, _2, _3, boost::ref( qqclient ) ) );

	mx.async_fetch_mail( boost::bind( on_mail, _1, _2, boost::ref( qqclient ) ) );

	std::vector<std::string> ircrooms;
	boost::split( ircrooms, ircroom, boost::is_any_of( "," ) );
	ircvercodechannel = ircrooms[0];
	BOOST_FOREACH( std::string room , ircrooms ) {
		ircclient.join( std::string( "#" ) + room );
	}

	std::vector<std::string> xmpprooms;
	boost::split( xmpprooms, xmpproom, boost::is_any_of( "," ) );
	BOOST_FOREACH( std::string room , xmpprooms ) {
		xmppclient.join( room );
	}

	boost::asio::io_service::work work( asio );

	if( !vm.count( "daemon" ) ) {
#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR
		boost::shared_ptr<boost::asio::posix::stream_descriptor> stdin( new boost::asio::posix::stream_descriptor( asio, 0 ) );
		boost::shared_ptr<boost::asio::streambuf> inputbuffer( new boost::asio::streambuf );
		boost::asio::async_read_until( *stdin, *inputbuffer, '\n', boost::bind( inputread , _1, _2, stdin, inputbuffer, boost::ref( qqclient ) ) );
#else
		boost::thread( boost::bind( input_thread, boost::ref( asio ), boost::ref( qqclient ) ) );
#endif
	}

	asio.run();
	return 0;
}

extern "C" void OPENSSL_add_all_algorithms_noconf(){}
