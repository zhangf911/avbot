// GetIP.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <winsock.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iconv.h>
#endif
#include "IPLocation.h"


// just a demo  !
extern int iii;
int main(int argc, char * argv[])
{
	try
	{
//		IPLocation l;
		CIPLocation ipl("QQWry.Dat");

//		in_addr p;
// 		if (argc > 1)
// 		{
// 			p.s_addr = inet_addr(argv[1]);
//		}
// 		else
// 		{
// 			char  in[80];
// 			printf("please inout IP address: ");
// 			scanf( "%s", in);
// 			p.s_addr = inet_addr(in);
// 		}
//
//		ipl.GetIPLocation(p,l);
//
//		printf("%s   %s\n",l.country,l.area);

		std::list<IP_regon> retips;

// 		char c[28]={0};
// 		char a[28]={0};
// 		printf("国家/地区:(支持通配符)");
// 
// 		scanf("%s",c);
// 		printf("地区/详细地址:(支持通配符)");
// 		scanf("%s",a);

//		ipl.GetIPs(&retips, c,a);

		ipl.GetIPs(&retips,"*杭州*","*网吧*");
		

		printf( "%ld found , %d次比较\n",retips.size(),iii);
	}
	catch ( char * e)
	{
#ifdef _WIN32
		MessageBox( GetForegroundWindow(),e,"err",MB_OK|MB_SYSTEMMODAL);
#else
		printf("%s\n",e);
#endif // _WIN32
	}
	return 0;

}

