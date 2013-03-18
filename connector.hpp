
#pragma once

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#ifdef BOOST_ENABLE_SSL
#include <boost/asio/ssl.hpp>
#endif

#include "boost/coro/yield.hpp"

namespace boost {

namespace detail{

struct my_connect_condition{
	template <typename Iterator>
	Iterator operator()(
		const boost::system::error_code& ec,
		Iterator next)
	{
// 		if (ec) std::cout << "Error: " << ec.message() << std::endl;
// 		std::cout << "Trying: " << next->endpoint() << std::endl;
		return next;
	}
};

}
	
class async_connect : coro::coroutine
{
public:
	typedef void result_type; // for boost::bind
public:
	template<class Socket, class Handler>
	async_connect(Socket & socket,const typename Socket::protocol_type::resolver::query & _query, BOOST_ASIO_MOVE_ARG(Handler) _handler)
		:handler(_handler),io_service(socket.get_io_service())
	{
		BOOST_ASIO_CONNECT_HANDLER_CHECK(Handler, handler) type_check;
		
		typedef typename Socket::protocol_type::resolver resolver_type;

		boost::shared_ptr<resolver_type>
			resolver(new resolver_type(io_service));
		resolver->async_resolve(_query, boost::bind(*this, _1, _2, boost::ref(socket), resolver));
	}

	template<class Socket>
	void operator()(const boost::system::error_code & ec, typename Socket::protocol_type::resolver::iterator begin, Socket & socket, boost::shared_ptr<typename Socket::protocol_type::resolver> resolver)
	{
		reenter(this)
		{
			_yield boost::asio::async_connect(socket,begin,detail::my_connect_condition(),boost::bind(*this, _1, _2, boost::ref(socket), resolver));
			io_service.post(asio::detail::bind_handler(handler,ec));
		}
	}

private:
	boost::asio::io_service&			io_service;
	boost::function<void(const boost::system::error_code & ec)>	handler;
};

class proxychain;

namespace detail {
	
	struct proxychain_adder{
		proxychain_adder( proxychain & _chain)
			:chain(&_chain)
		{
		}

		template<class Proxy>
		proxychain_adder operator()(const Proxy & proxy );

		operator proxychain& (){
			return *chain;
		}
		operator proxychain ();

	private:
		proxychain * chain;
	};

}

namespace detail {
	class proxy_base;
}

class proxychain {
public:
	proxychain(boost::asio::io_service& _io):io_(_io){}

	detail::proxy_base * front(){
		return m_chain.front().get();
	}
	
	// 克隆一个自己，然后弹出第一个元素.
	// 也就是克隆一个少一个元素的 proxychain
	proxychain clone_poped(){
		proxychain p(*this);
		p.m_chain.erase(p.m_chain.begin());
		return p;
	}

	detail::proxychain_adder add_proxy(){
		return detail::proxychain_adder(*this);
	}

	template<class Proxy>
	proxychain  add_proxy(const Proxy & proxy)
	{
		// 拷贝构造一个新的.
		detail::proxy_base * newproxy = new Proxy(proxy);
		add_proxy_base(newproxy);
		return *this;
	}

	boost::asio::io_service& get_io_service(){
		return io_;
	}
private:
	void add_proxy_base(detail::proxy_base * proxy){
		m_chain.push_back(boost::shared_ptr<detail::proxy_base>(proxy) );
	}
private:
	boost::asio::io_service&	io_;
	std::vector<boost::shared_ptr<detail::proxy_base> > m_chain;

	friend class detail::proxychain_adder;
};

namespace detail{
	class proxy_base {
	public:
		typedef boost::function< void (const boost::system::error_code&) > handler_type;
	public:
		template<class Handler>
		void resolve(Handler handler, proxychain subchain){
			_resolve(handler_type(handler), subchain);
		}
		
		template<class Handler>
		void handshake(Handler handler,proxychain subchain){
			_handshake(handler_type(handler), subchain);
		}

		virtual ~proxy_base(){}

	private:
		virtual void _resolve(handler_type, proxychain subchain) = 0;
		virtual void _handshake(handler_type, proxychain subchain) = 0;
	};
}

template<class Proxy>
detail::proxychain_adder detail::proxychain_adder::operator()( const Proxy & proxy )
{
	// 拷贝构造一个新的.
	proxy_base * newproxy = new Proxy(proxy);
	chain->add_proxy_base(newproxy);
	return *this;
}

inline
detail::proxychain_adder::operator proxychain()
{
	return *chain;
}

// 带　proxy 执行连接.
class async_avconnect :coro::coroutine{
public:
	typedef void result_type; // for boost::bind_handler
public:
	template<class Handler>
	async_avconnect(const proxychain &proxy_chain, BOOST_ASIO_MOVE_ARG(Handler) _handler)
		:proxy_chain_(new proxychain(proxy_chain)), handler(_handler)
	{
		proxy_chain_->get_io_service().post(boost::bind(*this,boost::system::error_code()));
	}

	void operator()(const boost::system::error_code & ec)//, typename Socket::protocol_type::resolver::iterator begin, Socket & socket, boost::shared_ptr<typename Socket::protocol_type::resolver> resolver)
	{
		reenter(this)
		{
			// resolve
			_yield proxy_chain_->front()->resolve(*this, proxy_chain_->clone_poped());
			_yield proxy_chain_->front()->handshake(*this, proxy_chain_->clone_poped());
			proxy_chain_->get_io_service().post(asio::detail::bind_handler(handler,ec));
		}
	}
private:
	boost::shared_ptr<proxychain>								proxy_chain_;
	boost::function<void(const boost::system::error_code & ec)>	handler;
};

// 使用　TCP 发起连接. 也就是不使用代理啦.
class proxy_tcp : public detail::proxy_base {
public:
	typedef boost::asio::ip::tcp::resolver::query query;
	typedef boost::asio::ip::tcp::socket socket;

	proxy_tcp(socket &_socket,const query & _query)
		:socket_(_socket), query_(_query)
	{
	}
private:
	void _resolve(handler_type handler, proxychain subchain){
		//handler(boost::system::error_code());
		socket_.get_io_service().post(boost::asio::detail::bind_handler(handler, boost::system::error_code()));
	}

	void _handshake(handler_type handler, proxychain subchain ){
		boost::async_connect(socket_, query_, handler);
	}

	socket&		socket_;
	const query	query_;
};

// 使用　socks5代理 发起连接.

// class proxy_socks5 : public proxy_base{
// public:
// 	void resolve();
// 	void handshake();
// };
//

#ifdef BOOST_ENABLE_SSL

// SSL 连接过程. 支持透过代理发起　SSL 连接哦!
class proxy_ssl : public detail::proxy_base{
public:
	typedef boost::asio::ip::tcp tcp;
	typedef tcp::resolver::query query;
	typedef tcp::socket socket;

	proxy_ssl(boost::asio::ssl::stream<tcp> &stream, query _query)
	{
		
	}

private:
	void _resolve(handler_type handler, proxychain subchain){
		// 递归调用　avconnect 传递到下一层.
		boost::async_avconnect(subchain, handler);
	}

	void _handshake(handler_type handler, proxychain subchain ){
		// 调用　ssl 的　async_handshake
	}

};

#endif

}
