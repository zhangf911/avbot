#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>

#include "pop3.hpp"

static void decode_mail(mailcontent thismail)
{
 	std::cout << "邮件内容begin" << std::endl;
	
	typedef std::pair<std::string,std::string> mc;
	BOOST_FOREACH(mc &v, thismail.content)
	{
		std::cout << v.second << std::endl;
	}
 	std::cout << "邮件内容end" << std::endl;
}

void pop3::process_mail ( std::istream& mail )
{
	int state = 0;
	mailcontent thismail;
	std::string	contenttype;
	std::string content;
	std::string boundary;

	std::string line;
	std::getline ( mail, line );
	boost::trim_right(line);

	// 以行为单位解析
	while ( !mail.eof() ) {
		boost::trim_right(line);
		std::string key, val;
		std::vector<std::string> kv;
		std::stringstream	linestream ( line );

		switch ( state ) {
			case 0: // 忽略起始空白.
				if ( line != "" ) {
					state = 1;
				}else
					break;
			case 1: // 解析　　XXX:XXX　文件头选项对.
			case 5: // MIME 子头，以 ------=_NextPart_51407E7E_09141558_64FF918C 的形式起始.
				if (line == "" ){ // 空行终止文件头.
					if( state == 1 )
						if(boundary.empty()){ //进入非MIME格式的邮件正文处理.
							state = 3;
						}else{
							state = 4; //正文为　MIME , 进入　MIME 模式子头处理.
						}
					else {
						state = 6; // MIME 模式的子body处理
					}
					break;
				}
				if( state == 5 && line == std::string("--") + boundary + "--"){
					state = 7 ; //MIME 结束.
				}
				//　进入解析.
				boost::split ( kv, line, boost::algorithm::is_any_of ( ":" ) );
				key = kv[0];
				val = kv[1];
				boost::to_lower ( key );
				boost::to_lower ( val );

				if ( key == "from" ) {
					thismail.from = val;
				} else if ( key == "to" ) {
					thismail.to = val;
				} else if ( key == "subject" ) {
					thismail.subject = val;
				} else if ( key == "content-type" ) {
					// 检查是不是　MIME　类型的.
					if ( boost::trim_copy ( val ) == "multipart/alternative;" ) {
						//是　MIME 类型的.
						state = 2;
						//进入　boundary 设定.
					} else {
						// 记录 content-type
						contenttype = val;
						if( state != 1){
							state = 7;
						}else{
							state = 1;
						}
					}
				}

				break;
			case 2:
			{
				using namespace boost;
				cmatch what;
				if(regex_match(line.c_str(),what,regex("\\tboundary=?\"(.*)?\"")))
				{
					boundary = what[1];
				}
			}
			state = 1;
			break;

			case 3: // 是　multipart 的么?
			case 6:
				if(state==3 && line == "."){
					state = 0;
					thismail.content.push_back(std::make_pair(contenttype,content));
					content.clear();
				}else if( state == 6 && line == std::string("--")+ boundary ){
					state = 5;
					thismail.content.push_back(std::make_pair(contenttype,content));
					content.clear();
				}else if ( state == 6 && line == std::string("--") + boundary + "--")
				{
					state = 8;
					thismail.content.push_back(std::make_pair(contenttype,content));
					content.clear();
				}else{
					//读取 content.
					content += line;
				}
				break;
			case 4: // 如果是　4 , 则是进入 MIME 的子头处理.
				// 查找 boundary
				if(line == std::string("--")+ boundary )
				{
					state = 5;
				}
				break;
			case 7: // charset = ??
				contenttype += line;
				state = 5;
				break;
			case 8:
				break;
		}
		line.clear();
		std::getline ( mail, line );
	}
	// 处理　base64 编码的邮件内容.
	io_service.post(boost::bind(decode_mail, thismail));
}


