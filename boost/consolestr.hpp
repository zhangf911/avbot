
#pragma once

#include <wchar.h>
#include <string>
#include <boost/locale.hpp>

#include "utf8.hpp"

#ifdef _WIN32
# include <windows.h>
#endif

inline std::string ansi_utf8( std::string const &source, const std::string &characters = "GB18030" )
{
	return boost::locale::conv::between( source, "UTF-8", characters ).c_str();
}


inline std::wstring ansi_wide( std::string const &source )
{
#ifdef _WIN32
	std::vector<wchar_t> szBuf(source.length()*2);
	MultiByteToWideChar(CP_ACP, 0, source.c_str(), source.length(), &szBuf[0], szBuf.capacity());
	return std::wstring(szBuf.data());
#else
	return utf8_wide(source);
#endif
}

// convert wide string for console output aka native encoding
// because mixing std::wcout and std::cout is broken on windows
inline std::string console_out_str(std::wstring in )
{
#ifdef _WIN32
	std::vector<char> szBuf(in.length()*6);
	WideCharToMultiByte(CP_ACP, 0, in.c_str(), -1, &szBuf[0], szBuf.capacity(), NULL, NULL);
	return std::string(szBuf.data());
#else
	return wide_utf8(in);
#endif
}

inline std::string utf8_ansi( std::string const &source, const std::string &characters = "GB18030" )
{
	return boost::locale::conv::between( source, characters, "UTF-8" ).c_str();
}


// convert utf8 for console output aka native encoding
// for linux ,  it does nothing (linux console always use utf8)
// for windows, it convert to CP_ACP
inline std::string console_out_str(const std::string & in)
{
#ifdef _WIN32
	return utf8_ansi(in);
#else
	return in;
#endif
}

