
#pragma once

#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <avhttp.hpp>

namespace detail {

static inline bool is_html( const std::string contenttype )
{
	if( contenttype.empty() )
		return false;

	if( contenttype == "text/html" )
		return true;

	if( contenttype.find( "text/html" ) != std::string::npos )
		return true;
}

//
struct urlpreview{
	boost::asio::io_service &io_service;
	boost::function<void (std::string) > m_sender;
	std::string m_channel_name;
	std::string m_speaker;
	std::string m_url;
	boost::shared_ptr<avhttp::http_stream> m_httpstream;

	boost::shared_ptr<boost::asio::streambuf> m_content;


	template<class MsgSender>
	urlpreview(boost::asio::io_service &_io_service,
				MsgSender sender, std::string channel_name,
				std::string speaker, std::string url)
		:io_service(_io_service), m_sender(sender), m_channel_name(channel_name)
		, m_speaker(speaker), m_url(url), m_httpstream(new avhttp::http_stream(io_service))
	{
		// 开启 avhttp 下载页面
		m_httpstream->check_certificate(false);
		m_httpstream->async_open(url, *this);
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

		if(! is_html( opt.find( avhttp::http_options::content_type ) ) )
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
		boost::asio::async_read(*m_httpstream, m_content->prepare( std::min<unsigned>( content_length, 4096 ) ), *this );
	}

	void operator()(const boost::system::error_code &ec, int bytes_transferred)
	{
		if (ec && ec != boost::asio::error::eof){
			m_sender(boost::str(boost::format("@%s, 获取url有错 %s") % m_speaker % ec.message() ));
			return;
		}
		m_content->commit(bytes_transferred);
		// 解析 <title>
		std::string content;
		content.resize(bytes_transferred);
		m_content->sgetn(&content[0], bytes_transferred);

		//去掉换行.
		boost::replace_all(content, "\r", "");
		boost::replace_all(content, "\n", "");
		// 匹配.
		boost::regex ex("<title[^>]*>(.*)</title>");
		boost::cmatch what;
		if (boost::regex_search(content.c_str(), what, ex))
		{
			std::string title = what[1];
			boost::trim(title);
			boost::replace_all(title, "&nbsp;", " ");
			// 将 &bnp 这种反格式化.
			m_sender(boost::str(boost::format("@%s ⇪ 标题： %s ") % m_speaker % title ));
		}else{
			// title 都没有！
			m_sender(boost::str(boost::format("@%s ⇪ url 无标题") % m_speaker  ));
		}
	}
};

}

class urlpreview{
	boost::asio::io_service &io_service;
	boost::function<void (std::string) > m_sender;
	std::string m_channel_name;

public:
	template<class MsgSender>
	urlpreview(boost::asio::io_service &_io_service,  MsgSender sender, std::string channel_name)
		:io_service(_io_service), m_sender(sender), m_channel_name(channel_name)
	{
	}

	// on_message 回调.
	void operator()(boost::property_tree::ptree message) const
	{
		if( message.get<std::string>( "channel" ) != m_channel_name )
		{
			return;
		}

		// 检查 URL
		std::string txt = message.get<std::string>("message.text");
		std::string speaker = message.get<std::string>("who.nick");; // 发了 url 的人的 nick

		// 用正则表达式
		// http://.*
		// https://.*
		// 统一为
		// http[s]?://[^ ].*
		// 使用 boost_regex_search
		boost::regex ex("https?://[^ ]*");
		boost::cmatch what;
		if (boost::regex_search(txt.c_str(), what, ex)){
			// 检查到 URL ?
			std::string url = what[0];
			// 把真正的工作交给真正的 urlpreview
			detail::urlpreview(io_service, m_sender, m_channel_name, speaker, url);
		}
	}
};
