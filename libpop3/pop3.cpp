#include <boost/regex.hpp>

#include "pop3.hpp"

void pop3::process_mail ( std::istream& mail )
{
	int state = 0;
	mailcontent thismail;
	std::string	contenttype;
	std::string content;

	// 以行为单位解析
	while ( !mail.eof() ) {
		std::string line;
		std::string key, val;
		std::vector<std::string> kv;
		std::getline ( mail, line );
		std::stringstream	linestream ( line );

		switch ( state ) {
			case 0:
			case 2:

				if ( line == "\r" ) {
					state = 3;
					break;
				}

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
						state = 1;
						//进入　boundary 设定.
					} else {
						// 记录 content-type
						//thismail.content.push_back(std::make_pair(""));
						contenttype = val;
						state = 2;
					}
				}

				break;
			case 1:
				//boundary=XXX
				break;
			case 3:

				//读取 content.
				if ( line != ".\r" )
					content += line;
		}
	}

	std::cout << "邮件内容begin" << std::endl;
	std::cout << ::boost::asio::buffer_cast<const char*> ( m_streambuf->data() ) << std::endl;
	std::cout << "邮件内容end" << std::endl;
}
