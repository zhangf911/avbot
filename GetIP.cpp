// GetIP.cpp : Defines the entry point for the console application.
//

#include <string>

#include <stdio.h>
#include <iostream>
#include <arpa/inet.h>
#include "IPLocation.h"

int main(int argc, char * argv[])
{
	IPLocation l;
	CIPLocation ipl(std::string("QQWry.Dat"));
	in_addr p;
	p.s_addr = inet_addr(argv[1]);
	l = ipl.GetIPLocation(p);
	std::cout << l.country << "  " << l.area << std::endl;
	return 0;
}

