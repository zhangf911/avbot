// IPLocation.h: interface for the CIPLocation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IPLOCATION_H__80D0E230_4815_4D01_9CCF_4DAF4DE175E8__INCLUDED_)
#define AFX_IPLOCATION_H__80D0E230_4815_4D01_9CCF_4DAF4DE175E8__INCLUDED_

#include <list>

#ifdef _WIN32

#include <windows.h>
typedef UINT32	uint32_t;

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

struct IPLocation
{
	char country[128];
	char area[128];
};

class CIPLocation
{
protected: // protected member
	uint32_t m_filesize;
	char* m_file;
	char* m_curptr;
	size_t m_first_record;
	size_t m_last_record;

#ifdef _WIN32
	HANDLE	m_filemap;
	HANDLE	m_ipfile;
#else
	bool   m_backup_byfile;
#endif // _WIN32

protected:
	IPLocation GetIPLocation(char *ptr);
	char* Get_String(char * p, char*out);
	char* GetArea(char * record);
	char* GetCountry(char * record);
	char* FindRecord(in_addr ip);
	char* read_string(size_t offset);
protected: //inline functions

	uint32_t inline GetDWORD(size_t offset)
	{
		uint32_t ret;
		ret = * ( uint32_t * )(m_file + offset);
		return to_hostending(ret);
	}

	uint32_t inline Get3BYTE3(size_t offset)
	{
		uint32_t ret = 0;
		ret = * ( uint32_t * )(m_file + offset);
		ret &= 0xFFFFFF;
		return to_hostending(ret);
	}
	uint32_t inline Get3BYTE3(char * var_ptr)
	{
		uint32_t ret = 0;
		memcpy(&ret, var_ptr, 3);
		return to_hostending(ret);
	}


public:
	//************************************
	// Method:    GetIPLocation
	// FullName:  CIPLocation::GetIPLocation
	// Access:    public 
	// Returns:   IPLocation
	// Parameter: in_addr ip
	//************************************
	IPLocation GetIPLocation(in_addr ip);	
	size_t GetIPs( std::list<int> * retips,char *exp);
public:
	CIPLocation(char*	memptr, size_t len);
	CIPLocation(char*	ipDateFile);
	~CIPLocation();
};

#endif // !defined(AFX_IPLOCATION_H__80D0E230_4815_4D01_9CCF_4DAF4DE175E8__INCLUDED_)
