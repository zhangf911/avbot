// IPLocation.cpp: implementation of the CIPLocation class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <iconv.h>
#include "IPLocation.h"

#define REDIRECT_MODE_1 1
#define REDIRECT_MODE_2 2
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIPLocation::CIPLocation(std::string ipDateFile)
{
#ifndef _WIN32
	int
#endif // _WIN32
			m_ipfile =
#ifdef _WIN32
			CreateFile(ipDateFile.c_str(),GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
#else
			open(ipDateFile.c_str(), O_RDONLY);
#endif // _WIN32
	if (m_ipfile == (FILE_HANDLE) (-1))
	{
		throw std::string("File Open Failed!");
	}

#ifdef _WIN32
	m_filesize = GetFileSize(m_ipfile,0);
#else
	struct stat fst;
	fstat(m_ipfile, &fst);
	m_filesize = fst.st_size;
#endif // _WIN#32
#ifdef _WIN32
	m_filemap = CreateFileMapping(m_ipfile,0,PAGE_READONLY,0,m_filesize,0);

	if ( !m_filemap )
	{
		throw std::string("CreatFileMapping Failed!");
	}

	m_file = (char*)MapViewOfFile(m_filemap,FILE_MAP_READ,0,0,m_filesize);
#else
	m_file = (char*) mmap(0, m_filesize, PROT_READ, MAP_PRIVATE, m_ipfile, 0);
#endif
	if (!m_file)
	{
		throw std::string("Creat File Mapping Failed!");
	}
#ifndef _WIN32
	close(m_ipfile);
#endif // _WIN32
	m_curptr = m_file;
	m_first_record = GetDWORD(0);
	m_last_record = GetDWORD(4);
}

CIPLocation::~CIPLocation()
{
#ifdef _WIN32
	UnmapViewOfFile( m_file);
	CloseHandle(m_filemap);
	CloseHandle(m_ipfile);
#else
	munmap(m_file, m_filesize);
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
	char * outstr = new char[1024];
	switch (ptr[0])
	{
	case REDIRECT_MODE_1:
		return GetIPLocation(m_file + Get3BYTE3(ptr + 1));
	case REDIRECT_MODE_2:
		areaptr = ptr + 4;
	default:
		ret.country = Get_String(ptr, outstr);
		if (!areaptr)
			areaptr = ptr + ret.country.length() + 1;
	}
	ret.area = Get_String(areaptr, outstr);
	delete[] outstr;
	return ret;
}

IPLocation CIPLocation::GetIPLocation(in_addr ip)
{

	char * ptr = FindRecord(ip);
	if (!ptr)
	{
		throw std::string("Not Found");
	}
	return GetIPLocation(ptr + 4);
}
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
