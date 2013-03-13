
#pragma once

#include <boost/function.hpp>
#include <boost/asio.hpp>
#include "boost/coro/coro.hpp"
#include "boost/coro/yield.hpp"

namespace boost {

template<typename Protocol>
class resolver
{
public:
	typedef boost::asio::ip::basic_resolver<Protocol> resolver_type;
	typedef typename boost::asio::ip::basic_resolver<Protocol>::query query;
	typedef typename boost::asio::ip::basic_resolver<Protocol>::iterator iterator;
	typedef typename Protocol::endpoint endpoint_type;
public:
	template<class Handler>
	resolver(boost::asio::io_service & _io_service, const query & _query, std::vector<endpoint_type>& _hosts, Handler _handler )
		:io_service(_io_service), m_resolver( new resolver_type(io_service) ), hosts(_hosts), handler(_handler)
	{
		m_resolver->async_resolve(_query, * this);
	}

	void operator()(const boost::system::error_code & ec, iterator endpoint)
	{
		for(iterator end;endpoint != end;endpoint++)
		{
			hosts.push_back(*endpoint);
		}
		io_service.post(boost::asio::detail::bind_handler(handler, ec));
	}
private:
	boost::asio::io_service&			io_service;
	boost::shared_ptr<resolver_type>	m_resolver;
	std::vector<endpoint_type>&			hosts;
	boost::function<void(const boost::system::error_code & ec)>	handler;	
};

}