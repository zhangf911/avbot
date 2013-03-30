#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>

#include "avproxy.hpp"
#include "boost/timedcall.hpp"
#include "boost/coro/yield.hpp"

#include "irc.h"

IrcClient::IrcClient(boost::asio::io_service &_io_service,const std::string& user,const std::string& user_pwd,const std::string& server, const std::string& port,const unsigned int max_retry)
	:io_service(_io_service), cb_(0), socket_(io_service),user_(user),pwd_(user_pwd),server_(server),port_(port),retry_count_(max_retry),c_retry_cuont(max_retry),login_(false), insending(false)
{
    connect();
}

void IrcClient::connect()
{
	using namespace boost::asio::ip;
	avproxy::async_proxy_connect(avproxy::autoproxychain(socket_, tcp::resolver::query(server_,port_)),
			boost::bind(&IrcClient::handle_connect_request, this, boost::asio::placeholders::error) );
}

void IrcClient::connected()
{
    
    if (!pwd_.empty())
        send_request("PASS "+pwd_);

    send_request("NICK "+user_);
    send_request("USER "+user_+ " 0 * "+user_);

    login_=true;
    retry_count_=c_retry_cuont;

    BOOST_FOREACH(std::string& str,msg_queue_)
    {
        join_queue_.push_back(str);
        send_request(str);
    }
    
    msg_queue_.clear();
    
}

void IrcClient::chat(const std::string whom,const std::string msg)
{
	std::vector<std::string> msgs;
	boost::split(msgs, msg, boost::is_any_of("\r\n"));
	BOOST_FOREACH(std::string _msg,  msgs)
	{
		if (_msg.length() > 0)
			send_request("PRIVMSG "+whom+" :"+_msg);
	}
}

void IrcClient::join(const std::string& ch,const std::string &pwd)
{
    std::string msg;

    pwd.empty()?msg="JOIN "+ch:msg="JOIN "+ch+" "+pwd;

    if (!login_)
        msg_queue_.push_back(msg);
    else
    {
        send_request(msg);
        join_queue_.push_back(msg);
    }
}

void IrcClient::relogin()
{

	login_=false;
	boost::system::error_code ec;
	socket_.close(ec);
	retry_count_--;

	if (retry_count_<=0)
	{
		std::cout<<"Irc Server has offline!!!"<<  std::endl;;
		return;
	}

	std::cout << "retry in 10s..." <<  std::endl;
	socket_.close();

	boost::delayedcallsec(io_service, 10, boost::bind(&IrcClient::relogin_delayed, this));
}

void IrcClient::relogin_delayed()
{
    BOOST_FOREACH (std::string& str,join_queue_)
		msg_queue_.push_back(str);
	join_queue_.clear();
	connect();
}

void IrcClient::process_request(boost::asio::streambuf& buf)
{
    std::string req;

    std::istream is(&buf);
    is.unsetf(std::ios_base::skipws);

    while ( req.clear(), std::getline(is, req), ( !is.eof() && ! req.empty()))
    {
        if (req.substr(0,4)=="PING")
        {
            send_request("PONG "+req.substr(6,req.length()-8));
            continue;
        }

        size_t pos=req.find(" PRIVMSG ")+1;

        if (!pos)
            continue;

        std::string msg=req;
        IrcMsg m;

        pos=msg.find("!")+1;
        if (!pos)
            continue;

        m.whom=msg.substr(1,pos-2);

        msg=msg.substr(pos);

        pos=msg.find(" PRIVMSG ")+1;
        if (!pos)
            continue;

        m.locate=msg.substr(0,pos-1);

        msg=msg.substr(pos);

        pos=msg.find("PRIVMSG ")+1;

        if (!pos)
            continue;

        msg=msg.substr(strlen("PRIVMSG "));

        pos=msg.find(" ")+1;

        if (!pos)
            continue;

        m.from=msg.substr(0,pos-1);

        msg=msg.substr(pos);

        pos=msg.find(":")+1;

        if (!pos)
            continue;

        m.msg=msg.substr(pos,msg.length()-pos);

        cb_(m);
    }
}

void IrcClient::handle_read_request(const boost::system::error_code& err, std::size_t readed)
{
    if (err)
    {
		response_.consume(response_.size());
		request_.consume(request_.size());
        relogin();
#ifdef DEBUG
        std::cout << "Error: " << err.message() << "\n";
#endif
	}else{
        boost::asio::async_read_until(socket_, response_, "\r\n",
            boost::bind(&IrcClient::handle_read_request, this, _1, _2)
		);

        process_request(response_);
    }
}

void IrcClient::handle_write_request(const boost::system::error_code& err, std::size_t bytewrited, boost::coro::coroutine coro)
{
	std::istream  req(&request_);
	std::string line;

	reenter(&coro)
	{
		if (!err)
		{
			if (request_.size()){
				std::getline(req, line);
				if (line.length() > 1){

					line.append("\n");

					_yield  boost::asio::async_write(socket_, boost::asio::buffer(line),
							boost::bind(&IrcClient::handle_write_request, this, _1, _2, coro)
					);

					boost::delayedcallms(io_service, 450, boost::bind(&IrcClient::handle_write_request, this, boost::system::error_code(), 0,  boost::coro::coroutine()));

				}else{
					_yield boost::delayedcallms(io_service, 5, boost::bind(&IrcClient::handle_write_request, this, boost::system::error_code(), 0,coro));
				}

			}else{
				insending = false;
			}
		} else {
			insending = false;
			relogin();
	#ifdef DEBUG        
			std::cout << "Error: " << err.message() << "\n";
	#endif
		}	
	}
}

void IrcClient::handle_connect_request(const boost::system::error_code& err)
{
    if (!err)
    {
        connected();

        boost::asio::async_read_until(socket_, response_, "\r\n",
            boost::bind(&IrcClient::handle_read_request, this,
            boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
    }
    else if (err != boost::asio::error::eof)
    {
		io_service.post( boost::bind(&IrcClient::relogin, this));

// #ifdef DEBUG
        std::cerr << "irc: connect error: " << err.message() << std::endl;
// #endif
    }
}

void IrcClient::send_data(const char* data,const size_t len)
{
    std::string msg;
    msg.append(data,len);

    std::ostream request_stream(&request_);
    request_stream << msg;

    if (!insending){
		insending = true;

		boost::asio::async_write(socket_, boost::asio::null_buffers(),
			boost::bind(&IrcClient::handle_write_request, this,
				boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, boost::coro::coroutine()));
	}
}
