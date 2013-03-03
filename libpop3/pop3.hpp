

#include <boost/asio.hpp>

class pop3{
public:
	pop3(boost::asio::io_service & _io_service);
	operator()(const boost::system::error_code & errorcode);
private:
	boost::asio::io_service & io_service;
};