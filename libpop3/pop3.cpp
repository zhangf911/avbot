#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include "pop3.hpp"

static fs::path configfilepath()
{
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

int main(int argc, char * argv[])
{
    std::string qqnumber, qqpwd;
    std::string ircnick, ircroom, ircpwd;
    std::string xmppuser, xmppserver, xmpppwd, xmpproom;
    std::string cfgfile;
	std::string logdir;
	std::string chanelmap;

    setlocale(LC_ALL, "");

	po::options_description desc("qqbot options");
	desc.add_options()
	    ( "version,v",										"output version" )
		( "help,h",											"produce help message" )
		( "daemon,d",										"go to background" )
		( "qqnum,u",	po::value<std::string>(&qqnumber),	"QQ number" )
		( "qqpwd,p",	po::value<std::string>(&qqpwd),		"QQ password" )
		( "logdir",		po::value<std::string>(&logdir),	"dir for logfile" )
		( "ircnick",	po::value<std::string>(&ircnick),	"irc nick" )
		( "ircpwd",		po::value<std::string>(&ircpwd),	"irc password" )
		( "ircrooms",	po::value<std::string>(&ircroom),	"irc room" )
		( "xmppuser",	po::value<std::string>(&xmppuser),	"id for XMPP,  eg: (microcaicai@gmail.com)" )
		( "xmppserver",	po::value<std::string>(&xmppserver),	"server to connect for XMPP,  eg: (xmpp.l.google.com)" )
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

	
	boost::asio::io_service asio;
	boost::asio::io_service::work work(asio);

	pop3 p(asio, qqnumber, qqpwd);
    asio.run();
}