

#include <boost/asio.hpp>

class pop3{
public:
	pop3(boost::asio::io_service & _io_service);

	operator()(const boost::system::error_code & errorcode, std::size_t length = 0)
	{

	}
private:
	boost::asio::io_service & io_service;
};