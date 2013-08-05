
#pragma once

#include <queue>
#include <boost/config.hpp>
#include <boost/asio.hpp>
#include <boost/function.hpp>
namespace boost{

/*
 * async_coro_queue 是一个用于协程的异步列队。
 *
 * ListType 只要任意支持 pop_front/push_back 的容器就可以。
 * 如 std::deque / std::list /boost::circular_buffer
 *
 * NOTE: 使用线程安全的列队做模板参数就可以让本协程列队用于多个线程跑的 io_service.
 */

template<typename ListType>
class async_coro_queue{
public: // typetraits
	typedef typename ListType::value_type value_type;
	typedef typename ListType::size_type size_type;
	typedef typename ListType::reference reference;
	typedef typename ListType::const_reference const_reference;
private:
	// async_pop 的回调原型
	typedef	boost::function<void(value_type)> handler_type;

public:
	// 构造函数
	async_coro_queue(boost::asio::io_service & io_service)
	  :m_io_service(io_service)
	{
	}

	// 构造函数的一个重载，为列队传入额外的参数
	template<typename T>
	async_coro_queue(boost::asio::io_service & io_service, T t)
	  :m_io_service(io_service), m_list(t)
	{
	}
	// 利用 C++11 的 泛模板参数写第三个构造函数重载
#ifndef  BOOST_NO_CXX11_VARIADIC_TEMPLATES
	template<typename... T>
	async_coro_queue(boost::asio::io_service & io_service, T... t)
	  : m_io_service(io_service), m_list(t...)
	{
	}
#endif
// 公开的接口。
public:

	/*
	 *  回调的类型是 void pop_handler(value_type)
	 *
	 *  value_type 由容器（作为模板参数）决定。
	 *  例子是

		void pop_handler(value_type value)
		{
			// DO SOME THING WITH value

			// start again
			list.async_pop(pop_handler);
		}

		// 然后在其他地方
		list.push(value); 即可唤醒 pop_handler

	 *  NOTE: 如果列队里有数据， 回调将投递(不是立即回调，是立即投递到 io_service), 否则直到有数据才回调.
     */
	template<class Handler>
	void async_pop(Handler handler)
	{
		if (m_list.empty())
		{
			// 进入睡眠过程.
			m_handlers.push(handler_type(handler));
		}
		else
		{
			m_io_service.post(
				boost::asio::detail::bind_handler(handler, m_list.front())
			);
 			m_list.pop_front();
		}
	}

	/**
	 * 向列队投递数据。
	 * 如果列队为空，并且有协程正在休眠在 async_pop 上， 则立即唤醒此协程，并投递数据给此协程
     */
	void push(value_type v)
	{
		// 有handler 挂着！
		if (!m_handlers.empty())
		{
			// 如果 m_list 不是空， 肯定是有严重的 bug
			BOOST_ASSERT(m_list.empty());

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
