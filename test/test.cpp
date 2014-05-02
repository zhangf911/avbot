/*
 * test.cpp
 *
 *  Created on: 2010-1-20
 *      Author: cai
 */
#include <list>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>

#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#else
#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <wininet.h>
#endif

#include <memory>

#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <string.h>

#include "qqwry/ipdb.hpp"

#pragma pack(1)

struct copywritetag{
	uint32_t sign;// "CZIP"
	uint32_t version;//一个和日期有关的值
	uint32_t unknown1;// 0x01
	uint32_t size;// qqwry.rar大小
	uint32_t unknown2;
	uint32_t key;// 解密qqwry.rar前0x200字节所需密钥
	char text[128];//提供商
	char link[128];//网址
};

#pragma pack()


#ifndef DEBUG
#ifdef _WIN32
static std::string internetDownloadFile(std::string url)
{
	std::shared_ptr<void> hinet(::InternetOpen(0, INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0), InternetCloseHandle);
	std::shared_ptr<void> hUrl(
		::InternetOpenUrl((HINSTANCE)hinet.get(), url.c_str(), 0, 0, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD, 0),
		::InternetCloseHandle
		);

	std::string filecontent;

	while (true)
	{
		// 读取文件
		std::vector<char> buf;

		buf.resize(1024);

		DWORD readed;

		InternetReadFile((HINTERNET)hUrl.get(), &buf[0], buf.size(), &readed);

		buf.resize(readed);

		if (readed == 0)
		{
			// 读取完毕，return
			return filecontent;
		}

		filecontent.append(buf.data(), readed);
	}
}
#else
// linux version
static std::string internetDownloadFile(std::string url)
{
	// call wget
	system("wget " + url);
	// TODO
//	return get_file_content();
}
#endif
#else
// common debug version that just load from file
static std::string internetDownloadFile(std::string url)
{
	std::ifstream f;
	if (url == "http://update.cz88.net/ip/copywrite.rar")
	{
		f.open("copywrite.rar", std::ios::binary);
	}
	else if (url == "http://update.cz88.net/ip/qqwry.rar")
	{
		// 打开 qqwry.rar 吧
		f.open("qqwry.rar", std::ios::binary);
	}
	else
		return "";

	std::string ret;
	ret.resize(819200);
 	ret.resize( f.readsome(&ret[0], 819200) );
	return ret;
}
#endif

inline std::string wstring2string(std::wstring wstr)
{
#if defined(_WIN32) || defined(_WIN64)

	// 接着转换到本地编码

	size_t required_buffer_size = WideCharToMultiByte(CP_ACP, 0, &wstr[0], wstr.size(), NULL, 0, NULL, NULL);
	std::vector<char> outstr;
	outstr.resize(required_buffer_size);

	int converted_size = WideCharToMultiByte(CP_ACP, 0, wstr.data(), wstr.size(), &outstr[0], required_buffer_size, NULL, NULL);
	outstr.resize(converted_size);

	return std::string(outstr.data(), outstr.size());
#else
	// TODO, use with iconv
	return str;
#endif
}


// search for QQWry.Dat
// first, look for ./QQWry.Dat
// then look for /var/lib/QQWry.Dat
// then look for $EXEPATH/QQWry.Dat
#ifndef _WIN32
std::string search_qqwrydat(const std::string exefile)
{
	if( access("QQWry.Dat", O_RDONLY) == 0){
		return  "QQWry.Dat";
	}

	if ( access("/var/lib//QQWry.Dat", O_RDONLY) == 0){
		return "/var/lib//QQWry.Dat";
	}
	// 找 exe 的位置
	if ( exefile.find_last_of("/") != std::string::npos){
		std::string ipfile = exefile.substr(0, exefile.find_last_of("/")+1);
		ipfile += "QQWry.Dat";
		return ipfile;
	}
	throw std::runtime_error("QQWry.Dat database not found");
}
#else
static bool check_exist(std::string filename)
{
	CLSID clsid;
	CLSIDFromProgID(CComBSTR("Scripting.FileSystemObject"), &clsid);
	void *p;
	CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER, __uuidof(IDispatch), &p);
	CComPtr<IDispatch> disp(static_cast<IDispatch*>(p));
	CComDispatchDriver dd(disp);
	CComVariant arg(filename.c_str());
	CComVariant ret(false);
	dd.Invoke1(CComBSTR("FileExists"), &arg, &ret);
	return ret.boolVal!=0;
}

