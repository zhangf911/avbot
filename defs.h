

#ifndef MYDEFS__H
#define MYDEFS__H

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif // _DEBUG


#ifdef _WIN32
#include <windows.h>
#include <winsock.h>

typedef  HANDLE FILE_HANDLE;

#else

#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iconv.h>

#endif

#endif // MYDEFS__H
