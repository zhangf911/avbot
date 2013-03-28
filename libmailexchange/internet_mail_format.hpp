
#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <istream>
#include <boost/variant.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/locale.hpp>

#include "boost/base64.hpp"

// that is for output to main.cpp
struct mailcontent{
	std::string		from;
	std::string		to;
	std::string		subject;
	std::string		content_type;                                 // without encoding part
	std::string		content;                                      // already decoded to UTF-8, and the best selected content
};

struct InternetMailFormat;
// MIME 格式的邮件内容.
typedef std::vector<InternetMailFormat> MIMEcontent;

// Internet Mail Format 简化格式.
struct InternetMailFormat{
	// 邮件头部.
	std::map<std::string, std::string> header;
	bool have_multipart;

	// 头部结束了就是 body了，如果是 MIME 消息，请从 body 构造 MIMEcontent
// 	std::string body;
	boost::variant<std::string, MIMEcontent> body;
};

namespace detail{

inline std::string ansi_utf8(std::string const &source, const std::string &characters = "GB18030")
{
	return boost::locale::conv::between(source, "UTF-8", characters);
}


inline std::string imf_base64inlinedecode(std::string str)
{
	boost::cmatch what;
	boost::regex ex("=\\?(.*)?\\?=");
	if (boost::regex_search(str.c_str(), what, ex))
	{
		std::string matched_encodedstring;
		matched_encodedstring = what[0];
		ex.set_expression("(.*)\\?(.)\\?(.*)");
		if(boost::regex_search(what[1].str().c_str(), what, ex))
		{
			std::string result, encode, charset;
			charset = what[1];	// 	gb18030
			encode = what[2]; //  B
	   
			if (encode =="B" )
			{
				result = ansi_utf8(boost::base64_decode(what[3].str()), charset);
				boost::replace_all(str, matched_encodedstring, result);
			}
		}
	}
	return str;
}

inline std::pair<std::string, std::string> process_line(const std::string & line)
{
	boost::regex ex("([^:]*)?:[ \t]+(.*)?");
	boost::cmatch what;
	if (boost::regex_match(line.c_str(), what, ex, boost::match_all))
	{
		std::string key = what[1];
		std::string val = what[2];
		boost::to_lower(key);
		return std::make_pair(key, imf_base64inlinedecode(val));
	}
	throw(boost::bad_expression("not matched"));
}
}

// 从 string 构造一个 MIMEcontent
template<class InputStream>
void mime_read_stream(MIMEcontent& mimecontent, InputStream &in,const std::string &boundary);

// 从 in stream 构造一个 InternetMailFormat
template<class InputStream>
void imf_read_stream(InternetMailFormat& imf, InputStream &in);

inline void imf_decode_multipart(InternetMailFormat &imf);

// 从 in stream 构造一个 InternetMailFormat
template<class InputStream>
void imf_read_stream(InternetMailFormat& imf, InputStream &in)
{
	std::string line;
	std::string pre_line;
	std::string body;
	imf.have_multipart = false;

	std::getline(in, pre_line);
    // 要注意处理 folding
    // 状态机开始!
    // 以行为单位解析
	while ( !in.eof() ){
		line.clear();

		std::getline(in, line);
		boost::trim_right(line);
		if ( !line.empty() && boost::is_space()(line[0])) // 开头是 space ,  则是 folding
		{
			boost::trim_left(line);
			pre_line += line;
			continue;
		} else {
			if (!pre_line.empty()) {

				// 之前是 fold  的，所以 pre_line 才是正确的一行.
				try {
					imf.header.insert(detail::process_line(pre_line));
				} catch (const boost::bad_expression & e)
				{}
			}else if (pre_line.empty() && !line.empty() ){
				// 应该是 body 模式了.
				body = line + "\r\n";
				while (!in.eof()){
					line.clear();
					std::getline(in, line);
					body += line + "\n";
				}
				// 依据 content-franster-encoding 解码 body 
				std::string content_transfer_encoding = imf.header["content-transfer-encoding"];
				if (boost::to_lower_copy(content_transfer_encoding) == "base64")
				{
					imf.body = boost::base64_decode(body);
				}else{
					imf.body = body;
				}
				imf_decode_multipart(imf);
				return;
			}
			pre_line =  line;
		}
	}
}

// 从 string 构造一个 MIMEcontent
template<class InputStream>
void mime_read_stream(MIMEcontent& mimecontent, InputStream &in, const std::string & boundary)
{
	// 以 boundary 为分割线，完成后的分割，又可以 InternetMailFormat 的格式解析
	// 等待进入 multipart 部分.
	// 以行模式进行解析
	while (!in.eof())
	{
		std::string line;
		std::getline(in, line);
		
		if (boost::trim_copy(line) == std::string("--")+ boundary)
		{
			std::string part;
			// 进入 multipart 部分.
			while (!in.eof())
			{
				std::string line;
				std::getline(in, line);
				if ((boost::trim_copy(line) == std::string("--") + boundary)||(boost::trim_copy(line) == std::string("--") + boundary + "--"))
				{
					// 退出 multipart
					// part 就是一个 IMF 格式了
					std::stringstream partstream(part);
					InternetMailFormat imf;
					imf_read_stream(imf, partstream);
					mimecontent.push_back(imf);
					part.clear();
					continue;
				}
				part += (line + "\n");
			}
		}
	}
}

inline void imf_decode_multipart(InternetMailFormat &imf)
{
	std::string content_type = imf.header["content-type"];

	boost::cmatch what;
	boost::regex ex("multipart/.*?;.*?boundary=\"(.*)?\"");
	if (boost::regex_search(content_type.c_str(), what, ex ))
	{
		imf.have_multipart = true;
		std::string boundary = what[1];
		MIMEcontent mime;

		std::stringstream bodystream(boost::get<std::string>(imf.body));
		mime_read_stream(mime, bodystream, boundary);
		imf.body = mime;
	}
	return;
}

