
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>

#include "boost/base64.hpp"
#include "boost/coro/yield.hpp"

#include "pop3.hpp"

static std::string find_mimetype(std::string contenttype)
{
	std::vector<std::string> splited;
	boost::split(splited, contenttype, boost::is_any_of("; "));
	return splited[0].empty()? splited[1]:splited[0];
}

// 有　text/plain　的就选　text/plain, 没的才选　text/html
static std::pair<std::string,std::string>
	select_best_mailcontent(MIMEcontent mime)
{
 	BOOST_FOREACH(InternetMailFormat &v, mime)
 	{
 		// 从 v.first aka contenttype 找到编码.
 		std::string mimetype = find_mimetype(v.header["content-type"]);
 		if( mimetype == "text/plain"){
			return std::make_pair(v.header["content-type"], boost::get<std::string>(v.body));
		}
 	}
 	BOOST_FOREACH(InternetMailFormat &v, mime)
 	{
 		// 从 v.first aka contenttype 找到编码.
 		std::string mimetype = find_mimetype(v.header["content-type"]);
 		if( mimetype == "text/html")
 			return std::make_pair(v.header["content-type"], boost::get<std::string>(v.body));
 	}
 	return std::make_pair("","");
}

static void broadcast_signal(boost::shared_ptr<pop3::on_gotmail_signal> sig_gotmail, mailcontent thismail, boost::function<void()> handler)
{
	(*sig_gotmail)(thismail);
	handler();
}

static std::string decode_content_charset(std::string body, std::string content_type)
{
	boost::cmatch what;
	boost::regex ex("(.*)?;[\t \r\a]?charset=(.*)?");
	if (boost::regex_search(content_type.c_str(), what,  ex))
	{
		// text/plain; charset="gb18030" 这种，然后解码成 UTF-8
		std::string charset = what[2];
		return detail::ansi_utf8(body, charset);
	}
	return body;
}

static void decode_mail(boost::asio::io_service & io_service, InternetMailFormat imf, boost::shared_ptr<pop3::on_gotmail_signal> sig_gotmail, boost::function<void()> handler)
{
	std::cout << "邮件内容begin" << std::endl;
	
	std::cout << "发件人:";
	std::cout << imf.header["from"];
	std::cout << std::endl;
	
	mailcontent thismail;
	thismail.from = imf.header["from"];
	thismail.to = imf.header["to"];
	thismail.subject = imf.header["subject"];

	if (!imf.have_multipart){
		thismail.content_type = find_mimetype(imf.header["content-type"]);
		thismail.content = decode_content_charset(boost::get<std::string>(imf.body), imf.header["content-type"]);
	}else{
		// base64 解码已经处理，都搞定了，接下来是从 multipart 里选一个.
		std::pair<std::string,std::string> mc = select_best_mailcontent(boost::get<MIMEcontent>(imf.body));
		thismail.content_type = find_mimetype(mc.first);
		thismail.content = decode_content_charset(mc.second, mc.first);
	}

	std::cout << "邮件内容end" << std::endl;
 	io_service.post(boost::bind(broadcast_signal,sig_gotmail,thismail, handler));
}

template<class Handler>
void pop3::process_mail(std::istream &mail, Handler handler)
{
	InternetMailFormat imf;
	imf_read_stream(imf, mail);
	// 解读 InternetMailFormat
	// 处理　base64 编码的邮件内容.
	io_service.post(boost::bind(&decode_mail, boost::ref(io_service), imf, m_sig_gotmail, boost::function<void()>(handler)));
	return ;
}

pop3::pop3(boost::asio::io_service& _io_service, std::string user, std::string passwd, std::string _mailserver) :io_service(_io_service),
    m_mailaddr(user), m_passwd(passwd),
    m_mailserver(_mailserver),
    m_sig_gotmail(new on_gotmail_signal())
{
    if(m_mailserver.empty()) // 自动从　mailaddress 获得.
    {
        if( m_mailaddr.find("@") == std::string::npos)
            m_mailserver = "pop.qq.com"; // 如果　邮箱是 qq 号码（没@），就默认使用 pop.qq.com .
        else
            m_mailserver =  std::string("pop.") + m_mailaddr.substr(m_mailaddr.find_last_of("@")+1);
    }
    io_service.post(boost::asio::detail::bind_handler(*this, boost::system::error_code(), 0));
}

