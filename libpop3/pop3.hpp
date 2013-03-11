

#include <boost/asio.hpp>
#include "boost/coro/coro.hpp"
#include "boost/coro/yield.hpp"

class pop3{
public:
	pop3(boost::asio::io_service & _io_service);

	// connect to pop.qq.com
	void operator()(const boost::system::error_code & errorcode, std::size_t length = 0)
	{

	}
private:
	boost::asio::io_service & io_service;
};

#include  "boost/coro/unyield.hpp"
