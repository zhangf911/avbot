
#include <boost/circular_buffer.hpp>

#include <vector>
#include <string>
#include <iostream>

#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/signals2.hpp>
#include <boost/async_coro_queue.hpp>

#include "avproxy.hpp"
#include "boost/timedcall.hpp"

#include "./irc.hpp"

namespace irc {
namespace impl {

class client;

class msg_sender_loop : boost::asio::coroutine
{
public:
	msg_sender_loop(boost::shared_ptr<client> _client);
	void operator()(boost::system::error_code ec, std::size_t bytes_transferred, std::string value);

private:
	boost::shared_ptr<client> m_client;
	boost::shared_ptr<std::string>	m_value;
};

class client : public boost::enable_shared_from_this<client>
{
public:
	client(boost::asio::io_service &_io_service, const std::string& user, const std::string& user_pwd = "", const std::string& server = "irc.freenode.net", const unsigned int max_retry_count = 1000)
		: io_service(_io_service)
		, socket_(io_service)
		, user_(user)
		, pwd_(user_pwd)
		, server_(server)
		, retry_count_(max_retry_count)
		, c_retry_cuont(max_retry_count)
		, login_(false)
		, quitting_(false)
		, messages_send_queue_(_io_service, 200) // 缓存最后  200 条指令.
	{
	}

	boost::asio::io_service & get_io_service()
	{
		return io_service;
	}

	void start()
	{
		msg_sender_loop op(shared_from_this());
		return io_service.post(boost::bind(&client::connect, shared_from_this()));
	}

	void stop()
	{
		messages_send_queue_.cancele();
		quitting_ = false;
	}

public:
	void on_privmsg_message(const privmsg_cb &cb)
	{
		cb_.connect(cb);
	}

	void join(const std::string& ch, const std::string &pwd)
	{
		std::string msg;

		pwd.empty() ? msg = "JOIN " + ch : msg = "JOIN " + ch + " " + pwd;

		if(!login_)
			msg_queue_.push_back(msg);
		else
		{
			send_request(msg);
			join_queue_.push_back(msg);
		}
	}

	void chat(const std::string whom, const std::string msg)
	{
		std::vector<std::string> msgs;
		boost::split(msgs, msg, boost::is_any_of("\r\n"));
		BOOST_FOREACH(std::string _msg,  msgs)
		{
			if(_msg.length() > 0)
				send_request("PRIVMSG " + whom + " :" + _msg);
		}
	}

	void send_command(const std::string& cmd)
	{
		send_request(cmd);
	}

	void oper(const std::string& user, const std::string& pwd)
	{
		send_request("OPER " + user + " " + pwd);
	}

private:
	void handle_read_request(const boost::system::error_code& err, std::size_t bytes_transferred)
	{
		if(err)
		{
			response_.consume(response_.size());
			relogin();
#ifdef DEBUG
			std::cout << "Error: " << err.message() << "\n";
#endif
		}
		else
		{
			boost::asio::async_read_until(socket_, response_, "\r\n",
										  boost::bind(&client::handle_read_request, shared_from_this(), _1, _2)
										 );

			process_request(response_, bytes_transferred);
		}
	}

