// IPLocation.cpp: implementation of the CIPLocation class.
//
//////////////////////////////////////////////////////////////////////


#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include <algorithm>

#include "defs.h"	// Added by ClassView
#include "IPLocation.h"

#pragma pack(1)
typedef struct
{
	uint32_t ip;
	struct _offset_
	{
		char _offset[3];
		inline operator size_t()
		{
			return ((*(uint32_t *) _offset) & 0xFFFFFF);
		}
	} offset;
} RECORD_INDEX;
#pragma pack()

static uint32_t inline Get3BYTE3(char const * var_ptr)
{
	uint32_t ret = (*(uint32_t*)var_ptr) & 0xffffff;
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

static int utf8_gbk(char *outbuf, size_t outlen, char *inbuf, size_t inlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open( "GBK","UTF-8");
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

CIPLocation::CIPLocation(char * memptr, size_t len)
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

CIPLocation::CIPLocation(const char* ipDateFile)
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
	if (-1 == (int) m_ipfile)
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
		throw("Creat File Mapping Failed!");
	}

#ifndef _WIN32
	m_backup_byfile = true;
	close(m_ipfile);
#endif // _WIN32
	m_curptr = (char*) m_file;
	m_first_record = GetDWORD(0);
	m_last_record = GetDWORD(4);
}

CIPLocation::~CIPLocation()
{
#ifdef _WIN32
	if (m_filemap)
	{
		UnmapViewOfFile((void*)m_file);
		CloseHandle(m_filemap);
	}
	if (m_ipfile!= INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_ipfile);
	}
#else
	if (m_backup_byfile)
	{
		munmap((void*)m_file, m_filesize);
	}
#endif // _WIN32
}

static int IP_IN_regon(uint32_t ip, uint32_t ip1, uint32_t ip2)
{
	if (ip < ip1)
	{
		return -1;
	}
	else if (ip > ip2)
	{
		return 1;
	}
	return 0;
}

char const * CIPLocation::FindRecord(in_addr ip)
{
	//TODO: need faster implementation
	size_t i = 0;
	size_t l = 0;
	size_t r = (m_last_record - m_first_record) / 7;

	ip.s_addr = ntohl(ip.s_addr);

	RECORD_INDEX * pindex = (RECORD_INDEX*) (m_file + m_first_record);

	for (size_t tryed = 0; tryed < 50; tryed++)
	{
#ifdef DEBUG
		in_addr ip1,ip2;
		ip1.s_addr = htonl( pindex[i].ip );
		ip2.s_addr = htonl(GetDWORD( pindex[i].offset));

		printf("start ip is %s ", inet_ntoa(ip1));printf("end ip is %s\n",inet_ntoa(ip2));

#endif
		switch (IP_IN_regon(ip.s_addr, pindex[i].ip, GetDWORD(pindex[i].offset)))
		{
		case 0:
			return m_file + pindex[i].offset;
		case 1:
			l = i;
			i += (r - i) / 2;
			if (l == i)
				return 0;
			break;
		case -1:
			r = i;
			i -= (i - l) / 2;
			break;
		}
	}
	return 0;
}

IPLocation & CIPLocation::GetIPLocation(char const *ptr,IPLocation & ret)
{
	char const * areaptr = 0;
	switch (ptr[0])
	{
	case REDIRECT_MODE_1:
		return GetIPLocation( (char const *)(m_file + ::Get3BYTE3(ptr + 1)) , ret);
	case REDIRECT_MODE_2:
		areaptr = ptr + 4;
	default:
		strcpy(ret.country,Get_String(ptr));
		if (!areaptr)
			areaptr = ptr + strlen(strcpy(ret.country,Get_String(ptr))) + 1;
	}
	strcpy(ret.area,Get_String(areaptr));
	return ret;
}

char* CIPLocation::Get_String( char const * p)
{
	char const *pp;
	switch (p[0])
	{
	case REDIRECT_MODE_1:
		return Get_String(m_file + ::Get3BYTE3(p + 1));
		break;
	case REDIRECT_MODE_2:
		return (char*)m_file + ::Get3BYTE3(p + 1);
		break;
	default:
		return (char*)p;
	}
//#ifndef _WIN32
//	code_convert(out, 128, (char*)pp, strlen(pp));
//#else
//	if (out)
//		strncpy(out,pp,128);
//#endif
	return (char*)pp;
}

