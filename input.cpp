
#include <boost/signals2.hpp>
#include <boost/asio.hpp>
#ifndef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR
#include <boost/thread.hpp>
#endif

#include "input.hpp"

static boost::signals2::signal< void(std::string)> input_got_one_line;

#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR

static void inputread(const boost::system::error_code &, std::size_t,
					  boost::shared_ptr<boost::asio::posix::stream_descriptor>,
					  boost::shared_ptr<boost::asio::streambuf>  );
#else
// workarround windows that can use posix stream for stdin
static void input_thread(boost::asio::io_service & io_service)
{
	while ( !boost::this_thread::interruption_requested() && !std::cin.eof()){
		std::string line;
		std::getline(std::cin, line);
		io_service.post(boost::bind(input_got_one_line, ansi_utf8(line)));
	}
}
#endif

// static void input_got_one_line(std::string line_input)
// {
// 	//验证码check
// 	if(qqneedvc){
// 		boost::trim(line_input);
// 		mybot.feed_login_verify_code(line_input);
// 		qqneedvc = false;
// 		return;
// 	}else{
// 		boost::trim_right(line_input);
// 		if(line_input.size() > 1){
// 			mybot.broadcast_message(
// 				boost::str(
// 					boost::format("来自 avbot 命令行的消息: %s") % line_input
// 				)
// 			);
// 		}
// 	}
// }

#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR

static void inputread(const boost::system::error_code & ec, std::size_t length,
					  boost::shared_ptr<boost::asio::posix::stream_descriptor> stdin,
					  boost::shared_ptr<boost::asio::streambuf> inputbuffer)
{
	std::istream input(inputbuffer.get());
	std::string line;
	std::getline(input,line);

	input_got_one_line(line);

	boost::asio::async_read_until(*stdin, *inputbuffer, '\n', boost::bind(inputread , _1,_2, stdin, inputbuffer));
}
#endif

void start_stdinput(boost::asio::io_service & io_service)
{
#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR
		boost::shared_ptr<boost::asio::posix::stream_descriptor> stdin( new boost::asio::posix::stream_descriptor( io_service, 0 ) );
		boost::shared_ptr<boost::asio::streambuf> inputbuffer( new boost::asio::streambuf );
		boost::asio::async_read_until( *stdin, *inputbuffer, '\n', boost::bind( inputread , _1, _2, stdin, inputbuffer ) );
#else
		boost::thread( boost::bind( input_thread, boost::ref( io_service ) ) );
#endif
}

void connect_stdinput(boost::function<void(std::string)> cb)
{
	input_got_one_line.connect(cb);
}
