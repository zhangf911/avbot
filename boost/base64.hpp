#pragma once

#include <string>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

namespace boost {

typedef	boost::archive::iterators::transform_width< 
			boost::archive::iterators::binary_from_base64<std::string::iterator>, 8, 6, char>
				base64Iterator;
	
// BASE64 解码.
std::string base64_decode(std::string str)
{
	// convert base64 characters to binary values
	return  std::string( base64Iterator(str.begin()) , base64Iterator(str.end()));
}

}