// GetIP.cpp : Defines the entry point for the console application.
//

#include <string>

#include <stdio.h>
#include <iostream>
#include "IPLocation.h"

int main(int argc, char * argv[])
{
	IPLocation l;
	CIPLocation ipl(std::string("QQWry.Dat"));
	in_addr p;
	if (argc > 1)
	{
		p.s_addr = inet_addr(argv[1]);
	} 
	else
	{
		std::string in;
		std::cout << "please inout IP address: " ;
		std::cin >> in;
		p.s_addr = inet_addr(in.c_str());
	}

	l = ipl.GetIPLocation(p);
	std::cout << l.country << "  " << l.area << std::endl;
	return 0;
}

