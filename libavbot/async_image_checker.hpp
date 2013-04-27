#pragma once

#include <fstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "boost/hash/md5.hpp"
#include "boost/hash/compute_digest.hpp"
#include "boost/coro/coro.hpp"
#include "boost/coro/yield.hpp"
#include "boost/timedcall.hpp"

#include "libwebqq/webqq.h"

namespace detail
{

// 比较文件名和 md5 数值是否一致.
template<typename SequenceT>
bool compare_image_degest_and_filename( SequenceT filename, const SequenceT & md5 )
{
	boost::replace_all( filename, "{", "" );
	boost::replace_all( filename, "}", "" );
	boost::replace_all( filename, "-", "" );
	boost::to_lower( filename );
	return filename == md5;
}

template<class Handler>
class async_image_check_and_download_op
{
	Handler handler;
	boost::asio::io_service &io_service;

public:
	typedef void result_type;

	async_image_check_and_download_op( boost::asio::io_service & _io_service, boost::filesystem::path item, Handler _handler )
		: handler( _handler ), io_service( _io_service )
	{
		using namespace boost::filesystem;
		using namespace boost::asio::detail;
		using namespace boost::system::errc;

		if( !exists( item ) )
		{
			// 这事实上应该算是严重的错误了 :)
			io_service.post( bind_handler( handler, make_error_code(no_such_file_or_directory)));
		}
		else
		{
			std::vector<char>  content( boost::filesystem::file_size( item ) );

			// 读取文件，然后执行 MD5 校验.
			std::ifstream imgfile( item.c_str(), std::ofstream::binary | std::ofstream::in );
			imgfile.read( &content[0], content.capacity() );

			boost::hashes::md5::digest_type	md5 = boost::hashes::compute_digest<boost::hashes::md5>( content );

			std::string imgfilename  = boost::filesystem::basename( item.filename() );
			std::string imgfilemd5 =  md5.str();

			if( compare_image_degest_and_filename( imgfilename, imgfilemd5 ) )
			{
				// do nothing
				io_service.post( bind_handler( handler, boost::system::error_code() ) );

			}
			else
			{
				// 重新下载图片.

				std::string cface = basename( item ) + extension( item );
				// 重新执行图片下载.
				webqq::async_fetch_cface( io_service, cface, boost::bind( *this, _1, _2, cface ) );
			}
		}
	}

	void operator()( boost::system::error_code ec, boost::asio::streambuf & buf, std::string cface )
	{
 		using namespace boost::asio::detail;

		if( !ec || ec == boost::asio::error::eof )
		{
			std::ofstream cfaceimg( ( std::string( "images/" ) + cface ).c_str(), std::ofstream::binary | std::ofstream::out );
			cfaceimg.write( boost::asio::buffer_cast<const char*>( buf.data() ), boost::asio::buffer_size( buf.data() ) );
			ec = boost::system::error_code();
		}

		io_service.post( bind_handler( handler, ec));
	}

};

/**
 * 给定一个 path, 检查文件是否存在，如果不存在就从 TX 的服务器重新下载. 主要依据文件名啦.
 */
template<class Handler>
void async_image_check_and_download( boost::asio::io_service & io_service, const boost::filesystem::path & item, const Handler & handler )
{
	async_image_check_and_download_op<Handler> op( io_service, item, handler );
}

class async_image_checker_op
{
	boost::asio::io_service &io_service;
	boost::filesystem::directory_iterator dir_it_end;
	boost::filesystem::directory_iterator dir_it_cur;
public:
	typedef void result_type;

	async_image_checker_op( boost::asio::io_service & _io_service )
		: io_service( _io_service ), dir_it_cur( boost::filesystem::path( "images" ) )
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
				_yield async_image_check_and_download( io_service, dir_it_cur->path(), boost::bind( *this, coro ) );

				_yield boost::delayedcallms( io_service, 1, boost::bind( *this, coro ) );
			}
		}
	}

};

async_image_checker_op make_image_checker_op( boost::asio::io_service & io_service )
{
	return async_image_checker_op( io_service );
}

}

void async_image_checker( boost::asio::io_service & __io_service )
{
	detail::make_image_checker_op( __io_service );
}

#include "boost/coro/unyield.hpp"
