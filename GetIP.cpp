// GetIP.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "IPLocation.h"

#include <fcntl.h>
#include <io.h>

int main(int argc, char * argv[])
{
// 	char buf[80];
// 	int fd =  open("QQWry.Dat",O_RDONLY);
// 	
// 	read(fd,buf,80);

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
		printf( "%s   %s\n",l.country,l.area);

	}
	catch ( char * e)
	{
#ifdef _WIN32
		MessageBox(0,e,"err",MB_OK);
#else
		printf("%s\n",e);
#endif // _WIN32
	}
	return 0;
}

