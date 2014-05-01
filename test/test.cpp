/*
 * test.cpp
 *
 *  Created on: 2010-1-20
 *      Author: cai
 */
#include <list>
#include <string>
#include <stdexcept>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include "qqwry/ipdb.hpp"

// search for QQWry.Dat
// first, look for ./QQWry.Dat
// then look for /var/lib/QQWry.Dat
// then look for $EXEPATH/QQWry.Dat
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


int main(int argc,char * argv[])
{
	std::string ipfile = search_qqwrydat(argv[0]);
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
	return 0;
}


