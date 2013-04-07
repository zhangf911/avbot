
#pragma once

#if defined(_MSC_VER) && _MSC_VER > 1310
// Visual C++ 2005 and later require the source files in UTF-8, and all strings 
// to be encoded as wchar_t otherwise the strings will be converted into the 
// local multibyte encoding and cause errors. To use a wchar_t as UTF-8, these 
// strings then need to be convert back to UTF-8. This function is just a rough 
// example of how to do this.
# define utf8str(str)  ConvertToUTF8(L##str)
inline const char * ConvertToUTF8(const wchar_t * pStr) {
    __declspec(thread) static char szBuf[8192];
    WideCharToMultiByte(CP_UTF8, 0, pStr, -1, szBuf, sizeof(szBuf), NULL, NULL);
    return szBuf;
}

# define localstr(str) ConvertToLocal(L##str)
inline const char * ConvertToLocal(const wchar_t * pStr) {
    __declspec(thread) static char szBuf[8192];
    WideCharToMultiByte(CP_ACP, 0, pStr, -1, szBuf, sizeof(szBuf), NULL, NULL);
    return szBuf;
}
#else
// Visual C++ 2003 and gcc will use the string literals as is, so the files 
// should be saved as UTF-8. gcc requires the files to not have a UTF-8 BOM.
# define utf8str(str)  str
# define localstr(str)  str
#endif