
#include <boost/circular_buffer.hpp>

#include <vector>
#include <string>
#include <iostream>

#include <boost/log/trivial.hpp>
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
	template<class Handler>
	void async_send_line(std::string line, Handler handler);
private:
	boost::shared_ptr<client> m_client;
	boost::shared_ptr<std::string> m_value;
	boost::shared_ptr<std::string> m_last_line;
};

class msg_reader_loop : boost::asio::coroutine
{
public:
	msg_reader_loop(boost::shared_ptr<client> _client);
	void operator()(boost::system::error_code ec, std::size_t bytes_transferred);

private:
	template<class Handler>
	void async_connect_irc(Handler handler);
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
		, c_retry_cuont(0)
		, connected_(false)
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
		msg_sender_loop op_send(shared_from_this());
		msg_reader_loop op_read(shared_from_this());
	}

	void stop()
	{
		boost::system::error_code ec;
		socket_.close(ec);
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

		join_queue_.push_back(msg);
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
	void send_request(const std::string& msg)
	{
		std::string data = msg + "\r\n";
		messages_send_queue_.push(data);
	}

public:
	void process_request(std::size_t bytes_transferred)
	{
		boost::smatch what;
		std::string req;

		req.resize(bytes_transferred);

		response_.sgetn(&req[0], bytes_transferred);

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

		//PING :5211858A
		boost::regex regex_ping("PING ([^ ]+).*");

		if (boost::regex_match(req, what, regex_ping))
		{
			send_request("PONG " + what[1]);
			return;
		}

		boost::regex regex_privatemsg(":([^!]+)!([^ ]+) PRIVMSG ([^ ]+) :(.*)[\\r\\n]*");

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

public:
	boost::asio::streambuf          response_;
	boost::signals2::signal<void(irc_msg)> cb_;
	std::string                     user_;
	std::string                     pwd_;
	std::string                     server_;
	std::vector<std::string>        join_queue_;
	const unsigned int retry_count_;
	unsigned int c_retry_cuont;

	boost::asio::io_service& io_service;
	boost::asio::ip::tcp::socket socket_;

	bool connected_;
	bool quitting_;

	boost::async_coro_queue<
		boost::circular_buffer_space_optimized<
			std::string
		>
	> messages_send_queue_;
};

msg_reader_loop::msg_reader_loop(boost::shared_ptr< client > _client)
	: m_client(_client)
{
	// start routine now!
	m_client->io_service.post(
		boost::bind<void>(*this, boost::system::error_code(), 0)
    );
}

template<class Handler>
void msg_reader_loop::async_connect_irc(Handler handler)
{
	using namespace boost::asio::ip;

	std::string server, port;
	boost::smatch what;

	if(boost::regex_match(m_client->server_, what, boost::regex("([a-zA-Z0-9\\.]+)(:([\\d]+))?")))
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
			m_client->socket_, tcp::resolver::query(server, port)
		),
		handler
	);
}

void msg_reader_loop::operator()(boost::system::error_code ec, std::size_t bytes_transferred)
{
	if (m_client->quitting_)
		return;

	BOOST_ASIO_CORO_REENTER(this)
	{
		// 成功? 失败?
		do
		{
	  		m_client->connected_ = false;

	  		if (m_client->c_retry_cuont > m_client->retry_count_)
			{
				BOOST_LOG_TRIVIAL(error) <<  "irc: to many retries! quite irc!";
				return;
			}

	  		BOOST_LOG_TRIVIAL(info) <<  "irc: connecting to server: " << m_client->server_  << " ...";

			BOOST_ASIO_CORO_YIELD async_connect_irc(
				boost::bind<void>(*this, _1, bytes_transferred)
			);

			if (ec)
			{

				BOOST_LOG_TRIVIAL(error) <<  "irc: error connecting to server : " << ec.message();

				BOOST_LOG_TRIVIAL(info) <<  "retry in 15s ...";

				m_client->c_retry_cuont ++;

				BOOST_ASIO_CORO_YIELD boost::delayedcallsec(
					m_client->io_service,
					15,
					boost::bind<void>(*this, ec, bytes_transferred)
				);
			}

		} while (ec);

  		BOOST_LOG_TRIVIAL(info) <<  "irc: conneted to server" << m_client->server_  << " .";

		// 完成登录! 接着该发送登录数据了!
		m_client->connected_ = true;
		m_client->c_retry_cuont = 0;

		// 唤醒 write 协程
		m_client->messages_send_queue_.push(std::string(""));
		m_client->response_.consume(m_client->response_.size());

		// 异步读取循环操作
		while (!ec)
		{
			// 读取 !
			BOOST_ASIO_CORO_YIELD boost::asio::async_read_until(
				m_client->socket_,
				m_client->response_,
				"\r\n",
				*this
			);
			// 处理
			if (!ec)
				m_client->process_request(bytes_transferred);
		}
		m_client->connected_ = false;

		_coro_value = 0;

		m_client->io_service.post(
			boost::bind<void>(*this, boost::system::error_code(), 0)
		);
	}

}