IPLocation& CIPLocation::GetIPLocation(in_addr ip, IPLocation & l)
{
	char const * ptr = FindRecord(ip);
	if (!ptr)
	{
		throw("IP Record Not Found");
	}
#ifndef _WIN32
	IPLocation gbk;
	GetIPLocation(ptr + 4,gbk);

	code_convert(l.country, 128, gbk.country, strlen(gbk.country));
	code_convert(l.area, 128, gbk.area, strlen(gbk.area));
#else
	GetIPLocation(ptr + 4,l);
#endif
	return l;
}

#include "regular.h"

bool CIPLocation::MatchRecord( char const * pRecord, const char *exp_country,const char * exp_area,
							  std::map<uint32_t,char*> &country_matched,std::map<uint32_t,char*> &area_matched )
{
	bool match;
	char const * parea = 0;
	char const * country;
	std::map<uint32_t,char*>::iterator it;

	// First , match the country field
	if (  exp_country[0] == '*' && exp_country[1]==0)
	{
		match = true;
	}else
	{
		switch (*pRecord)
		{
		case REDIRECT_MODE_1:
			return MatchRecord( m_file + ::Get3BYTE3( pRecord +1 ) , exp_country, exp_area, country_matched,area_matched);
		case REDIRECT_MODE_2:
			parea = pRecord + 4;

			it = country_matched.find(::Get3BYTE3(pRecord + 1));

			if ( it != country_matched.end())
			{
				match = true;
			}
			else
			{
				return false ;
// 				match = match_exp( Get_String( pRecord ) , exp_country);
// 				// update the matched country list
// 				if (match)
// 					country_matched.insert(std::pair<uint32_t,char*>(::Get3BYTE3(pRecord + 1),0));
			}
			break;
		default:
			country = Get_String(pRecord);
			match = match_exp( (char*)country, (char*)exp_country);
			if (match){
				country_matched.insert( std::pair<uint32_t,char*>( country - m_file , 0 ));

				parea = pRecord + strlen(country)+1;

			}else{
				return false;
			}
			
		}

	}

	// then , match the area field

	if (  exp_area[0] == '*' && exp_area[1]==0)
	{
		return true;
	}

	switch ( *parea )
	{
	case REDIRECT_MODE_2:
	case REDIRECT_MODE_1:
		it = area_matched.find(::Get3BYTE3( parea +1 ));

		if (  it  != area_matched.end() )
		{
			return true;
		}
		else
		{
			match = match_exp(Get_String(parea),(char*) exp_area);
			if ( match)
			{
				// update the matched area list
				area_matched.insert( std::pair<uint32_t,char*>(::Get3BYTE3( parea +1 ),0) );
			}else
			{
				return false;
			}
		}
		break;
	default:
		match = match_exp( Get_String(parea), (char*)exp_area);
		// update the matched area list
		area_matched.insert(  std::pair<uint32_t,char*>(  (parea - m_file ) & 0xFFFFFF ,0)  );

	}


	return match;
}

size_t CIPLocation::GetIPs(std::list<IP_regon> * retips, const char *_exp_country,
		const char * _exp_area)
{
	RECORD_INDEX * pindex;

	size_t i;
	bool match;
	std::map<uint32_t,char*> country_matched; // matched country
	std::map<uint32_t,char*> area_matched; // matched country


#ifndef _WIN32
	char exp_country[128];
	char exp_area[128];

	utf8_gbk(exp_country, 128,(char*) _exp_country, strlen(_exp_country));
	utf8_gbk(exp_area, 128, (char*) _exp_area, strlen(_exp_area));

#else
	#define exp_country _exp_country
	#define exp_area _exp_area
#endif

	for (i = m_first_record; i < m_last_record; i += 7)
	{
		pindex = (RECORD_INDEX*)(m_file + i );

		char const * pRecord = m_file + pindex->offset + 4;

		match = MatchRecord(pRecord, exp_country, exp_area, country_matched,area_matched);
		if (match)
		{
			IP_regon inst;
			inst.start.s_addr = htonl( pindex->ip );
			inst.end.s_addr = htonl( GetDWORD(pindex->offset));

			#ifndef _WIN32
				IPLocation gbk;
				GetIPLocation(pRecord, gbk);

				code_convert(inst.location.country, 128, gbk.country, strlen(gbk.country));
				code_convert(inst.location.area, 128, gbk.area, strlen(gbk.area));
			#else
				GetIPLocation(pRecord,inst.location);
			#endif
#ifdef DEBUG

			printf( "%s to ", inet_ntoa(inst.start));
			printf( "%s ,location : %s %s\n", inet_ntoa(inst.end),inst.location.country,inst.location.area);
#endif
			retips->insert(retips->end(), inst);

		}
	}
	return retips->size();
}
