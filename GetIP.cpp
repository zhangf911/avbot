// GetIP.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "defs.h"
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
		char c[28]={0};
		char a[28]={0};
		printf("国家/地区:(支持通配符)");

		scanf("%s",c);
		printf("地区/详细地址:(支持通配符)");
		scanf("%s",a);

		//scanf( "%s",c);

		ipl.GetIPs(&retips, c,a);

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

