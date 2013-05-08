#pragma once

#include <fstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "boost/async_dir_walk.hpp"
#include "boost/hash/md5.hpp"
#include "boost/hash/compute_digest.hpp"
#include "boost/coro/coro.hpp"
#include "boost/coro/yield.hpp"
#include "boost/timedcall.hpp"
#include "boost/avloop.hpp"

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

class async_image_check_and_download_op
{
	boost::asio::io_service &io_service;

public:
	typedef void result_type;
	async_image_check_and_download_op( boost::asio::io_service & _io_service)
		: io_service( _io_service )
	{
	}

	// boost::async_dir_walk 回调.
	void operator()(const boost::filesystem::path & item, boost::function<void(const boost::system::error_code&)> handler)
	{
		using namespace boost::filesystem;
		using namespace boost::asio::detail;
		using namespace boost::system::errc;

		if( !exists( item ) )
		{
			// 这事实上应该算是严重的错误了 :)
			io_service.post( bind_handler( handler, make_error_code(no_such_file_or_directory)));
		}
		else if (boost::filesystem::is_directory(item)){
			// 递归啊递归.
			boost::async_dir_walk(io_service, item,
				async_image_check_and_download_op(io_service),
				handler
			);
		}
		else
		{
			std::vector<char>  content( boost::filesystem::file_size( item ) );

			// 读取文件，然后执行 MD5 校验.
			std::ifstream imgfile( item.string().c_str(), std::ifstream::openmode(std::ifstream::binary | std::ifstream::in) );
			imgfile.read( &content[0], content.capacity() );

			boost::hashes::md5::digest_type	md5 = boost::hashes::compute_digest<boost::hashes::md5>( content );

			std::string imgfilename  = boost::filesystem::basename( item.filename() );
			std::string imgfilemd5 =  md5.str();

			if( compare_image_degest_and_filename( imgfilename, imgfilemd5 ) )
			{
				// do nothing
				avloop_idle_post(io_service, bind_handler( handler, boost::system::error_code() ) );
			}
			else
			{
				// 重新下载图片.

				std::string cface = basename( item ) + extension( item );
				// 重新执行图片下载.
				webqq::async_fetch_cface( io_service, cface, boost::bind( *this, _1, _2, cface, handler) );
			}
		}
	}

	// 那个webqq::async_fetch_cface回调.
	template<class Handler>
	void operator()( boost::system::error_code ec, boost::asio::streambuf & buf, std::string cface , Handler handler)
	{
 		using namespace boost::asio::detail;

		if( !ec || ec == boost::asio::error::eof )
		{
			std::ofstream cfaceimg( ( std::string( "images/" ) + cface ).c_str(), std::ofstream::openmode(std::ofstream::binary | std::ofstream::out) );
			cfaceimg.write( boost::asio::buffer_cast<const char*>( buf.data() ), boost::asio::buffer_size( buf.data() ) );
			ec = boost::system::error_code();
		}

		io_service.post( bind_handler( handler, ec));
	}

};


}

/**
 * 给定一个 path, 检查文件是否存在，如果不存在就从 TX 的服务器重新下载. 主要依据文件名啦.
 */
void async_image_checker( boost::asio::io_service & io_service )
{
	// 防止 images 文件不存在的时候退出.
	try{
		boost::async_dir_walk(io_service, boost::filesystem::path("images"),
				detail::async_image_check_and_download_op(io_service)
		);
	}catch (...){}
}

#include "boost/coro/unyield.hpp"
