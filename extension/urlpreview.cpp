#include <algorithm>
#include <boost/date_time/time_duration.hpp>

#include "urlpreview.hpp"
#include <boost/algorithm/string.hpp>

namespace detail
{

static inline bool is_html( const std::string contenttype )
{
	if( contenttype.empty() )
		return false;

	if( contenttype == "text/html" )
		return true;

	if( contenttype.find( "text/html" ) != std::string::npos )
		return true;
	return false;
}

static inline std::string get_char_set( std::string type,  const std::string & header )
{
	boost::cmatch what;
	// 首先是 text/html; charset=XXX
	boost::regex ex( "charset=([a-zA-Z0-9\\-]+)" );
	boost::regex ex2( "<meta charset=([a-zA-Z0-9]+)\"?>" );

	if( boost::regex_search( type.c_str(), what, ex ) )
	{
		return what[1];
	}
	else if( boost::regex_search( type.c_str(), what, ex2 ) )
	{
		return what[1];
	}

	return "utf8";
}

inline std::size_t read_until_title(boost::system::error_code ec, std::size_t bytes_transferred, std::size_t max_transfer, boost::asio::streambuf & buf)
{
	if (ec)
		return 0;
	// 有 </title> 就不读了, 或则已经读取到 4096 个字节也就不读了
	// 或则已经读到 content_length 也不读了.
	if (bytes_transferred >= max_transfer)
		return 0;
	std::string data(boost::asio::buffer_cast<const char*>(buf.data()), boost::asio::buffer_size(buf.data()));
	boost::to_lower( data );
	if (data.find("</title>")==std::string::npos){
		return max_transfer - bytes_transferred;
	}
	if (data.find("<meta>")==std::string::npos){
		return max_transfer - bytes_transferred;
	}

	return 0;
}

static std::string html_unescape_char(std::string escaped)
{
	//	boost::replace_all( htmlcharseq, "&nbsp;", " " );
	if (escaped[0] == '#'){
		// &#XX; 这样的模式
		// strtol
		std::wstring unistr;
		unistr.append(1, strtol(&escaped[1], NULL, 10));

		return boost::locale::conv::utf_to_utf<char>(unistr);

	}else{
		if (escaped== "nbsp")
			return " ";
		if (escaped== "mdash")
			return "-";
		if (escaped == "quot")
			return "\"";
		if (escaped == "amp")
			return "&";
		if (escaped == "lt")
			return "<";
		if (escaped == "gt")
			return ">";
#ifndef _MSC_VER	// 处理
		if (escaped== "ndash")
			return "–";
		if (escaped == "euro")
			return "€";
		if (escaped == "sup1")
			return "¹";
		if (escaped == "sup2")
			return "²";
		if (escaped == "sup3")
			return "³";
		if (escaped == "iexcl")
			return "¡";
		if (escaped == "cent")
			return "¢";
		if (escaped == "pound")
			return "£";
		if (escaped == "curren")
			return "¤";
		if (escaped == "yen")
			return "¥";
		if (escaped == "brvbar")
			return "¦";
		if (escaped == "sect")
			return "§";
		if (escaped == "uml")
			return "¨";
		if (escaped == "copy")
			return "©";
		if (escaped == "ordf")
			return "ª";
		if (escaped == "not")
			return "¬";
		if (escaped == "reg")
			return "®";
		if (escaped == "macr")
			return "¯";
		if (escaped == "deg")
			return "°";
		if (escaped == "plusmn")
			return "±";
		if (escaped == "acute")
			return "´";
		if (escaped == "micro")
			return "µ";
		if (escaped == "para")
			return "¶";
		if (escaped == "middot")
			return "·";
		if (escaped == "cedil")
			return "¸";

		if (escaped == "ordm")
			return "º";
		if (escaped == "raquo")
			return "»";
		if (escaped == "frac14")
			return "¼";
		if (escaped == "frac12")
			return "½";
		if (escaped == "frac34")
			return "¾";
		if (escaped == "iquest")
			return "¿";
		if (escaped == "Agrave")
			return "À";
		if (escaped == "Aacute")
			return "Á";
		if (escaped == "Acirc")
			return "Â";
		if (escaped == "Atilde")
			return "Ã";
		if (escaped == "Auml")
			return "Ä";
		if (escaped == "Aring")
			return "Å";
		if (escaped == "AElig")
			return "Æ";
		if (escaped == "Ccedil")
			return "Ç";
		if (escaped == "Egrave")
			return "È";
		if (escaped == "Eacute")
			return "É";
		if (escaped == "Ecirc")
			return "Ê";
		if (escaped == "Euml")
			return "Ë";
		if (escaped == "Igrave")
			return "Ì";
		if (escaped == "Iacute")
			return "Í";
		if (escaped == "Icirc")
			return "Î";
		if (escaped == "Iuml")
			return "Ï";
		if (escaped == "ETH")
			return "Ð";
		if (escaped == "Ntilde")
			return "Ñ";
		if (escaped == "Ograve")
			return "Ò";
		if (escaped == "Oacute")
			return "Ó";
		if (escaped == "Ocirc")
			return "Ô";
		if (escaped == "Otilde")
			return "Õ";
		if (escaped == "Ouml")
			return "Ö";
		if (escaped == "times")
			return "×";
		if (escaped == "Oslash")
			return "Ø";
		if (escaped == "Ugrave")
			return "Ù";
		if (escaped == "Uacute")
			return "Ú";
		if (escaped == "Ucirc")
			return "Û";
		if (escaped == "Uuml")
			return "Ü";
		if (escaped == "Yacute")
			return "Ý";
		if (escaped == "THORN")
			return "Þ";
		if (escaped == "szlig")
			return "ß";
		if (escaped == "agrave")
			return "à";
		if (escaped == "aacute")
			return "á";
		if (escaped == "acirc")
			return "â";
		if (escaped == "atilde")
			return "ã";
		if (escaped == "auml")
			return "ä";
		if (escaped == "aring")
			return "å";
		if (escaped == "aelig")
			return "æ";
		if (escaped == "ccedil")
			return "ç";
		if (escaped == "egrave")
			return "è";
		if (escaped == "eacute")
			return "é";
		if (escaped == "ecirc")
			return "ê";
		if (escaped == "euml")
			return "ë";
		if (escaped == "igrave")
			return "ì";
		if (escaped == "iacute")
			return "í";
		if (escaped == "agrave")
			return "í";
		if (escaped == "icirc")
			return "î";
		if (escaped == "iuml")
			return "ï";
		if (escaped == "eth")
			return "ð";
		if (escaped == "ntilde")
			return "ñ";
		if (escaped == "ograve")
			return "ò";
		if (escaped == "oacute")
			return "ó";
		if (escaped == "ocirc")
			return "ô";
		if (escaped == "otilde")
			return "õ";
		if (escaped == "ouml")
			return "ö";
		if (escaped == "divide")
			return "÷";
		if (escaped == "oslash")
			return "ø";
		if (escaped == "ugrave")
			return "ù";
		if (escaped == "uacute")
			return "ú";
		if (escaped == "ucirc")
			return "û";
		if (escaped == "uuml")
			return "ü";
		if (escaped == "yacute")
			return "ý";
		if (escaped == "thorn")
			return "þ";
#endif
	}

}

static std::string html_unescape(std::string htmlcharseq)
{
	std::string retstr;
	std::string::iterator chariter = htmlcharseq.begin();

	bool unes = false;

	while (chariter != htmlcharseq.end()){
		if (unes){
			// 找到 ;
			std::string::iterator es_end =  std::find(chariter, htmlcharseq.end(), ';');
			// 把 ; 之前的给组合一下.

			std::string es_seq(chariter, es_end);

			retstr.append( html_unescape_char(es_seq) );

			chariter = es_end + 1;
			unes = false;
		}else{
			if (*chariter == '&'){
				unes = true;
			}else{
				retstr.append(1, *chariter);
			}
			++ chariter;
		}
	}

	return retstr;
}

//
struct urlpreview
{
	boost::asio::io_service &io_service;
	boost::function<void ( std::string ) > m_sender;
	std::string m_speaker;
	std::string m_url;
	boost::shared_ptr<avhttp::http_stream> m_httpstream;

