

#ifndef MYDEFS__H
#define MYDEFS__H

#include <sys/types.h>
#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
#include <winsock.h>

typedef  HANDLE FILE_HANDLE;
typedef	 DWORD	uint32_t;

#else

#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <netinet/in.h>

#define  FILE_HANDLE int

#endif

#if BIGENDIAN

uint32_t inline to_hostending(uint32_t v)
{
	uint32_t ret;
	ret  = ( v & 0x000000ff) << 24;
	ret |= ( v & 0x0000ff00) << 8 ;
	ret |= ( v & 0x00ff0000) >> 8;
	ret |= ( v & 0xff000000) >> 24;
	return ret;
}
#else
#define  to_hostending(x) (x)
#endif

#include <string>

#endif // MYDEFS__H