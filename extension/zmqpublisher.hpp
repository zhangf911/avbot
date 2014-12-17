
#include "extension.hpp"

avbot_extension make_zmq_publisher(boost::asio::io_service& io, std::string channel_name, boost::function<void(std::string)> sender);
