static void input_got_one_line(std::string line_input, avbot & mybot);


#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR

static void inputread(const boost::system::error_code &, std::size_t,
					  boost::shared_ptr<boost::asio::posix::stream_descriptor>,
					  boost::shared_ptr<boost::asio::streambuf>, avbot & mybot  );
#else
// workarround windows that can use posix stream for stdin
static void input_thread(boost::asio::io_service & io_service, avbot & mybot )
{
	while ( !boost::this_thread::interruption_requested() && !std::cin.eof()){
		std::string line;
		std::getline(std::cin, line);
		io_service.post(boost::bind(input_got_one_line, ansi_utf8(line), boost::ref(mybot)));
	}
}
#endif

static void input_got_one_line(std::string line_input, avbot & mybot)
{
	//验证码check
	if(qqneedvc){
		boost::trim(line_input);
		mybot.feed_login_verify_code(line_input);
		qqneedvc = false;
		return;
	}else{
		mybot.broadcast_message(
			boost::str(
				boost::format("来自 avbot 命令行的消息: %s") % line_input
			)
		);
	}	
}

#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR

static void inputread(const boost::system::error_code & ec, std::size_t length,
					  boost::shared_ptr<boost::asio::posix::stream_descriptor> stdin,
					  boost::shared_ptr<boost::asio::streambuf> inputbuffer, avbot & mybot )
{
	std::istream input(inputbuffer.get());
	std::string line;
	std::getline(input,line);

	input_got_one_line(line, mybot);

	boost::asio::async_read_until(*stdin, *inputbuffer, '\n', boost::bind(inputread , _1,_2, stdin, inputbuffer, boost::ref(mybot)));
}
#endif