// IPLocation.h: interface for the CIPLocation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IPLOCATION_H__80D0E230_4815_4D01_9CCF_4DAF4DE175E8__INCLUDED_)
#define AFX_IPLOCATION_H__80D0E230_4815_4D01_9CCF_4DAF4DE175E8__INCLUDED_

#include <map>
#include <list>

#ifdef _WIN32

#include <windows.h>
typedef UINT32	uint32_t;
#else
#include <stdint.h>
#include <netinet/in.h>
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

struct IP_regon{
	in_addr start;
	in_addr end;
	IPLocation	location;
};

class CIPLocation
{
protected: // protected member
	uint32_t m_filesize;
	char const * m_file;
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
	IPLocation & GetIPLocation(char const *ptr, IPLocation &);
	char* Get_String(char const * p);
	char* GetArea(char * record);
	char* GetCountry(char * record);
	char const * FindRecord(in_addr ip);
	char* read_string(size_t offset);
	bool MatchRecord(char const * pRecord, const char *exp_country,const char * exp_area, std::map<uint32_t,char*> &country_matched,std::map<uint32_t,char*> &area_matched);
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

public:
	//************************************
	// Method:    GetIPLocation
	// FullName:  CIPLocation::GetIPLocation
	// Access:    public
	// Returns:   IPLocation
	// Parameter: in_addr ip
	//************************************
	IPLocation & GetIPLocation(in_addr ip,IPLocation &);

	size_t GetIPs(std::list<IP_regon> * retips, const char *exp_country, const char * exp_area);
public:
	CIPLocation(char*	memptr, size_t len);
	CIPLocation(const char*	ipDateFile);
	~CIPLocation();
protected:

};

#endif // !defined(AFX_IPLOCATION_H__80D0E230_4815_4D01_9CCF_4DAF4DE175E8__INCLUDED_)
