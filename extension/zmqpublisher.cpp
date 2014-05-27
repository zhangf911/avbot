#include "zmqpublisher.hpp"
#include "zmq.h"
#include <boost/property_tree/json_parser.hpp>

class ZmqPublisher
{
public:
	ZmqPublisher()
	{
		ctx_ = zmq_ctx_new();
		socket_ = zmq_socket(ctx_, ZMQ_PUB);
		zmq_bind(socket_, "tcp://*:8123");
	}

	void send(const std::string& channel, const std::string& str)
	{
		zmq_send(socket_, channel.c_str(), channel.size(), ZMQ_SNDMORE);
		zmq_send(socket_, str.c_str(), str.size(), 0);
	}

	virtual ~ZmqPublisher()
	{
		zmq_close(socket_);
		zmq_ctx_term(ctx_);
	}

private:
	void* ctx_;
	void* socket_;
};

class ZmqPublisherClient
	: virtual private ZmqPublisher
{
public:
	ZmqPublisherClient(asio::io_service &io, std::string channel_name, boost::function<void (std::string)> sender)
		: io_(io)
		, channel_name_(channel_name)
		, sender_(sender)
	{

	}

	void operator()(boost::property_tree::ptree msg)
	{
		std::stringstream ss;
		boost::property_tree::json_parser::write_json(ss, msg);
		send(channel_name_, ss.str());
	}

private:
	asio::io_service &io_;
	std::string channel_name_;
	boost::function<void (std::string)> sender_;
};

avbot_extension make_zmq_publisher(asio::io_service &io, std::string channel_name, boost::function<void (std::string)> sender)
{
	return avbot_extension(channel_name, ZmqPublisherClient(io, channel_name, sender));
}