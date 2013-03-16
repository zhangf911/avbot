
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>

#include "boost/connector.hpp"
#include "boost/base64.hpp"
#include "boost/coro/yield.hpp"
#include "pop3.hpp"

static inline std::string ansi_utf8(std::string const &source, const std::string &characters = "GB18030")
{
	return boost::locale::conv::between(source, "UTF-8", characters);
}

template<typename strIerator, typename Char>
static std::string endmark(strIerator & s,strIerator e, Char delim)
{
	std::string str;
	while( s !=  e && *s != delim){
		str.append(1,*s);
		s++;
	}
	return str;
}

static std::string base64inlinedecode(std::string str)
{
	std::string result;
	std::string::iterator p = str.begin();
	bool qout = false;
	while(p!=str.end())
	{
		if( *p == '=' && *(p+1)=='?') // 进入　base64 模式
		{
			p+=2;
			std::string charset = endmark(p,str.end(),'?');
			p+=3; // skip "?B?"
			std::string base64encoded = endmark(p,str.end(),'?');
			result.append(ansi_utf8(boost::base64_decode(base64encoded), charset));
			p+=2; // skip ?=
		}else if( *p=='\"'){
			p++;
		}else{
			result.append(1,*p);
			p++;
		}
	}
	return result;
}

static std::string find_charset(std::string contenttype)
{
	boost::cmatch what;
	//charset="xxx"
	std::size_t p =  contenttype.find("charset=");
	if( p != contenttype.npos )
	{
		std::string charset = contenttype.substr(p).c_str();
		boost::regex ex("charset=?(.*)?");
		if(boost::regex_match(charset.c_str(), what, ex))
		{
			std::string charset = what[1];
			boost::trim_if(charset,boost::is_any_of(" \""));
			return charset;
		}
	}
	return "UTF-8"; // default to utf8
}

static std::string find_mimetype(std::string contenttype)
{
	std::vector<std::string> splited;
	boost::split(splited, contenttype, boost::is_any_of("; "));
	return splited[0].empty()? splited[1]:splited[0];
}

// 有　text/plain　的就选　text/plain, 没的才选　text/html
static std::pair<std::string,std::string>
	select_best_mailcontent(mailcontent & thismail)
{
	typedef std::pair<std::string,std::string> mc;
	BOOST_FOREACH(mc &v, thismail.contents)
	{
		// 从 v.first aka contenttype 找到编码.
		std::string mimetype = find_mimetype(v.first);
		if( mimetype == "text/plain")
			return v;
	}
	BOOST_FOREACH(mc &v, thismail.contents)
	{
		// 从 v.first aka contenttype 找到编码.
		std::string mimetype = find_mimetype(v.first);
		if( mimetype == "text/html")
			return v;
	}
	return std::make_pair("","");
}

static void broadcast_signal(boost::shared_ptr<pop3::on_gotmail_signal> sig_gotmail, mailcontent thismail)
{
	(*sig_gotmail)(thismail);
}

static void decode_mail(boost::asio::io_service & io_service, boost::shared_ptr<pop3::on_gotmail_signal> sig_gotmail, mailcontent thismail)
{
 	std::cout << "邮件内容begin" << std::endl;

	thismail.from = base64inlinedecode( thismail.from );
	thismail.to = base64inlinedecode( thismail.to );
	thismail.subject = base64inlinedecode( thismail.subject );
	std::cout << "发件人:";
	std::cout << thismail.from ;
	std::cout << std::endl;

	std::pair<std::string,std::string> mc = select_best_mailcontent(thismail);

	{
		if(thismail.content_encoding == "base64"){
			// 从 v.first aka contenttype 找到编码.
			thismail.content = ansi_utf8(boost::base64_decode(mc.second), find_charset(mc.first));
		}else{ // 7bit decoding
			thismail.content = ansi_utf8(mc.second, find_charset(mc.first));
		}
	}
	std::cout << "邮件内容end" << std::endl;

	io_service.post(boost::bind(broadcast_signal,sig_gotmail,thismail));
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
					thismail.from = kv[1];
				} else if ( key == "to" ) {
					thismail.to = kv[1];
				} else if ( key == "subject" ) {
					thismail.subject = kv[1];
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
				} else if ( key == "content-transfer-encoding" ){
					thismail.content_encoding = val;
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
					thismail.contents.push_back(std::make_pair(contenttype,content));
					content.clear();
				}else if( state == 6 && line == std::string("--")+ boundary ){
					state = 5;
					thismail.contents.push_back(std::make_pair(contenttype,content));
					content.clear();
				}else if ( state == 6 && line == std::string("--") + boundary + "--")
				{
					state = 8;
					thismail.contents.push_back(std::make_pair(contenttype,content));
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
	io_service.post(boost::bind(decode_mail, boost::ref(io_service), m_sig_gotmail, thismail));
}

