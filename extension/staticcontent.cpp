#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/random.hpp>
#include <boost/function.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/regex.hpp>
#include <ctime>
#include <boost/foreach.hpp>

#include "staticcontent.hpp"

struct StaticContent
{
	StaticContent(boost::asio::io_service& io, boost::function<void(std::string)> sender);

	void operator()(boost::system::error_code ec) {}

	void operator()(const boost::property_tree::ptree& pt);

	boost::asio::io_service& io_;
	boost::function<void(std::string)> sender_;
	typedef boost::regex Keywords;
	typedef std::vector<std::string> Messages;
	std::map<Keywords, Messages> static_contents_;

	boost::random::mt19937 g_;
	boost::uniform_int<> d_;

};

StaticContent::StaticContent(boost::asio::io_service& io, boost::function<void(std::string)> sender)
	: io_(io)
	, sender_(sender)
	, d_(0, 10000)
{
	using namespace boost::property_tree::xml_parser;
	g_.seed(std::time(0));
	std::string filename = "static.xml";
	if(fs::exists(filename))
	{
		boost::property_tree::ptree pt;
		read_xml(filename, pt);
		BOOST_FOREACH(const boost::property_tree::ptree::value_type & item,  pt.get_child("static"))
		{
			std::string keyword = item.second.get<std::string>("keyword");
			std::vector<std::string> messages;
			BOOST_FOREACH(const boost::property_tree::ptree::value_type & message,  item.second.get_child("messages"))
			{
				messages.push_back(message.second.get_value<std::string>());
			}
			static_contents_[boost::regex(keyword)] = messages;
		}
	}
}

void StaticContent::operator()(const boost::property_tree::ptree& pt)
{
	using boost::property_tree::ptree;
	typedef std::pair<Keywords, Messages> item_type;
	std::string text = pt.get<std::string>("message.text");

	BOOST_FOREACH(const item_type & item,  static_contents_)
	{
		if(boost::regex_search(text, item.first))
		{
			sender_(item.second[d_(g_) % item.second.size()]);
		}
	}
}

avbot_extension make_static_content(boost::asio::io_service& io, std::string channel_name, boost::function<void(std::string)> sender)
{
	return avbot_extension(
		channel_name,
		StaticContent(io, sender)
	);
}
