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

#include <algorithm>

#include "defs.h"	// Added by ClassView
#include "IPLocation.h"

#pragma pack(1)
typedef struct{
	uint32_t	ip;
	struct _offset_{
		char	_offset[3];
		inline operator	size_t ()
		{
			return ( ( *(uint32_t *) _offset) & 0xFFFFFF );
		}		
	}offset;
}RECORD_INDEX;
#pragma pack()

static uint32_t inline Get3BYTE3(char * var_ptr)
{
	uint32_t ret = 0;
	memcpy(&ret, var_ptr, 3);
	return to_hostending(ret);
}

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

static int IP_IN_regon(uint32_t ip, uint32_t ip1, uint32_t ip2)
{	
	if (  ip < ip1)
	{
		return -1;
	}else if ( ip > ip2 )
	{
		return 1;
	} 
	return 0;
}

char * CIPLocation::FindRecord(in_addr ip)
{
	//TODO: need faster implementation
	size_t i = 0;
	size_t l = 0 ;
	size_t r = (m_last_record - m_first_record) / 7;

	ip.s_addr = ntohl(ip.s_addr);

	RECORD_INDEX * pindex = ( RECORD_INDEX* ) (m_file + m_first_record);

	for ( size_t tryed = 0 ; tryed < 50 ; tryed ++ )
	{
#ifdef DEBUG
		in_addr ip1,ip2;
		ip1.s_addr = htonl( pindex[i].ip );
		ip2.s_addr = htonl(GetDWORD( pindex[i].offset));
		
		printf("start ip is %s ", inet_ntoa(ip1));printf("end ip is %s\n",inet_ntoa(ip2));

#endif
		switch ( IP_IN_regon(ip.s_addr, pindex[i].ip, GetDWORD( pindex[i].offset)) )
		{
		case 0:
			return m_file + pindex[i].offset ;
		case 1:			
			l = i;
			i +=  (r - i)/2;
			if ( l == i)return 0;
			break;
		case -1:
			r = i;
			i -= ( i  -l) /2 ;			
			break;
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
		return GetIPLocation(m_file + ::Get3BYTE3(ptr + 1));
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
		pp = Get_String(m_file + ::Get3BYTE3(p + 1),out);
		break;
	case REDIRECT_MODE_2:
		pp = m_file + ::Get3BYTE3(p + 1);
		break;
	default:
		pp = p;
	}
#ifndef _WIN32
	code_convert(out, 128, pp, strlen(pp));
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

static bool match_exp(char * input , const char  * const exp )
{

	return false;
}

bool CIPLocation::MatchRecord( char * pRecord , const char *exp_country ,const char * exp_area ,std::list< uint32_t > &country_matched )
{
	bool match;
	switch ( *pRecord )
	{
	case REDIRECT_MODE_1:
		return MatchRecord(pRecord,exp_country,exp_country,country_matched);		
	case REDIRECT_MODE_2:		
		if ( std::find( country_matched.begin() ,country_matched.end(),::Get3BYTE3( pRecord +4 ) ) != country_matched.end() )
		{
			match = true;
		}else
		{

			
		}
		break;
	default:
		match = match_exp( pRecord +4 , exp_country);
	}	
	return match;
}


size_t CIPLocation::GetIPs( std::list<int> * retips,const char *exp_country ,const char * exp_area)
{
	RECORD_INDEX * pindex;
	
	size_t i ;
	bool match;
	std::list< uint32_t > country_matched; // matched country
	
	for ( i = m_first_record ; i < m_last_record ; i+=7)
	{
		pindex = (RECORD_INDEX *) ( m_file + i);

		char * pRecord = m_file + pindex->offset +4 ;

		match = MatchRecord( pRecord , exp_country , exp_area ,country_matched);
		if ( match)
		{
			retips
		}

	}
	return 0;	
}
