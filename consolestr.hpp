
#pragma once

#include <wchar.h>
#include <string>
#include <boost/locale.hpp>

#include "utf8.hpp"

#ifdef _WIN32
# include <windows.h>
#endif

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

inline std::string console_utf8( std::string const &source)
{
#ifdef _WIN32
	return wide_utf8(ansi_wide(source).c_str()).c_str();
# endif
	return source;
}

// convert wide string for console output aka native encoding
// because mixing std::wcout and std::cout is broken on windows
inline std::string console_out_str(std::wstring in )
{
#ifdef _WIN32
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
#ifdef _WIN32
	return console_out_str(utf8_wide(in.c_str()).c_str());
#else
	return in;
#endif
}

