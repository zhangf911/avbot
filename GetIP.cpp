// GetIP.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "defs.h"
#include "IPLocation.h"

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
		scanf( "%s",c);

//		ipl.GetIPs(&retips, "美国","加州");
		ipl.GetIPs(&retips, c ,"*");

		printf( "%ld found \n",retips.size());
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