msg_sender_loop::msg_sender_loop(boost::shared_ptr<client> _client)
	: m_client(_client)
	, m_value(boost::make_shared<std::string>())
	, m_last_line(boost::make_shared<std::string>())
{
	boost::system::error_code ec;
	m_client->messages_send_queue_.async_pop(
		boost::bind<void>(*this, ec, 0, _1)
	);
}

template<class Handler>
void msg_sender_loop::async_send_line(std::string line, Handler handler)
{
	*m_last_line = line + "\r\n";

	boost::asio::async_write(
		m_client->socket_,
		boost::asio::buffer(*m_last_line),
		handler
	);
}

void msg_sender_loop::operator()(boost::system::error_code ec, std::size_t bytes_transferred, std::string value)
{
	int i;

	if (m_client->quitting_)
		return;

	BOOST_ASIO_CORO_REENTER(this)
	{
		if ( value.empty() )
		{
			// 开启登录过程.
			if(! m_client->pwd_.empty())
			{
				BOOST_ASIO_CORO_YIELD async_send_line(
					"PASS " + m_client->pwd_,
					boost::bind<void>(*this, _1, _2, value)
				);
			}

			BOOST_ASIO_CORO_YIELD async_send_line(
				"NICK " + m_client->user_,
				boost::bind<void>(*this, _1, _2, value)
			);

			BOOST_ASIO_CORO_YIELD async_send_line(
				"USER " + m_client->user_ + " 0 * " + m_client->user_,
				boost::bind<void>(*this, _1, _2, value)
			);

			BOOST_ASIO_CORO_YIELD boost::delayedcallsec(
				m_client->io_service,
				5,
				boost::bind<void>(*this, ec, bytes_transferred, value)
			);

			//  发送 join 信息,  加入频道.

			for ( i = 0; i < m_client->join_queue_.size(); i++)
			{
				BOOST_ASIO_CORO_YIELD async_send_line(
					m_client->join_queue_[i],
					boost::bind<void>(*this, _1, _2, value)
				);
			}

			if (! m_value->empty())
			{
				BOOST_ASIO_CORO_YIELD boost::asio::async_write(
					m_client->socket_,
					boost::asio::buffer(*m_value),
					boost::bind<void>(*this, _1, _2, value)
				);
				m_value->clear();
			}

			_coro_value = 0;
			m_client->messages_send_queue_.async_pop(
				boost::bind<void>(*this, ec, 0, _1)
			);
		}
		else if (m_client->connected_)
		{
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

			_coro_value = 0;
			m_client->messages_send_queue_.async_pop(
				boost::bind<void>(*this, ec, 0, _1)
			);
		}else {
			// 重新进入登录循环
			*m_value = value;

			// 等待 login 状态
			while (!m_client->connected_)
			{
				BOOST_ASIO_CORO_YIELD boost::delayedcallsec(
					m_client->get_io_service(), 2, boost::bind<void>(*this, ec, bytes_transferred, value));
			}
			// 然后进入登录过程.

			_coro_value = 0;
			m_client->io_service.post(
				boost::bind<void>(*this, ec, bytes_transferred, std::string(""))
			);
		}
	}
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