	boost::shared_ptr<boost::asio::streambuf> m_content;
	int m_redirect ;

	template<class MsgSender>
	urlpreview( boost::asio::io_service &_io_service,
				MsgSender sender,
				std::string speaker, std::string url, int redirectlevel = 0 )
		: io_service( _io_service ), m_sender( sender )
		, m_speaker( speaker ), m_url( url )
		, m_httpstream( new avhttp::http_stream( io_service ) )
		, m_redirect(redirectlevel)
	{
		// 开启 avhttp 下载页面
		m_httpstream->check_certificate( false );
		m_httpstream->async_open( url, *this );
	}

	// 打开在这里
	void operator()( const boost::system::error_code &ec )
	{
		if( ec )
		{
			// 报告出错。
			m_sender( boost::str( boost::format( "@%s, 获取url有错 %s" ) % m_speaker % ec.message() ) );
			return;
		}

		// 根据 content_type 了， 如果不是 text/html 的就不要继续下去了.
		avhttp::response_opts opt = m_httpstream->response_options();

		if( ! is_html( opt.find( avhttp::http_options::content_type ) ) )
		{
			// 报告类型就可以
			m_sender( boost::str( boost::format( "%s 发的 ⇪ 类型是 %s " ) % m_speaker % opt.find( avhttp::http_options::content_type ) ) );
			return;
		}

		m_content.reset( new boost::asio::streambuf );

		unsigned content_length = 0;

		try
		{
			std::string _content_length = opt.find( avhttp::http_options::content_length );

			content_length = boost::lexical_cast<unsigned>( _content_length );

			// 下载第一个数据包. 咱不下全，就第一个足够了.
			// std::min( );
		}
		catch( boost::bad_lexical_cast & err )
		{
			content_length = 4096;
		}

		boost::asio::async_read(*m_httpstream, *m_content,
						boost::bind(read_until_title, _1, _2, std::min<unsigned>( content_length, 4096 ), boost::ref(*m_content) ),
						*this
		);
	}

