// IPLocation.cpp: implementation of the CIPLocation class.
//
//////////////////////////////////////////////////////////////////////


#include <sys/stat.h>
#include <sys/types.h>

#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "defs.h"	// Added by ClassView
#include "IPLocation.h"

#define REDIRECT_MODE_1 1
#define REDIRECT_MODE_2 2
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifndef _WIN32
static int code_convert(char *outbuf, size_t outlen, char *inbuf, size_t inlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;
	
	cd = iconv_open("UTF-8", "GBK");
	if (cd == 0)
		return -1;
	
	memset(outbuf, '\0', outlen);
	if (iconv(cd, pin, &inlen, pout, &outlen) == (size_t) -1)
	{
		return -1;
	}
	iconv_close(cd);
	
	return 0;
}
#endif

CIPLocation::CIPLocation( char * memptr, size_t len )
{
	m_file = memptr;
	m_filesize = len;
	
	m_first_record = GetDWORD(0);
	m_last_record = GetDWORD(4);
	m_curptr = memptr;
#ifdef _WIN32
	m_filemap = NULL;
	m_ipfile = INVALID_HANDLE_VALUE;
#else
	m_backup_byfile = false;
#endif
}

CIPLocation::CIPLocation(char* ipDateFile)
{
#ifndef _WIN32
	int 
#endif // _WIN32
	m_ipfile = 
#ifdef _WIN32
	CreateFile(ipDateFile,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
#else
	open(ipDateFile, O_RDONLY);
#endif // _WIN32
	if ( -1 == (int)m_ipfile )
	{
		throw("File Open Failed!");
	}

#ifdef _WIN32
	m_filesize = GetFileSize(m_ipfile,0);
#else
	struct stat fst;
	fstat(m_ipfile, &fst);
	m_filesize = fst.st_size;
#endif // _WIN#32
#ifdef _WIN32
	m_filemap = CreateFileMapping((HANDLE)m_ipfile,0,PAGE_READONLY,0,m_filesize,0);

	if ( !m_filemap )
	{
		throw ("CreatFileMapping Failed!");
	}

	m_file = (char*)MapViewOfFile(m_filemap,FILE_MAP_READ,0,0,m_filesize);
#else
	m_file = (char*) mmap(0, m_filesize, PROT_READ, MAP_PRIVATE, m_ipfile, 0);
#endif

	if (!m_file)
	{
		throw ("Creat File Mapping Failed!");
	}

#ifndef _WIN32
	m_backup_byfile = true;
	close(m_ipfile);
#endif // _WIN32
	m_curptr = m_file;
	m_first_record = GetDWORD(0);
	m_last_record = GetDWORD(4);
}

CIPLocation::~CIPLocation()
{
#ifdef _WIN32
	if (m_filemap)
	{
		UnmapViewOfFile(m_file);
		CloseHandle(m_filemap);
	}
	if (m_ipfile!= INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_ipfile);
	}
#else
	if (m_backup_byfile)
	{
		munmap(m_file, m_filesize);
	}
#endif // _WIN32
}

static bool IP_IN_regon(uint32_t ip, uint32_t ip1, uint32_t ip2)
{
#if 0
	size_t i;
	for (i = 0; i < 4; ++i)
	{
		if( ((u_char*)(&ip))[i] >= ((u_char*)(&ip1))[i] && ((u_char*)(&ip))[i] <= ((u_char*)(&ip2))[i] )
		continue;
		else
		return false;
	}
	return false;
#endif
	return (ip >= ip1 && ip <= ip2);

}

char * CIPLocation::FindRecord(in_addr ip)
{
	//TODO: need faster implementation
	size_t i;

	for (i = m_first_record; i < m_last_record; i += 7)
	{
#ifdef DEBUG
		in_addr ip1,ip2;
		ip1.s_addr = htonl(GetDWORD(i));

		printf("start ip is %s ", inet_ntoa(ip1));

		ip2.s_addr = htonl(GetDWORD(Get3BYTE3(i + 4)));

		printf("end ip is %s\n",inet_ntoa(ip2));
#endif
		if (IP_IN_regon(ntohl(ip.s_addr), GetDWORD(i), GetDWORD(Get3BYTE3(i + 4))))
		{
			return m_file + Get3BYTE3(i + 4);
		}
	}
	return 0;
}

IPLocation CIPLocation::GetIPLocation(char *ptr)
{
	IPLocation ret;
	char * areaptr = 0;
	switch (ptr[0])
	{
	case REDIRECT_MODE_1:
		return GetIPLocation(m_file + Get3BYTE3(ptr + 1));
	case REDIRECT_MODE_2:
		areaptr = ptr + 4;
	default:
		Get_String(ptr, ret.country);
		if (!areaptr)
			areaptr = ptr + strlen(ret.country) + 1;
	}
	Get_String(areaptr, ret.area);
	return ret;
}

char * CIPLocation::Get_String(char *p, char * out)
{
	char *pp;
	switch (p[0])
	{
	case REDIRECT_MODE_1:
		pp = Get_String(m_file + Get3BYTE3(p + 1),out);
		break;
	case REDIRECT_MODE_2:
		pp = m_file + Get3BYTE3(p + 1);
		break;
	default:
		pp = p;
	}
#ifndef _WIN32
	code_convert(out, 256, pp, strlen(pp));
#else
	strcpy(out,pp);
#endif
	return out;
}

IPLocation CIPLocation::GetIPLocation(in_addr ip)
{
	char * ptr = FindRecord(ip);
	if (!ptr)
	{
		throw ("IP Record Not Found");
	}
	return GetIPLocation(ptr + 4);
}

bool match_exp(char * input , const std::string const & exp )
{
return false;

}

size_t CIPLocation::GetIPs( std::list<int> * retips,char *exp)
{
	
	return 0;	
}
