#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
namespace asio = boost::asio;
using boost::property_tree::ptree;
#include <boost/random.hpp>

struct StaticContent
{
	StaticContent(asio::io_service& io, std::function<void(std::string)> sender);

	void operator()(boost::system::error_code ec) {}

	void operator()(const ptree& pt);

	asio::io_service& io_;
	std::function<void(std::string)> sender_;
	typedef boost::regex Keywords;
	typedef std::vector<std::string> Messages;
	std::map<Keywords, Messages> static_contents_;

	boost::random::mt19937 g_;
	boost::uniform_int<> d_;

};


template<typename MsgSender>
StaticContent
make_static_content(asio::io_service& io, MsgSender sender)
{
	return StaticContent(io, sender);
}
