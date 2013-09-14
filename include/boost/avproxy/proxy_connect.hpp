
#pragma once

#ifdef __llvm__
#pragma GCC diagnostic ignored "-Wdangling-else"
#endif

#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "detail/proxy_chain.hpp"

namespace avproxy {

// convient helper class that made for direct TCP connection without proxy
// usage:
//    avproxy::async_connect(  socket ,  ip::tcp::resolve::query( "host" ,  "port" ) ,  handler );
// the hander should be this signature
//    void handle_connect(
//         const boost::system::error_code & ec
//    )
class async_connect : boost::asio::coroutine
{
public:
	typedef void result_type; // for boost::bind
public:
	template<class Socket, class Handler, class connect_condition>
	async_connect(Socket & socket,const typename Socket::protocol_type::resolver::query & _query, BOOST_ASIO_MOVE_ARG(Handler) handler, connect_condition _connect_condition)
	{
		//BOOST_ASIO_CONNECT_HANDLER_CHECK(Handler, handler) type_check;
		_async_connect(socket, _query, boost::function<void (const boost::system::error_code&)>(handler), _connect_condition);
	}

	template<class Socket, class Handler>
	async_connect(Socket & socket,const typename Socket::protocol_type::resolver::query & _query, BOOST_ASIO_MOVE_ARG(Handler) handler)
	{
		//BOOST_ASIO_CONNECT_HANDLER_CHECK(Handler, handler) type_check;
		_async_connect(socket, _query, boost::function<void (const boost::system::error_code&)>(handler), detail::avproxy_default_connect_condition());
	}

	template<class Handler, class Socket, class connect_condition>
	void operator()(const boost::system::error_code & ec, typename Socket::protocol_type::resolver::iterator begin, Socket & socket, boost::shared_ptr<typename Socket::protocol_type::resolver> resolver, Handler handler, connect_condition _connect_condition)
	{
 		//BOOST_ASIO_CONNECT_HANDLER_CHECK(Handler, handler) type_check;

		BOOST_ASIO_CORO_REENTER(this)
		{
			BOOST_ASIO_CORO_YIELD boost::asio::async_connect(socket,begin, _connect_condition, boost::bind(*this, _1, _2, boost::ref(socket), resolver, handler, _connect_condition));
			socket.get_io_service().post(boost::asio::detail::bind_handler(handler,ec));
		}
	}
private:
	template<class Socket, class Handler, class connect_condition>
	void _async_connect(Socket & socket,const typename Socket::protocol_type::resolver::query & _query, BOOST_ASIO_MOVE_ARG(Handler) handler, connect_condition _connect_condition)
	{
		//BOOST_ASIO_CONNECT_HANDLER_CHECK(Handler, handler) type_check;

		typedef typename Socket::protocol_type::resolver resolver_type;

		boost::shared_ptr<resolver_type>
			resolver(new resolver_type(socket.get_io_service()));
		resolver->async_resolve(_query, boost::bind(*this, _1, _2, boost::ref(socket), resolver, handler, _connect_condition));
	}
};

// 带　proxy 执行连接.
class async_proxy_connect : boost::asio::coroutine{
public:
	typedef void result_type; // for boost::bind_handler
public:
	template<class Handler>
	async_proxy_connect(const proxy_chain &proxy_chain, BOOST_ASIO_MOVE_ARG(Handler) handler)
		:proxy_chain_(proxy_chain)
	{
		boost::function<void (const boost::system::error_code&)> _handler(handler);
		proxy_chain_.get_io_service().post(boost::bind(*this,boost::system::error_code(), _handler));
	}

	template <class Handler>
	void operator()(const boost::system::error_code & ec, Handler  handler)//, typename Socket::protocol_type::resolver::iterator begin, Socket & socket, boost::shared_ptr<typename Socket::protocol_type::resolver> resolver)
	{
		BOOST_ASIO_CORO_REENTER(this)
		{
			// resolve
			BOOST_ASIO_CORO_YIELD proxy_chain_.back()->resolve(boost::bind(*this,_1, handler), proxy_chain_.clone_poped());
			if(! ec)
				BOOST_ASIO_CORO_YIELD proxy_chain_.back()->handshake(boost::bind(*this,_1, handler), proxy_chain_.clone_poped());
			proxy_chain_.get_io_service().post(boost::asio::detail::bind_handler(handler,ec));
		}
	}
private:
	proxy_chain proxy_chain_;
};

}
