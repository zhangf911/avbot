// IPLocation.cpp: implementation of the CIPLocation class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "IPLocation.h"

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
		open(ipDateFile.c_str(),O_RDONLY);
	#endif // _WIN32
		
		if (  m_ipfile == (FILE_HANDLE)(-1) )
		{
			throw std::string("File Open Failed!");			
		}
		
	#ifdef _WIN32
		m_filesize = GetFileSize(m_ipfile,0);
	#else
		stat fst;
		fstat(m_ipfile,&fst);
		m_filesize=fst.st_size;
	#endif // _WIN#32
		
	#ifdef _WIN32
		m_filemap =  CreateFileMapping(m_ipfile,0,PAGE_READONLY,0,m_filesize,0);

		if ( !m_filemap )
		{
			throw std::string("CreatFileMapping Failed!");
		}

		m_file = (char*)MapViewOfFile(m_filemap,FILE_MAP_READ,0,0,m_filesize);
	#else
		//TODO
		m_file = mmap(0,0,0,0);
	#endif
		if (!m_file)
		{
			throw std::string("Creat File Mapping Failed!");
		}
#ifndef _WIN32
		close(m_ipfile);
#endif // _WIN32
		m_curptr = m_file;
		m_first_record = * (uint32_t*)m_file ;
		m_last_record = * (uint32_t*)(m_file + 4);

}

CIPLocation::~CIPLocation()
{
#ifdef _WIN32
	UnmapViewOfFile( m_file);
	CloseHandle(m_filemap);
	CloseHandle(m_ipfile);
#else
	munmap(m_file); 
#endif // _WIN32
}

char* CIPLocation::read_string(size_t offset)
{
	char * ptr = m_file + offset;
	return ptr;
}


#pragma pack(1)
struct redirect_mode1{
	u_char		mode;
	unsigned	offset:24;	
};
#define REDIRECT_MODE_1 1

struct redirect_mode2{
	u_char		mode;
	unsigned	ccountry_offset:24;
	char		area[1];
};
#define REDIRECT_MODE_2 2
#pragma pack()

char * CIPLocation::FindRecord(in_addr ip)
{
	uint32_t	realip;
	realip = ip.s_addr;
	for ( size_t i = m_first_record ; i < m_last_record ; i+=4)
	{
		if (  realip >=  ntohl(GetDWORD(i)) && realip <= ntohl(GetDWORD(Get3BYTE3(i+4))))
		{
			return m_file + i;
		}
	}
	return 0;
}

IPLocation CIPLocation::GetIPLocation(in_addr ip)
{
	IPLocation	ret;

	char * ptr = FindRecord(ip);
	if (!ptr)
	{
		throw std::string("Not Found");
	}
	ret.country = Get_String(ptr + 4);

	if ( ptr[4] )
	{


	}
		
			
	
	
	return ret;
}


char * CIPLocation::Get_String(char *p)
{
	switch ( p[0] )
	{
	case REDIRECT_MODE_1:
		return Get_String(m_file  + Get3BYTE3(p +1));
		break;
	case REDIRECT_MODE_2:
		return m_file + Get3BYTE3(p+1);
		break;
	default:
		return p;
	}
}
