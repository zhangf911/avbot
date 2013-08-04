
#pragma once

#include <queue>
#include <boost/asio.hpp>
#include <boost/function.hpp>
namespace boost{

/*
 * async_coro_queue 是一个用于协程的异步列队。
 *
 * ListType 只要任意支持 pop_front/push_back 的容器就可以。
 * 如 std::deque / std::list /boost::circular_buffer
 */
template<typename ListType>
class async_coro_queue{
public: // typetraits
	typedef typename ListType::value_type value_type;
	typedef typename ListType::size_type size_type;
	typedef typename ListType::reference reference;
	typedef typename ListType::const_reference const_reference;
private:
	typedef	boost::function<void(value_type)> handler_type;

public:
	async_coro_queue(boost::asio::io_service & io_service)
	  :m_io_service(io_service)
	{
	}

	// 为列队传入额外的参数用于构造函数.
	template<typename T>
	async_coro_queue(boost::asio::io_service & io_service, T t)
	  :m_io_service(io_service), m_list(t)
	{
	}

	/*
	 * 如果列队里有数据， 回调将投递(不是立即回调，是立即投递到 io_service), 否则直到有数据才回调.
     */
	template<class Handler>
	void async_pop(Handler handler)
	{
		if (m_list.empty()){
			// 进入睡眠过程.
			m_handlers.push(handler_type(handler));
		}else{
			m_io_service.post(
				boost::asio::detail::bind_handler(handler, m_list.front())
			);
 			m_list.pop_front();
		}
	}

	void push(value_type v)
	{
		// 有handler 挂着！
		if (!m_handlers.empty())
		{
			// 如果 m_list 不是空， 肯定是有严重的 bug
			BOOST_ASSERT(  m_list.empty() );

			m_io_service.post(
				boost::asio::detail::bind_handler(m_handlers.front(), v)
			);
			m_handlers.pop();
		}
		else
		{
			m_list.push_back(v);
		}
	}
private:

	boost::asio::io_service & m_io_service;
	ListType m_list;
	std::queue<handler_type> m_handlers;
};

}
