#pragma once

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/coro/coro.hpp>
#include <boost/coro/yield.hpp>

#include <boost/avloop.hpp>

namespace boost{
namespace detail{

template<class DirWalkHandler, class CompleteHandler>
class async_dir_walk
{
	boost::asio::io_service &io_service;
	boost::filesystem::directory_iterator dir_it_end;
	boost::filesystem::directory_iterator dir_it_cur;
	DirWalkHandler dir_walk_handler;
	CompleteHandler complete_handler;
public:
	typedef void result_type;

	async_dir_walk( boost::asio::io_service & _io_service, boost::filesystem::path path, DirWalkHandler _dir_walk_handler, CompleteHandler _complete_handler)
		: io_service( _io_service ), dir_it_cur(path), dir_walk_handler(_dir_walk_handler), complete_handler(_complete_handler)
	{
		io_service.post( boost::asio::detail::bind_handler( *this, boost::coro::coroutine() ) );
	}

	void operator()( boost::coro::coroutine coro )
	{
		// 好了，每次回调检查一个文件，这样才好，对吧.
		reenter( &coro )
		{
			for( ; dir_it_cur != dir_it_end ; dir_it_cur++ )
			{
				// 好，处理 dir_it_cur dir_it_;
				_yield dir_walk_handler(dir_it_cur->path(), io_service.wrap(boost::bind( *this, coro )) );

				_yield avloop_idle_post(io_service, boost::bind( *this, coro ) );
			}

			avloop_idle_post(io_service, complete_handler);
		}
	}

};

class DirWalkDumyHandler{
public:
	void operator()(){}
};

template<class DirWalkHandler, class CompleteHandler>
async_dir_walk<DirWalkHandler, CompleteHandler> make_async_dir_walk_op( boost::asio::io_service & io_service, boost::filesystem::path path, DirWalkHandler dir_walk_handler, CompleteHandler complete_handler)
{
	return async_dir_walk<DirWalkHandler, CompleteHandler>( io_service, path, dir_walk_handler, complete_handler);
}


} // namespace detail

template<class DirWalkHandler>
void async_dir_walk(boost::asio::io_service & io_service, boost::filesystem::path path, DirWalkHandler dir_walk_handler)
{
	detail::make_async_dir_walk_op(io_service, path,  dir_walk_handler, detail::DirWalkDumyHandler());
}


template<class DirWalkHandler, class CompleteHandler>
void async_dir_walk(boost::asio::io_service & io_service, boost::filesystem::path path, DirWalkHandler dir_walk_handler, CompleteHandler complete_handler)
{
	detail::make_async_dir_walk_op(io_service, path,  dir_walk_handler, complete_handler);
}

}