std::string get_parent_path(std::string path)
{
	CLSID clsid;
	CLSIDFromProgID(CComBSTR("Scripting.FileSystemObject"), &clsid);
	void *p;
	CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER, __uuidof(IDispatch), &p);
	CComPtr<IDispatch> disp(static_cast<IDispatch*>(p));
	CComDispatchDriver dd(disp);
	CComVariant arg(path.c_str());
	CComVariant ret("");
	dd.Invoke1(CComBSTR("GetParentFolderName"), &arg, &ret);
	return wstring2string(ret.bstrVal);
}

std::string search_qqwrydat()
{
	std::vector<char> Filename;
	Filename.resize(_MAX_FNAME);

	GetModuleFileName(NULL, &Filename[0], _MAX_FNAME);

	std::string exepath = get_parent_path( Filename.data() );

	// 下载 copywrite.rar
	std::string copywrite = internetDownloadFile("http://update.cz88.net/ip/copywrite.rar");
	// 获取解压密钥 key
	uint32_t key = ntohl(reinterpret_cast<const copywritetag*>(copywrite.data())->key);
	std::string link = reinterpret_cast<const copywritetag*>(copywrite.data())->link;
	// 下载 qqwry.rar
	std::string qqwrydat = internetDownloadFile("http://update.cz88.net/ip/qqwry.rar");

	for (int i = 0; i<0x200; i++)
	{
		key *= 0x805;
		key++;
		key &=  0xFF;
		qqwrydat[i] ^= key;
	}

	// 解压 qqwry.rar 为 qqwry.dat
	std::ofstream ofile("qqwry.rar", std::ios::binary);
	ofile.write(qqwrydat.data(), qqwrydat.size());
	ofile.close();

	if (check_exist("QQWry.Dat"))
		return "QQWry.Dat";

	if (check_exist(exepath + "\\" + "QQWry.Dat"))
		return exepath + "\\" + "QQWry.Dat";

	throw  std::runtime_error("not found");
}
#endif

int main(int argc,char * argv[])
{
#ifdef _WIN32
	CoInitialize(NULL);
#endif
#ifdef _WIN32
	std::string ipfile = search_qqwrydat();
#else
	std::string ipfile = search_qqwrydat(argv[0]);
#endif
	QQWry::ipdb iplook(ipfile.c_str());

	//使用 CIPLocation::GetIPLocation 获得对应 ip 地址的地区表示.
	in_addr ip;

	if (argc <= 2)
	{

		ip.s_addr = inet_addr(argc == 2 ? argv[1] : "8.8.8.8");
		if (ip.s_addr && ip.s_addr != (-1)) // 如果命令行没有键入合法的ip地址，就进行地址-》ip的操作
		{
			QQWry::IPLocation iplocation = iplook.GetIPLocation(ip);
			puts(iplocation.country);
			puts(iplocation.area);
		}

		return 0;
	}
	//使用 CIPLocation::GetIPs 获得匹配地址的所有 ip 区间
	std::list<QQWry::IP_regon> ipregon;

	char country[80],area[80];

	if(argc!=3)
		ipregon = iplook.GetIPs("浙江省温州市","*网吧*");
	else
		ipregon = iplook.GetIPs(argv[1],argv[2]);

	std::list<QQWry::IP_regon>::iterator it;

	//在一个循环中打印出来
	for(it=ipregon.begin(); it != ipregon.end() ; it ++)
	{
		char	ipstr[30];
		strcpy(ipstr,inet_ntoa(it->start));

		printf(  "%s to %s :%s %s\n",ipstr, inet_ntoa(it->end), it->location.country,it->location.area);
	}
#ifdef _WIN32
	CoUninitialize();
#endif
	return 0;
}


