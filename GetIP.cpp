// GetIP.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "IPLocation.h"

#include <fcntl.h>
#include <io.h>

int main(int argc, char * argv[])
{
	try
	{
		CIPLocation ipl("QQWry.Dat");
//		in_addr p;
// 		if (argc > 1)
// 		{
// 			p.s_addr = inet_addr(argv[1]);
// 		} 
// 		else
// 		{
// 			char  in[80];
// 			printf("please inout IP address: ");
// 			scanf( "%s", in);
// 			p.s_addr = inet_addr(in);
// 		}
		
//		l = ipl.GetIPLocation(p);
		ipl.GetIPs(0,"*ÎÂÖÝ*","*");

/*		printf( "%s   %s\n",l.country,l.area);*/
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

