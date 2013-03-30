static void input_got_one_line(std::string line_input, webqq & qqclient);


#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR

static void inputread(const boost::system::error_code &, std::size_t,
					  boost::shared_ptr<boost::asio::posix::stream_descriptor>,
					  boost::shared_ptr<boost::asio::streambuf>, webqq &  );
#else
// workarround windows that can use posix stream for stdin
static void input_thread(boost::asio::io_service & io_service, webqq & qqclient)
{
	while ( !boost::this_thread::interruption_requested() && !std::cin.eof()){
		std::string line;
		std::getline(std::cin, line);
		io_service.post(boost::asio::detail::bind_handler(input_got_one_line, line, boost::ref(qqclient)));
	}
}
#endif

static void input_got_one_line(std::string line_input, webqq & qqclient)
{
	//验证码check
	if(qqneedvc){
		boost::trim(line_input);
		qqclient.login_withvc(line_input);
		qqneedvc = false;
		return;
	}else{
		BOOST_FOREACH(messagegroup & g ,  messagegroups)
		{
			g.broadcast(boost::str(
				boost::format("来自 avbot 命令行的消息: %s") % line_input
			));
		}
	}	
}

#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR

static void inputread(const boost::system::error_code & ec, std::size_t length,
					  boost::shared_ptr<boost::asio::posix::stream_descriptor> stdin,
					  boost::shared_ptr<boost::asio::streambuf> inputbuffer, webqq & qqclient )
{
	std::istream input(inputbuffer.get());
	std::string line;
	std::getline(input,line);

	input_got_one_line(line, qqclient);

	boost::asio::async_read_until(*stdin, *inputbuffer, '\n', boost::bind(inputread , _1,_2, stdin, inputbuffer, boost::ref(qqclient)));
}
#endif