	// boost::asio::async_read 回调.
	void operator()( const boost::system::error_code &ec, int bytes_transferred )
	{
		if( ec && ec != boost::asio::error::eof )
		{
			m_sender( boost::str( boost::format( "@%s, 获取url有错 %s" ) % m_speaker % ec.message() ) );
			return;
		}

		// 解析 <title>
		std::string content;
		content.resize( m_content->size() );
		m_content->sgetn( &content[0], m_content->size() );

		// 转换成小写, 统一处理, 避免遗漏大写的title标签.
		boost::to_lower( content );

		//去掉换行.
		boost::replace_all( content, "\r", "" );
		boost::replace_all( content, "\n", "" );
		// 获取charset
		std::string charset = get_char_set( m_httpstream->response_options().find( avhttp::http_options::content_type ), content );
		content = content.substr(0, content.find("</title>")+8);
		// 匹配.
		boost::regex ex( "<title[^>]*>(.*)</title>" );
		boost::cmatch what;

		if( boost::regex_search( content.c_str(), what, ex ) )
		{
			std::string title = what[1];
			boost::trim_left(title);

			try
			{
				if( charset != "utf8" && charset != "utf" && charset != "utf-8" )
				{
					title = boost::locale::conv::between( title, "UTF-8", charset );
				}

				boost::trim( title );

				title = html_unescape(title);
				// 将 &bnp 这种反格式化.
				m_sender( boost::str( boost::format( "@%s ⇪ 标题： %s " ) % m_speaker % title ) );
			}
			catch( const std::runtime_error & )
			{
				m_sender( boost::str( boost::format( "@%s ⇪ 解码网页发生错误 " ) % m_speaker ) );
			}
		}
		else
		{
			// 解析是不是 html 重定向
			boost::regex ex("<meta +http-equiv=\"refresh\" +content=\"[^;];url=(.*)\" *>");
			// title 都没有！
			if (m_redirect < 10 && boost::regex_search(content.c_str(), what, ex)){
				urlpreview(io_service, m_sender, m_speaker, what[1], m_redirect + 1);
			}else{
				m_sender( boost::str( boost::format( "@%s ⇪ url 无标题 " ) % m_speaker ) );
			}
		}
	}
};

}

void urlpreview::operator()( boost::property_tree::ptree message )
{
	if( message.get<std::string>( "channel" ) != m_channel_name )
	{
		return;
	}

	// 检查 URL
	std::string txt = message.get<std::string>( "message.text" );
	std::string speaker = message.get<std::string>( "who.nick" );; // 发了 url 的人的 nick

	// 用正则表达式
	// http://.*
	// https://.*
	// 统一为
	// http[s]?://[^ ].*
	// 使用 boost_regex_search
	boost::regex ex( "https?://[^ 】]*" );
	boost::cmatch what;

	if( boost::regex_search( txt.c_str(), what, ex ) )
	{
		// 检查到 URL ?
		std::string url = what[0];
		do_urlpreview(speaker, boost::trim_copy(url), boost::posix_time::from_time_t(std::time(NULL)));
	}
}

static bool find_url(std::string url, const std::pair<std::string, boost::posix_time::ptime> & item)
{
	return item.first == url;
}

void urlpreview::do_urlpreview(std::string speaker, std::string url, boost::posix_time::ptime current )
{
	boost::posix_time::time_duration jiange = boost::posix_time::minutes(99);
	// 到 urllist 里找一下是否有重复的.
	if (!urllist->empty()){
		urllist_type::iterator prev_url_iter = std::find_if(urllist->begin(), urllist->end(), boost::bind(find_url, url, _1));
		if (prev_url_iter != urllist->end()){
			// 有找到啊! 检查时间.
			jiange = current -  prev_url_iter->second;

			urllist->erase(prev_url_iter);
		}
	}
	urllist->push_back(std::make_pair(url, current));

	if (jiange.minutes() >= 1){
		// 超过一分钟了,  应该说了.
		// 把真正的工作交给真正的 urlpreview
		detail::urlpreview( io_service, m_sender, speaker, url);
	}
}

