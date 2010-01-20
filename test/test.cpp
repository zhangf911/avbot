/*
 * test.cpp
 *
 *  Created on: 2010-1-20
 *      Author: cai
 */
#include <list>
#include <string.h>

#include <stdio.h>

#include <arpa/inet.h>
#include "../IPLocation.h"

int main(int argc,char * argv[])
{
	const char * ipfile = "QQWry.Dat";
	CIPLocation	iplook(ipfile);

	//使用 CIPLocation::GetIPLocation 获得对应 ip 地址的地区表示.
	in_addr ip;

	if (argc <= 2)
	{

		ip.s_addr = inet_addr(argc == 2 ? argv[1] : "8.8.8.8");
		if (ip.s_addr && ip.s_addr != (-1)) // 如果命令行没有键入合法的ip地址，就进行地址-》ip的操作
		{
			IPLocation iplocation = iplook.GetIPLocation(ip, iplocation);
			puts(iplocation.country);
			puts(iplocation.area);
		}
	}
	//使用 CIPLocation::GetIPs 获得匹配地址的所有 ip 区间
	std::list<IP_regon> ipregon;

	char country[80],area[80];

	if(argc!=3)
		iplook.GetIPs(&ipregon,"浙江省温州市","*网吧*");
	else
		iplook.GetIPs(&ipregon,argv[1],argv[2]);

	std::list<IP_regon>::iterator it;

	//在一个循环中打印出来
	for(it=ipregon.begin(); it != ipregon.end() ; it ++)
	{
		char	ipstr[30];
		strcpy(ipstr,inet_ntoa(it->start));

		printf(  "%s to %s :%s %s\n",ipstr, inet_ntoa(it->end), it->location.country,it->location.area);
	}
	return 0;
}


