// GetIP.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "IPLocation.h"

int main(int argc, char * argv[])
{
	try
	{
		IPLocation l;
		CIPLocation ipl("QQWry.Dat");

		in_addr p;
 		if (argc > 1)
 		{
 			p.s_addr = inet_addr(argv[1]);
		}
 		else
 		{
 			char  in[80];
 			printf("please inout IP address: ");
 			scanf( "%s", in);
 			p.s_addr = inet_addr(in);
 		}

		l = ipl.GetIPLocation(p);

		printf("%s   %s",l.country,l.area);


		std::list<IP_regon> retips;

		ipl.GetIPs(&retips,"ÃÀ¹ú","*");

		printf( "%d found \n",retips.size());
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

