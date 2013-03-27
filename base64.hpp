#pragma once

#include <string>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/algorithm/string.hpp>

namespace boost {

namespace detail{

struct is_base64_char {
  bool operator()(char x) { return boost::is_any_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+/=")(x);}
};

}


typedef	archive::iterators::transform_width< 
			archive::iterators::binary_from_base64<filter_iterator<detail::is_base64_char, std::string::iterator> >, 8, 6, char>
				base64decodeIterator;

typedef	archive::iterators::base64_from_binary<
			archive::iterators::transform_width<std::string::iterator , 6, 8> >
				base64encodeIterator;

// BASE64 解码.
template<typename Char>
std::basic_string<Char> base64_decode(std::basic_string<Char> str)
{
	// convert base64 characters to binary values
	std::basic_string<Char> result(base64decodeIterator(str.begin()) , base64decodeIterator(str.end()));
	return result.c_str();
}

// BASE64 编码.
template<typename Char>
std::basic_string<Char> base64_encode(std::basic_string<Char> src)
{
	char tail[3] = {0,0,0};

	std::basic_string<Char> result;
	uint one_third_len = src.length()/3;

	uint len_rounded_down = one_third_len*3;
	uint j = len_rounded_down + one_third_len;

	// 3 的整数倍可以编码.
	std::copy(base64encodeIterator(src.begin()), base64encodeIterator(src.begin() + len_rounded_down), result);

	// 结尾 0 填充以及使用 = 补上
	if (len_rounded_down != src.length())
	{
		uint i=0;
		for(; i < src.length() - len_rounded_down; ++i)
		{
			tail[i] = src[len_rounded_down+i];
		}

		std::copy(base64encodeIterator(tail), base64encodeIterator(tail + 3), result.begin() + j);

		// 写入尾部的 ==
        result.append(j + 3 - (src.length() + one_third_len), '=');
	}
	return result;
}

}
