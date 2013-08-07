
#pragma once

#include <boost/asio.hpp>
#include <boost/asio/detail/push_options.hpp>

namespace avhttpd{
namespace detail{

  template <typename AsyncReadStream, typename Allocator, typename ReadHandler>
  class read_until_delim_string_op
  {
  public:
    read_until_delim_string_op(AsyncReadStream& stream,
        boost::asio::basic_streambuf<Allocator>& streambuf,
        const std::string& delim, ReadHandler& handler)
      : stream_(stream),
        streambuf_(streambuf),
        delim_(delim),
        start_(0),
        search_position_(0),
        handler_(BOOST_ASIO_MOVE_CAST(ReadHandler)(handler))
    {
    }

#if defined(BOOST_ASIO_HAS_MOVE)
    read_until_delim_string_op(const read_until_delim_string_op& other)
      : stream_(other.stream_),
        streambuf_(other.streambuf_),
        delim_(other.delim_),
        start_(other.start_),
        search_position_(other.search_position_),
        handler_(other.handler_)
    {
    }

    read_until_delim_string_op(read_until_delim_string_op&& other)
      : stream_(other.stream_),
        streambuf_(other.streambuf_),
        delim_(BOOST_ASIO_MOVE_CAST(std::string)(other.delim_)),
        start_(other.start_),
        search_position_(other.search_position_),
        handler_(BOOST_ASIO_MOVE_CAST(ReadHandler)(other.handler_))
    {
    }
#endif // defined(BOOST_ASIO_HAS_MOVE)

    void operator()(const boost::system::error_code& ec,
        std::size_t bytes_transferred, int start = 0)
    {
      const std::size_t not_found = (std::numeric_limits<std::size_t>::max)();
      std::size_t bytes_to_read;
      switch (start_ = start)
      {
      case 1:
        for (;;)
        {
          {
            // Determine the range of the data to be searched.
            typedef typename boost::asio::basic_streambuf<
              Allocator>::const_buffers_type const_buffers_type;
            typedef boost::asio::buffers_iterator<const_buffers_type> iterator;
            const_buffers_type buffers = streambuf_.data();
            iterator begin = iterator::begin(buffers);
            iterator start_pos = begin + search_position_;
            iterator end = iterator::end(buffers);

            // Look for a match.
            std::pair<iterator, bool> result = boost::asio::detail::partial_search(
                start_pos, end, delim_.begin(), delim_.end());
            if (result.first != end && result.second)
            {
              // Full match. We're done.
              search_position_ = result.first - begin + delim_.length();
              bytes_to_read = 0;
            }

            // No match yet. Check if buffer is full.
            else if (streambuf_.size() == streambuf_.max_size())
            {
              search_position_ = not_found;
              bytes_to_read = 0;
            }

            // Need to read some more data.
            else
            {
              if (result.first != end)
              {
                // Partial match. Next search needs to start from beginning of
                // match.
                search_position_ = result.first - begin;
              }
              else
              {
                // Next search can start with the new data.
                search_position_ = end - begin;
              }

              bytes_to_read = boost::asio::read_size_helper(streambuf_, 1);
            }
          }

          // Check if we're done.
          if (!start && bytes_to_read == 0)
            break;

          // Start a new asynchronous read operation to obtain more data.
          stream_.async_read_some(streambuf_.prepare(bytes_to_read),
              BOOST_ASIO_MOVE_CAST(read_until_delim_string_op)(*this));
          return; default:
          streambuf_.commit(bytes_transferred);
          if (ec || bytes_transferred == 0)
            break;
        }

        const boost::system::error_code result_ec =
          (search_position_ == not_found)
          ? boost::asio::error::not_found : ec;

        const std::size_t result_n =
          (ec || search_position_ == not_found)
          ? 0 : search_position_;

        handler_(result_ec, result_n);
      }
    }

  //private:
    AsyncReadStream& stream_;
    boost::asio::basic_streambuf<Allocator>& streambuf_;
    std::string delim_;
    int start_;
    std::size_t search_position_;
    ReadHandler handler_;
  };

  template <typename AsyncReadStream, typename Allocator, typename ReadHandler>
  inline void* asio_handler_allocate(std::size_t size,
      read_until_delim_string_op<AsyncReadStream,
        Allocator, ReadHandler>* this_handler)
  {
    return boost_asio_handler_alloc_helpers::allocate(
        size, this_handler->handler_);
  }

  template <typename AsyncReadStream, typename Allocator, typename ReadHandler>
  inline void asio_handler_deallocate(void* pointer, std::size_t size,
      read_until_delim_string_op<AsyncReadStream,
        Allocator, ReadHandler>* this_handler)
  {
    boost_asio_handler_alloc_helpers::deallocate(
        pointer, size, this_handler->handler_);
  }

  template <typename AsyncReadStream, typename Allocator, typename ReadHandler>
  inline bool asio_handler_is_continuation(
      read_until_delim_string_op<AsyncReadStream,
        Allocator, ReadHandler>* this_handler)
  {
    return this_handler->start_ == 0 ? true
      : boost_asio_handler_cont_helpers::is_continuation(
          this_handler->handler_);
  }

  template <typename Function, typename AsyncReadStream,
      typename Allocator, typename ReadHandler>
  inline void asio_handler_invoke(Function& function,
      read_until_delim_string_op<AsyncReadStream,
        Allocator, ReadHandler>* this_handler)
  {
    boost_asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
  }

  template <typename Function, typename AsyncReadStream,
      typename Allocator, typename ReadHandler>
  inline void asio_handler_invoke(const Function& function,
      read_until_delim_string_op<AsyncReadStream,
        Allocator, ReadHandler>* this_handler)
  {
    boost_asio_handler_invoke_helpers::invoke(
        function, this_handler->handler_);
  }
} // namespace detail


/*
 * 不同于 boost::asio::async_read_until, 本 read_until 一次只读取一个字节。
 * 虽然导致了代码效率的下降，但是也正因为如此，造就了 async_read_request 的简洁。
 *
 * 用户无需准备 streambuf 就可以使用.
 */

template <typename AsyncReadStream, typename Allocator, typename ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
    void (boost::system::error_code, std::size_t))
async_read_until(AsyncReadStream& s,
    boost::asio::basic_streambuf<Allocator>& b, const std::string& delim,
    BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
{
	// If you get an error on the following line it means that your handler does
	// not meet the documented type requirements for a ReadHandler.

	boost::asio::detail::async_result_init <
	ReadHandler, void (boost::system::error_code, std::size_t) > init(
		BOOST_ASIO_MOVE_CAST(ReadHandler)(handler));

	detail::read_until_delim_string_op < AsyncReadStream,
		   Allocator, typename boost::asio::handler_type<ReadHandler,
				   void (boost::system::error_code, std::size_t)>::type > (
			   s, b, delim, init.handler)(
			   boost::system::error_code(), 0, 1);

	return init.result.get();
}

} // namespace avhttpd

#include <boost/asio/detail/pop_options.hpp>

