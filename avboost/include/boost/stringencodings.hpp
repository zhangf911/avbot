
#pragma once

#include <wchar.h>
#include <string>
#include <boost/locale.hpp>

inline std::string ansi_utf8( std::string const &source, const std::string &characters = "GB18030" )
{
	return boost::locale::conv::between( source, "UTF-8", characters ).c_str();
}

// convert wide string for console output aka native encoding
// because mixing std::wcout and std::cout is broken on windows
inline std::string utf8_ansi( std::string const &source, const std::string &characters = "GB18030" )
{
	return boost::locale::conv::between( source, characters, "UTF-8" ).c_str();
}

// convert utf8 for console output aka native encoding
// for linux ,  it does nothing (linux console always use utf8)
// for windows, it convert to CP_ACP
inline std::string utf8_to_local_encode(const std::string & str)
{
#if defined(_WIN32) || defined(_WIN64)

	std::vector<WCHAR> wstr;

	int required_buffer_size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), NULL, 0);
	wstr.resize(required_buffer_size);

	wstr.resize(MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &wstr[0], wstr.capacity()));

	// 接着转换到本地编码

	required_buffer_size = WideCharToMultiByte(CP_ACP, 0, &wstr[0], wstr.size(), NULL, 0, NULL, NULL);
	std::vector<char> outstr;
	outstr.resize(required_buffer_size);

	int converted_size = WideCharToMultiByte(CP_ACP, 0, wstr.data(), wstr.size(), &outstr[0], required_buffer_size, NULL, NULL);
	outstr.resize(converted_size);

	return std::string(outstr.data(),outstr.size());
#else
	return str;
#endif
}

// convert to utf8 from console input aka native encoding
// for linux ,  it does nothing (linux console always use utf8)
// for windows, it convert from CP_ACP
inline std::string local_encode_to_utf8(const std::string & str)
{
#if defined(_WIN32) || defined(_WIN64)

	std::vector<WCHAR> wstr;

	int required_buffer_size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), NULL, 0);
	wstr.resize(required_buffer_size);

	wstr.resize(MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), &wstr[0], wstr.capacity()));

	// 接着转换到UTF8
	required_buffer_size = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], wstr.size(), NULL, 0, NULL, NULL);
	std::vector<char> outstr;
	outstr.resize(required_buffer_size);

	int converted_size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstr.size(), &outstr[0], required_buffer_size, NULL, NULL);
	outstr.resize(converted_size);

	return std::string(outstr.data(),outstr.size());
#else
	return str;
#endif
}
