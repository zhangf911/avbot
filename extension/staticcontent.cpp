#include "staticcontent.hpp"
#include <boost/property_tree/xml_parser.hpp>
using namespace boost::property_tree::xml_parser;
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/regex.hpp>
#include <ctime>

StaticContent::StaticContent(asio::io_service& io, std::function<void(std::string)> sender)
	: io_(io)
	, sender_(sender)
	, d_(0, 10000)
{
	g_.seed(std::time(0));
	std::string filename = "static.xml";
	if(fs::exists(filename))
	{
		ptree pt;
		read_xml(filename, pt);
		for(auto item : pt.get_child("static"))
		{
			std::string keyword = item.second.get<std::string>("keyword");
			std::vector<std::string> messages;	
			for(auto message : item.second.get_child("messages"))
			{
				messages.push_back(message.second.get_value<std::string>());
			}
			static_contents_[boost::regex(keyword)] = boost::move(messages);
		}
	}
}

void StaticContent::operator()(const ptree& pt)
{
	std::string text = pt.get<std::string>("message.text");
	for(auto item : static_contents_)
	{
		if(boost::regex_search(text, item.first))
		{
			sender_(item.second[d_(g_) % item.second.size()]);
		}
	}
}
