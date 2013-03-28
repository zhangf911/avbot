#pragma once

#include <string>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <boost/iterator/filter_iterator.hpp>
#include <boost/algorithm/string.hpp>

namespace boost {

namespace detail{

struct is_base64_char {
  bool operator()(char x) { return boost::is_any_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+/=")(x);}
};

}


// typedef	archive::iterators::transform_width< 
// 			archive::iterators::binary_from_base64<filter_iterator<detail::is_base64_char, std::string::iterator> >, 8, 6, char>
// 				base64decodeIterator;

typedef	archive::iterators::transform_width< 
			archive::iterators::binary_from_base64<boost::archive::iterators::remove_whitespace< std::string::iterator > >, 8, 6, char>
				base64decodeIterator;

typedef	archive::iterators::base64_from_binary<
			archive::iterators::transform_width<std::string::iterator , 6, 8, char> >
				base64encodeIterator;

// BASE64 解码.
inline std::string base64_decode(std::string str)
{
	static int shrik_map[] = {0, 2, 1};

	// 移除尾部的  == 后面的 \r\n\r\n
	while ( boost::is_any_of("\r\n.")(* str.rbegin()))
		str.erase(str.length()-1);
	// 统计结尾的 = 数目
	std::string::reverse_iterator rit = str.rbegin();
	std::size_t	num = 0;
	while ( * rit == '=' )
	{
		rit ++;
		num ++;
	}

	BOOST_ASSERT(num < 3);

	std::size_t num_to_shrik = shrik_map[num];

	// convert base64 characters to binary values
	std::string  result(base64decodeIterator(str.begin()), base64decodeIterator(str.end()));

	result.erase(result.length() -1 - num_to_shrik,  num_to_shrik);
	return result;
}

// BASE64 编码.
inline std::string base64_encode(std::string src)
{
	char tail[3] = {0,0,0};

	std::vector<char> result(src.length()/3*4+6);
	uint one_third_len = src.length()/3;

	uint len_rounded_down = one_third_len*3;
	uint j = len_rounded_down + one_third_len;

	std::string	base64str;

	// 3 的整数倍可以编码.
	std::copy(base64encodeIterator(src.begin()), base64encodeIterator(src.begin() + len_rounded_down), result.begin());

	base64str = result.data();

	// 结尾 0 填充以及使用 = 补上
	if (len_rounded_down != src.length())
	{
		uint i=0;
		for(; i < src.length() - len_rounded_down; ++i)
		{
			tail[i] = src[len_rounded_down+i];
		}

		std::vector<char> tailbase(5);

		std::copy(base64encodeIterator(tail), base64encodeIterator(tail + 3), tailbase.begin());
		for (int k=0;k<(3-i);k++){
			tailbase[3-k] = '=';
		}
		
		base64str += tailbase.data();
	}
	return base64str;
}

}
