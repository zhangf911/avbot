
#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>

namespace boost {

template<class timeunit>
class base_delayedcall{
public:
	template<typename handler>
	base_delayedcall(boost::asio::io_service &_io_service, int timeunitcount, BOOST_ASIO_MOVE_ARG(handler) _cb)
		:io_service(_io_service),timer( new boost::asio::deadline_timer(io_service, timeunit(timeunitcount)))
	{
	    BOOST_ASIO_WAIT_HANDLER_CHECK(handler, _cb) type_check;
		cb = _cb;
		timer->async_wait(*this);
	}
	void operator()(const boost::system::error_code& error)
	{
		cb();
	}
private:
	boost::asio::io_service &io_service;
	boost::shared_ptr<boost::asio::deadline_timer> timer;
	boost::function< void () > cb;
};

typedef base_delayedcall<boost::posix_time::seconds> delayedcallsec;
typedef base_delayedcall<boost::posix_time::milliseconds> delayedcallms;

}