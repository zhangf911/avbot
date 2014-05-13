
#include <boost/asio.hpp>
#ifdef HAVE_ZLIB
#include <zlib.h>
#else
#include "qqwry/miniz.c"
#endif
#include "libavbot/avbot.hpp"
#include "extension.hpp"

#include "libavlog/avlog.hpp"

#ifdef ENABLE_LUA
#	include "luascript/luascript.hpp"
#endif

#include "urlpreview.hpp"
#include "joke.hpp"
#include "bulletin.hpp"
#include "metalprice.hpp"
#include "stockprice.hpp"
#include "exchangerate.hpp"
#include "iplocation.hpp"

// dummy file

extern avlog logfile;			// 用于记录日志文件.

static void sender(avbot & mybot,std::string channel_name, std::string txt, bool logtohtml)
{
	mybot.broadcast_message(channel_name, txt);
	if (logtohtml){
		logfile.add_log(channel_name, avlog::html_escape(txt), 0);
	}
}

void new_channel_set_extension(boost::asio::io_service &io_service, avbot & mybot , std::string channel_name)
{
	mybot.on_message.connect(
		botextension(
			channel_name,
			joke(
				io_service,
				io_service.wrap(boost::bind(sender, boost::ref(mybot), channel_name, _1, 0)),
				channel_name, boost::posix_time::seconds(600)
			)
		)
	);

	mybot.on_message.connect(
		botextension(
			channel_name,
			urlpreview(io_service,
				io_service.wrap(boost::bind(sender, boost::ref(mybot), channel_name, _1, 1)),
				channel_name
			)
		)
	);
#ifdef ENABLE_LUA
	mybot.on_message.connect(
		botextension(
			channel_name,
			callluascript(io_service,
				io_service.wrap(boost::bind(sender, boost::ref(mybot), channel_name, _1, 1)),
				channel_name
			)
		)
	);
#endif
	mybot.on_message.connect (
		botextension(
			channel_name,
			::bulletin(
				io_service,
				io_service.wrap(boost::bind(sender, boost::ref(mybot), channel_name, _1, 1)),
				channel_name
			)
		)
	);
	mybot.on_message.connect (
		botextension(
			channel_name,
			::metalprice(
				io_service,
				io_service.wrap(boost::bind(sender, boost::ref(mybot), channel_name, _1, 1))
			)
		)
	);
	mybot.on_message.connect (
		botextension(
			channel_name,
			::stockprice(
				io_service,
				io_service.wrap(boost::bind(sender, boost::ref(mybot), channel_name, _1, 1))
			)
		)
	);
	mybot.on_message.connect (
		botextension(
			channel_name,
			::exchangerate(
				io_service,
				io_service.wrap(boost::bind(sender, boost::ref(mybot), channel_name, _1, 1)),
				channel_name
			)
		)
	);

	static boost::shared_ptr<iplocation::ipdb_mgr> ipdb_mgr;

	if (!ipdb_mgr)
	{
		// check for file "qqwry.dat"
		// if not exist, then download that file
		// after download that file, construct ipdb
		ipdb_mgr.reset(new  iplocation::ipdb_mgr(mybot.get_io_service(), uncompress));
		ipdb_mgr->search_and_build_db();
	}

	mybot.on_message.connect(
		botextension(
			channel_name,
			iplocation(
				io_service,
				io_service.wrap(boost::bind(sender, boost::ref(mybot), channel_name, _1, 0)),
				ipdb_mgr
			)
		)
	);
}
