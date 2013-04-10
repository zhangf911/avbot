
#pragma once

#include <wchar.h>
#include <string>
#include <boost/locale.hpp>

#include "utf8.hpp"

inline std::string ansi_utf8( std::string const &source, const std::string &characters = "GB18030" )
{
	return boost::locale::conv::between( source, "UTF-8", characters ).c_str();
}

// convert wide string for console output aka native encoding
// because mixing std::wcout and std::cout is broken on windows
inline std::string console_out_str(std::wstring in )
{
#ifdef WIN32
	std::vector<char> szBuf(in.length()*2);
	WideCharToMultiByte(CP_ACP, 0, in.c_str(), -1, &szBuf[0], szBuf.capacity(), NULL, NULL);
	return std::string(szBuf.data());
#else
	return wide_utf8(in);
#endif
}

// convert utf8 for console output aka native encoding
// for linux ,  it does nothing (linux console always use utf8)
// for windows, it convert to CP_ACP
inline std::string console_out_str(std::string in)
{
#ifdef WIN32
	return console_out_str(utf8_wide(in));
#else
	return in;
#endif
}

