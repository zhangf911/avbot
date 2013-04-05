
#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

namespace boost {

template<class timeunit>
class base_delayedcall{
public:
	typedef void result_type;
	template<class handler>
	base_delayedcall(boost::asio::io_service &_io_service, int timeunitcount, handler _cb)
		:io_service(_io_service),timer( new boost::asio::deadline_timer(io_service))
	{
	    BOOST_ASIO_WAIT_HANDLER_CHECK(handler, _cb) type_check;
	    timer->expires_from_now(timeunit(timeunitcount));
 		timer->async_wait(boost::bind(*this, boost::function<void()>(_cb), _1));
	}
	template<class handler>
	void operator()(handler _cb, const boost::system::error_code& error)
	{
		io_service.post(_cb);
	}
private:
	boost::asio::io_service &io_service;
	boost::shared_ptr<boost::asio::deadline_timer> timer;
};

typedef base_delayedcall<boost::posix_time::seconds> delayedcallsec;
typedef base_delayedcall<boost::posix_time::milliseconds> delayedcallms;

}