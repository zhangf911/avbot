// IPLocation.h: interface for the CIPLocation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IPLOCATION_H__80D0E230_4815_4D01_9CCF_4DAF4DE175E8__INCLUDED_)
#define AFX_IPLOCATION_H__80D0E230_4815_4D01_9CCF_4DAF4DE175E8__INCLUDED_

#include "defs.h"	// Added by ClassView
struct IPLocation
{
	std::string country;
	std::string area;
};

class CIPLocation
{
protected:
	uint32_t m_filesize;
	char* m_file;
	char* m_curptr;
	size_t m_first_record;
	size_t m_last_record;

protected:
	char* Get_String(char * p, char*out);
	char* GetArea(char * record);
	char* GetCountry(char * record);
	uint32_t inline GetDWORD(size_t offset)
	{
		uint32_t ret;
		memcpy(&ret, m_file + offset, 4);
		return to_hostending(ret);
	}

	uint32_t inline Get3BYTE3(size_t offset)
	{
		uint32_t ret = 0;
		memcpy(&ret, m_file + offset, 3);
		return to_hostending(ret);
	}
	uint32_t inline Get3BYTE3(char * var_ptr)
	{
		uint32_t ret = 0;
		memcpy(&ret, var_ptr, 3);
		return to_hostending(ret);
	}

	char * FindRecord(in_addr ip);
	char* read_string(size_t offset);
#ifdef _WIN32
	HANDLE m_filemap;
	HANDLE m_ipfile;
#endif // _WIN32
public:
	IPLocation GetIPLocation(in_addr ip);
	IPLocation GetIPLocation(char *ptr);

	CIPLocation(std::string ipDateFil);
	virtual ~CIPLocation();
};

#endif // !defined(AFX_IPLOCATION_H__80D0E230_4815_4D01_9CCF_4DAF4DE175E8__INCLUDED_)
