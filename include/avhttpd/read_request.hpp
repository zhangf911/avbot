
#pragma once

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "settings.hpp"
#include "read_until.hpp"

namespace avhttpd {
namespace detail {

template<class Stream, class ConstBufferSequence, class Handler>
class async_read_request_op
{
public:
	async_read_request_op(Stream & stream, request_opts & opts,
						  const ConstBufferSequence & buffer, Handler handler)
		: m_stream(stream)
		, m_handler(handler)
		, m_opts(opts)
		, m_strembuf(boost::make_shared<boost::asio::streambuf>())
	{
		boost::asio::detail::consuming_buffers<
			boost::asio::const_buffer, ConstBufferSequence
		> buffers_(buffer);

		std::size_t size = buffers_.begin() - buffers_.end();
		// 首先处理 m_buffer 里面的东西
		boost::asio::buffer_copy(m_strembuf->prepare(size), buffers_);
		// buffers 里的东西就跑到咱的 m_strembuf 了

		avhttpd::async_read_until(m_stream, *m_strembuf, "\r\n\r\n", *this);
	};

	void operator()(boost::system::error_code ec, std::size_t bytes_transferred)
	{
		// 完成 header 的读取，用 boost::split 以行为分割.

		// TODO
	}

private:
	// 传入的变量.
	Stream &m_stream;
	Handler m_handler;
	request_opts & m_opts;

	// 这里是协程用到的变量.
	boost::shared_ptr<boost::asio::streambuf> m_strembuf;
};

template<class Stream, class ConstBufferSequence, class Handler>
async_read_request_op<Stream, ConstBufferSequence, Handler>
make_async_read_request_op(Stream & stream, request_opts & opts,
						   const ConstBufferSequence & buffer, Handler handler)
{
	return async_read_request_op<Stream, ConstBufferSequence, Handler>(
			   stream, opts, buffer, handler);
}

}

/*@}*/
/**
 * @defgroup async_read_request avhttpd::async_read_request
 *
 * @brief Start an asynchronous operation to read a http header from a stream.
 */
/*@{*/

/// Start an asynchronous operation to read a certain amount of data from a
/// stream.
/**
 * This function is used to asynchronously read a http header from a stream.
 * The function call always returns immediately. The asynchronous operation
 * will continue all http request header have been read or an error occurred.
 *
 * @li The supplied buffers are full. That is, the bytes transferred is equal to
 * the sum of the buffer sizes.
 *
 * @li An error occurred.
 *
 * This operation is implemented in terms of zero or more calls to the stream's
 * async_read_some function, and is known as a <em>composed operation</em>. The
 * program must ensure that the stream performs no other read operations (such
 * as async_read, the stream's async_read_some function, or any other composed
 * operations that perform reads) until this operation completes.
 *
 * @param s The stream from which the data is to be read. The type must support
 * the AsyncReadStream concept.
 *
 * @param buffers One or more buffers into which the data will be read. The sum
 * of the buffer sizes indicates the maximum number of bytes to read from the
 * stream. Although the buffers object may be copied as necessary, ownership of
 * the underlying memory blocks is retained by the caller, which must guarantee
 * that they remain valid until the handler is called.
 *
 * @param handler The handler to be called when the read operation completes.
 * Copies will be made of the handler as required. The function signature of the
 * handler must be:
 * @code void handler(
 *   const boost::system::error_code& error, // Result of operation.
 *
 *   std::size_t bytes_transferred           // Number of bytes copied into the
 *                                           // buffers. If an error occurred,
 *                                           // this will be the  number of
 *                                           // bytes successfully transferred
 *                                           // prior to the error.
 * ); @endcode
 * Regardless of whether the asynchronous operation completes immediately or
 * not, the handler will not be invoked from within this function. Invocation of
 * the handler will be performed in a manner equivalent to using
 * boost::asio::io_service::post().
 *
 * @par Example
 * To read into a single data buffer use the @ref buffer function as follows:
 * @code
 * boost::asio::async_read(s, boost::asio::buffer(data, size), handler);
 * @endcode
 * See the @ref buffer documentation for information on reading into multiple
 * buffers in one go, and how to use it with arrays, boost::array or
 * std::vector.
 *
 * @note This overload is equivalent to calling:
 * @code boost::asio::async_read(
 *     s, buffers,
 *     boost::asio::transfer_all(),
 *     handler); @endcode
 */

template<class Stream, class Handler>
void async_read_request(Stream & stream, request_opts & opts, Handler handler)
{
	detail::make_async_read_request_op(stream, opts, boost::asio::null_buffers(), handler);
}

template<class Stream, class ConstBufferSequence, class BufferedHandler>
void async_read_request(Stream & stream, request_opts & opts, const ConstBufferSequence & buffer, BufferedHandler handler)
{
	detail::make_async_read_request_op(stream, opts, buffer, handler);
}

}
