//
// http_stream.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2013 Jack (jack dot wgm at gmail dot com)
// Copyright (c) 2013 microcai ( microcaiicai at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __AVHTTPD_ERROR_CODEC_HPP__
#define __AVHTTPD_ERROR_CODEC_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <string>
#include <boost/system/system_error.hpp>
#include <boost/system/error_code.hpp>

#ifndef BOOST_SYSTEM_NOEXCEPT
#define BOOST_SYSTEM_NOEXCEPT BOOST_NOEXCEPT
#endif

namespace avhttpd {

namespace detail {
	class error_category_impl;
}

template<class error_category>
const boost::system::error_category& error_category_single()
{
	static error_category error_category_instance;
	return reinterpret_cast<const boost::system::error_category&>(error_category_instance);
}

inline const boost::system::error_category& error_category()
{
	return error_category_single<detail::error_category_impl>();
}

namespace errc {

/// HTTP error codes.
/**
 * The enumerators of type @c errc_t are implicitly convertible to objects of
 * type @c boost::system::error_code.
 *
 * @par Requirements
 * @e Header: @c <error_codec.hpp> @n
 * @e Namespace: @c avhttp::errc
 */
enum errc_t
{
	// Client-generated errors.

	/// The request's uri line was malformed.
	malformed_request_line = 1,

	version_not_supported,

	/// Invalid request method
	invalid_request_method,

	/// HTTP/1.1 demand that client send Host: header
	header_missing_host,

	/// The request's headers were malformed.
	malformed_request_headers,

	/// Header too large
	header_too_large,

	/// Invalid chunked encoding.
	invalid_chunked_encoding,

	// Server-generated status codes.
};

/// Converts a value of type @c errc_t to a corresponding object of type
/// @c boost::system::error_code.
/**
 * @par Requirements
 * @e Header: @c <error_codec.hpp> @n
 * @e Namespace: @c avhttp::errc
 */
inline boost::system::error_code make_error_code(errc_t e)
{
	return boost::system::error_code(static_cast<int>(e), avhttpd::error_category());
}

} // namespace errc
} // namespace avhttp

namespace boost {
namespace system {

template <>
struct is_error_code_enum<avhttpd::errc::errc_t>
{
  static const bool value = true;
};

} // namespace system
} // namespace boost

namespace avhttpd {
namespace detail {

class error_category_impl
  : public boost::system::error_category
{
	virtual const char* name() const BOOST_SYSTEM_NOEXCEPT
	{
		return "HTTP";
	}

	virtual std::string message(int e) const
	{
		switch (e)
		{
		case errc::malformed_request_line:
			return "Malformed request line";
		case errc::version_not_supported:
			return "version not supported";
		case errc::invalid_request_method:
			return "invalid request method";
		case errc::header_missing_host:
			return "header missing host";
		case errc::malformed_request_headers:
			return "Malformed request headers";
		case errc::header_too_large:
			return "header too large";
		case errc::invalid_chunked_encoding:
			return "Invalid chunked encoding";
		default:
			return "Unknown HTTP error";
		}
	}
};

} // namespace detail
} // namespace avhttpd

#endif // __AVHTTPD_ERROR_CODEC_HPP__
