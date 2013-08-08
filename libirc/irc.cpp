
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
	msg_sender_loop(boost::shared_ptr<client> _client)
		: m_client(_client)
	{
	};

	void operator()()
	{
	}

private:
	boost::shared_ptr<client> m_client;
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
		, insending(false)
		, quitting_(false)
		, messages_send_queue_(_io_service, 200) // 缓存最后  200 条指令.
	{
	}

	void start()
	{
		return io_service.post(boost::bind(&client::connect, shared_from_this()));
	}

	void stop()
	{
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
	void handle_read_request(const boost::system::error_code& err, std::size_t readed)
	{
		if(err)
		{
			response_.consume(response_.size());
			request_.consume(request_.size());
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

			process_request(response_);
		}
	}


	void handle_write_request(const boost::system::error_code& err, std::size_t bytewrited, boost::asio::coroutine coro)
	{
		std::istream  req(&request_);
		line.clear();

		BOOST_ASIO_CORO_REENTER(&coro)
		{
			if(!err)
			{
				if(request_.size())
				{
					std::getline(req, line);

					if(line.length() > 1)
					{

						line.append("\n");

						BOOST_ASIO_CORO_YIELD  boost::asio::async_write(socket_, boost::asio::buffer(line),
								boost::bind(&client::handle_write_request, shared_from_this(), _1, _2, coro)
																	   );

						boost::delayedcallms(io_service, 450, boost::bind(&client::handle_write_request, shared_from_this(), boost::system::error_code(), 0,  boost::asio::coroutine()));

					}
					else
					{
						BOOST_ASIO_CORO_YIELD boost::delayedcallms(io_service, 5, boost::bind(&client::handle_write_request, shared_from_this(), boost::system::error_code(), 0, coro));
					}

				}
				else
				{
					insending = false;
				}
			}
			else
			{
				insending = false;
				relogin();
#ifdef DEBUG
				std::cout << "Error: " << err.message() << "\n";
#endif
			}
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
		std::string msg;
		msg.append(data, len);

		std::ostream request_stream(&request_);
		request_stream << msg;

		if(!insending)
		{
			insending = true;

			boost::asio::async_write(socket_, boost::asio::null_buffers(),
									 boost::bind(&client::handle_write_request, shared_from_this(),
												 boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, boost::asio::coroutine()));
		}
	}

	void process_request(boost::asio::streambuf& buf)
	{
		std::string req;

		std::istream is(&buf);
		is.unsetf(std::ios_base::skipws);

		while(req.clear(), std::getline(is, req), (!is.eof() && ! req.empty()))
		{
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

				continue;
			}

			if(req.substr(0, 4) == "PING")
			{
				send_request("PONG " + req.substr(6, req.length() - 8));
				continue;
			}

			std::size_t pos = req.find(" PRIVMSG ") + 1;

			if(!pos)
				continue;

			std::string msg = req;
			irc_msg m;

			pos = msg.find("!") + 1;

			if(!pos)
				continue;

			m.whom = msg.substr(1, pos - 2);

			msg = msg.substr(pos);

			pos = msg.find(" PRIVMSG ") + 1;

			if(!pos)
				continue;

			m.locate = msg.substr(0, pos - 1);

			msg = msg.substr(pos);

			pos = msg.find("PRIVMSG ") + 1;

			if(!pos)
				continue;

			msg = msg.substr(strlen("PRIVMSG "));

			pos = msg.find(" ") + 1;

			if(!pos)
				continue;

			m.from = msg.substr(0, pos - 1);

			msg = msg.substr(pos);

			pos = msg.find(":") + 1;

			if(!pos)
				continue;

			m.msg = msg.substr(pos, msg.length() - pos);

			if(* m.msg.rbegin() == '\r')
				m.msg.resize(m.msg.length() - 1);

			cb_(m);
		}
	}

	void connect()
	{
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
		msg_queue_.clear();
		BOOST_FOREACH(std::string & str, join_queue_)
		msg_queue_.push_back(str);
		join_queue_.clear();
		connect();
	}

	void connected()
	{
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
	boost::asio::ip::tcp::socket    socket_;
	boost::asio::streambuf          request_;
	boost::asio::streambuf          response_;
	boost::signals2::signal<void(irc_msg)> cb_;
	std::string                     user_;
	std::string                     pwd_;
	std::string                     server_;
	bool                            login_;
	std::vector<std::string>        msg_queue_;
	std::vector<std::string>        join_queue_;
	unsigned int                    retry_count_;
	const unsigned int              c_retry_cuont;
	bool insending;

	std::string line;

	bool quitting_;

	boost::async_coro_queue<
		boost::circular_buffer_space_optimized<
			std::string
		>
	> messages_send_queue_;
};

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