void pop3::operator() ( const boost::system::error_code& ec, std::size_t length )
{
	using namespace boost::asio;

	ip::tcp::endpoint endpoint;
	std::string		status;
	std::string		maillength;
	std::istream	inbuffer ( m_streambuf.get() );
	std::string		msg;

	reenter ( this ) {
restart:
		m_socket.reset( new ip::tcp::socket(io_service) );

		do {
			// 延时 60s
			_yield ::boost::delayedcallsec( io_service, 60, boost::bind(*this, ec, 0) );

			// dns 解析并连接.
			_yield boost::async_connect(*m_socket, ip::tcp::resolver::query ( "pop.qq.com", "110" ), *this);

			// 失败了延时 10s
			if ( ec )
				_yield ::boost::delayedcallsec ( io_service, 10, boost::bind(*this, ec, 0) );
		} while ( ec ); // 尝试到连接成功为止!

		i = 0;

		// 好了，连接上了.
		m_streambuf.reset ( new streambuf );
		// "+OK QQMail POP3 Server v1.0 Service Ready(QQMail v2.0)"
		_yield	async_read_until ( *m_socket, *m_streambuf, "\n", *this );
		inbuffer >> status;

		if ( status != "+OK" ) {
			// 失败，重试.
			goto restart;
		}

		// 发送用户名.
		_yield m_socket->async_write_some ( buffer ( std::string ( "user " ) + m_user + "\n" ), *this );
		if(ec) goto restart;
		// 接受返回状态.
		m_streambuf.reset ( new streambuf );
		_yield	async_read_until ( *m_socket, *m_streambuf, "\n", *this );
		inbuffer >> status;

		// 解析是不是　OK.
		if ( status != "+OK" ) {
			// 失败，重试.
			goto restart;
		}

		// 发送密码.
		_yield m_socket->async_write_some ( buffer ( std::string ( "pass " ) + m_passwd + "\n" ), *this );
		// 接受返回状态.
		m_streambuf.reset ( new streambuf );
		_yield	async_read_until ( *m_socket, *m_streambuf, "\n", *this );
		inbuffer >> status;

		// 解析是不是　OK.
		if ( status != "+OK" ) {
			// 失败，重试.
			goto restart;
		}

		// 完成登录. 开始接收邮件.

		// 发送　list 命令.
		_yield m_socket->async_write_some ( buffer ( std::string ( "list\n" ) ), *this );
		// 接受返回的邮件.
		m_streambuf.reset ( new streambuf );
		_yield	async_read_until ( *m_socket, *m_streambuf, "\n", *this );
		inbuffer >> status;

		// 解析是不是　OK.
		if ( status != "+OK" ) {
			// 失败，重试.
			goto restart;
		}

		// 开始进入循环处理邮件.
		maillist.clear();
		_yield	m_socket->async_read_some ( m_streambuf->prepare ( 8192 ), *this );
		m_streambuf->commit ( length );

		while ( status != "." ) {
			maillength.clear();
			status.clear();
			inbuffer >> status;
			inbuffer >> maillength;

			// 把邮件的编号push到容器里.
			if ( maillength.length() )
				maillist.push_back ( status );

			if ( inbuffer.eof() && status != "." )
				_yield	m_socket->async_read_some ( m_streambuf->prepare ( 8192 ), *this );
		}

		// 获取邮件.
		while ( !maillist.empty() ) {
			// 发送　retr #number 命令.
			msg = boost::str ( boost::format ( "retr %s\r\n" ) %  maillist[0] );
			_yield m_socket->async_write_some ( buffer ( msg ), *this );
			// 获得　+OK
			m_streambuf.reset ( new streambuf );
			_yield	async_read_until ( *m_socket, *m_streambuf, "\n", *this );
			inbuffer >> status;

			// 解析是不是　OK.
			if ( status != "+OK" ) {
				// 失败，重试.
				goto restart;
			}

			// 获取邮件内容，邮件一单行的 . 结束.
			_yield	async_read_until ( *m_socket, *m_streambuf, "\r\n.\r\n", *this );
			// 然后将邮件内容给处理.
			process_mail ( inbuffer );
			// 删除邮件啦.
			msg = boost::str ( boost::format ( "dele %s\r\n" ) %  maillist[0] );
			_yield m_socket->async_write_some ( buffer ( msg ), *this );

			maillist.erase ( maillist.begin() );
			// 获得　+OK
			m_streambuf.reset ( new streambuf );
			_yield	async_read_until ( *m_socket, *m_streambuf, "\n", *this );
			inbuffer >> status;

			// 解析是不是　OK.
			if ( status != "+OK" ) {
				// 失败，但是并不是啥大问题.
				std::cout << "deleting mail failed" << std::endl;
				// but 如果是连接出问题那还是要重启的.
				if(ec) goto restart;
			}
		}

		// 处理完毕.
		_yield async_write ( *m_socket, buffer ( "quit\n" ), *this );
		_yield ::boost::delayedcallsec ( io_service, 1, boost::bind ( *this, ec, 0 ) );
		if(m_socket->is_open())
			m_socket->shutdown ( ip::tcp::socket::shutdown_both );
		_yield ::boost::delayedcallsec ( io_service, 1, boost::bind ( *this, ec, 0 ) );
		m_socket.reset();
		std::cout << "邮件处理完毕" << std::endl;
		_yield ::boost::delayedcallsec ( io_service, 30, boost::bind ( *this, ec, 0 ) );
		goto restart;
	}
}