	void handle_connect_request(const boost::system::error_code& err)
	{
		if(!err)
		{
			connected();

			boost::asio::async_read_until(socket_, response_, "\r\n",
										  boost::bind(&client::handle_read_request, shared_from_this(),
												  boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}
		else if(err != boost::asio::error::eof)
		{
			io_service.post(boost::bind(&client::relogin, shared_from_this()));

#ifdef DEBUG
			std::cerr << "irc: connect error: " << err.message() << std::endl;
#endif
		}
	}

	void send_request(const std::string& msg)
	{
		std::string data = msg + "\r\n";
		send_data(data.c_str(), data.length());
	}

	void send_data(const char* data, const size_t len)
	{
		messages_send_queue_.push(std::string(data, len));
	}

	void process_request(boost::asio::streambuf& buf, std::size_t bytes_transferred)
	{
		boost::smatch what;
		std::string req;

		req.resize(bytes_transferred);

		buf.sgetn(&req[0], bytes_transferred);

		req.resize(bytes_transferred - 2);

#ifdef DEBUG
		std::cout << req << std::endl;
#endif

		//add auto modify a nick
		if(req.find("Nickname is already in use.") != std::string::npos)
		{
			user_ += "_";
			send_request("NICK " + user_);
			send_request("USER " + user_ + " 0 * " + user_);

			BOOST_FOREACH(std::string & str, join_queue_)
			send_request(str);
			return;
		}

		if(req.substr(0, 4) == "PING")
		{
			send_request("PONG " + req.substr(6, req.length() - 8));
			return;
		}

		boost::regex regex_privatemsg(":([^!]+)!([^ ]+) PRIVMSG ([#a-zA-Z0-9]+) :(.*)[\\r\\n]*");

		if (boost::regex_match(req, what, regex_privatemsg))
		{
			irc_msg m;
			m.whom = what[1];
			m.locate = what[2];
			m.from = what[3];
			m.msg = what[4];

			cb_(m);
		};
	}

	void connect()
	{
		if (quitting_)
			return;

		using namespace boost::asio::ip;

		std::string server, port;
		boost::smatch what;

		if(boost::regex_match(server_, what, boost::regex("([a-zA-Z\\.]+)(:[\\d]+)?")))
		{
			server = what[1];

			if(what[2].matched)
			{
				port = std::string(what[2]).substr(1);
			}
			else
			{
				port = "6667";
			}
		}
		else
		{
			boost::throw_exception(
				std::invalid_argument("bad server name for irc")
			);
		};

		avproxy::async_proxy_connect(
			avproxy::autoproxychain(
				socket_, tcp::resolver::query(server, port)
			),
			boost::bind(
				&client::handle_connect_request, shared_from_this(),
				boost::asio::placeholders::error
			)
		);
	}

	void relogin()
	{
		if (quitting_)
			return;
		login_ = false;
		boost::system::error_code ec;
		socket_.close(ec);
		retry_count_--;

		if(retry_count_ <= 0)
		{
			std::cout << "Irc Server has offline!!!" <<  std::endl;;
			return;
		}

		std::cout << "irc: retry in 10s..." <<  std::endl;
		socket_.close();

		boost::delayedcallsec(io_service, 10, boost::bind(&client::relogin_delayed, shared_from_this()));
	}

	void relogin_delayed()
	{
		if (quitting_)
			return;
		msg_queue_.clear();
		BOOST_FOREACH(std::string & str, join_queue_)
		msg_queue_.push_back(str);
		join_queue_.clear();
		connect();
	}

	void connected()
	{
	 	if (quitting_)
			return;

		if(!pwd_.empty())
			send_request("PASS " + pwd_);

		send_request("NICK " + user_);
		send_request("USER " + user_ + " 0 * " + user_);

		login_ = true;
		retry_count_ = c_retry_cuont;

		BOOST_FOREACH(std::string & str, msg_queue_)
		{
			join_queue_.push_back(str);
			send_request(str);
		}

		msg_queue_.clear();

	}

private:
	boost::asio::io_service &       io_service;

	boost::asio::streambuf          response_;
	boost::signals2::signal<void(irc_msg)> cb_;
	std::string                     user_;
	std::string                     pwd_;
	std::string                     server_;
	std::vector<std::string>        msg_queue_;
	std::vector<std::string>        join_queue_;
	unsigned int                    retry_count_;
	const unsigned int              c_retry_cuont;

	std::string line;

public:
	boost::asio::ip::tcp::socket    socket_;
	bool login_;
	bool quitting_;
	boost::async_coro_queue<
		boost::circular_buffer_space_optimized<
			std::string
		>
	> messages_send_queue_;
};

msg_sender_loop::msg_sender_loop(boost::shared_ptr<client> _client)
	: m_client(_client), m_value(boost::make_shared<std::string>())
{
	m_client->messages_send_queue_.async_pop(
		boost::bind<void>(*this, boost::system::error_code(), 0, _1)
	);
}

void msg_sender_loop::operator()(boost::system::error_code ec, std::size_t bytes_transferred, std::string value)
{
	BOOST_ASIO_CORO_REENTER(this)
	{for (;!m_client->quitting_;){

		// 等待 login 状态

		while (!m_client->login_)
		{
			BOOST_ASIO_CORO_YIELD boost::delayedcallsec(
				m_client->get_io_service(), 5, boost::bind<void>(*this, ec, bytes_transferred, value));
		}

		while (!m_client->quitting_ && m_client->login_){
			*m_value = value;
			// 发送
			BOOST_ASIO_CORO_YIELD boost::asio::async_write(
				m_client->socket_,
				boost::asio::buffer(*m_value),
				boost::bind<void>(*this, _1, _2, value)
			);

			if (ec){
				// 错误? 恩 ~~~ 糟糕咯
				m_client->socket_.close(ec);

				BOOST_ASIO_CORO_YIELD boost::delayedcallsec(m_client->get_io_service(), 25,
					boost::bind<void>(*this, ec, bytes_transferred, value)
				);
			}

			BOOST_ASIO_CORO_YIELD boost::delayedcallms(
				m_client->get_io_service(), 468, boost::bind<void>(*this, ec, bytes_transferred, value));

			BOOST_ASIO_CORO_YIELD m_client->messages_send_queue_.async_pop(
				boost::bind<void>(*this, ec, 0, _1)
			);
		}
	}}
}

}

client::client(boost::asio::io_service& _io_service, const std::string& user, const std::string& user_pwd, const std::string& server, const unsigned int max_retry_count)
{
	impl = boost::make_shared<impl::client>(
			   boost::ref(_io_service), user, user_pwd, server, max_retry_count
		   );
	impl->start();
}
client::~client()
{
	impl->stop();
}

void client::on_privmsg_message(const privmsg_cb& cb)
{
	impl->on_privmsg_message(cb);
}

void client::join(const std::string& ch, const std::string& pwd)
{
	impl->join(ch, pwd);
}

void client::chat(const std::string whom, const std::string msg)
{
	impl->chat(whom, msg);
}

}
