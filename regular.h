
class Xstring
{
protected:
	char*	m_data;
protected:
	void inline Allocate()
	{
		m_data = (char*)malloc( length +1);
	}
public:
	Xstring()
	{
		length = 64;
		Allocate();
	}
	Xstring( const  char* begin, const char * end )
	{
		length = end - begin;
		Allocate();
		memcpy(m_data,begin,length);
		m_data[length] = 0;
	}
	~Xstring()
	{
		free(m_data);
	}
	inline operator const char * ()
	{
		return (const char*)m_data;
	}
	Xstring & operator =( const Xstring & str)
	{
		length = str.length;
		Allocate();
		memcpy(m_data,str.m_data,length);
		m_data[length] = 0;
		return * this;
	}
public:
	size_t	length;
};

static bool searchMatch(unsigned int& i, unsigned int& it, const char* str, const char *cstr)
{
    size_t si, j;
	size_t len_str;

	len_str = strlen(str);

    for(si=i; cstr[si]!=NULL; si++)
	{
        for( it=si,j=0; (j < len_str)  && cstr[it]!=NULL && ( '?' == str[j] || (cstr[it] == str[j] ) ); j++)
			it++;
        if(j>= len_str)
		{
            i=si;
            return true;
        }
    }
    return false;
}
int iii=0;

static bool match_exp(char str[], char exp[])
{
	iii++;
	
    /// 空串处理，空串或者 “**...*”可以匹配空串
    if(str[0]==NULL)
	{
        int j;
        for(j=0; exp[j] && exp[j]=='*';)
            j++;
        return exp[j]==NULL;
    }else if(exp[0]==NULL)
        return false;
    /// 如果 exp 不以 * 开始，则先匹配开始部分
    unsigned i, j, it, jt;
    for(i=j=0; str[i]!=NULL&&exp[j]!=NULL&&exp[j]!='*'; i++,j++)
	{
        if(exp[j]!='?' && str[i]!=exp[j])
            return false;
    }
    if(str[i]==NULL)
	{
        for(; exp[j] && exp[j]=='*';)
            j++;
        return exp[j]==NULL;
    }else if(exp[j]==NULL)
        return false;
    /// 扫描 以 * 开始的串，进行匹配
	Xstring lastSubstr;
    for(jt=j; exp[jt]!=NULL ; i=it,j=jt)
	{ 
        for(; exp[j]!=NULL && exp[j]=='*';)
            j++;
        if(exp[j]==NULL)
            break;
        for(jt=j; exp[jt]!=NULL && exp[jt]!='*';)
            jt++;
        
		Xstring tmp(&exp[j],&exp[jt]);
		
        if(!searchMatch(i,it,tmp,str))
            return false;
        lastSubstr = tmp;
    }
    /// 最后一个子串（exp中的）处理。
    if(exp[j-1]=='*')
        return true;
    for(i=strlen(str)- lastSubstr.length,j=0; j<lastSubstr.length; i++,j++)
        if(lastSubstr[j]!='?' && str[i]!=lastSubstr[j])
            return false;
		return true;
}