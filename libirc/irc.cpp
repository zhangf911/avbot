#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>

#include "boost/connector.hpp"
#include "boost/timedcall.hpp"

#include "irc.h"

IrcClient::IrcClient(boost::asio::io_service &_io_service,const std::string& user,const std::string& user_pwd,const std::string& server, const std::string& port,const unsigned int max_retry)
	:io_service(_io_service), cb_(0), socket_(io_service),user_(user),pwd_(user_pwd),server_(server),port_(port),retry_count_(max_retry),c_retry_cuont(max_retry),login_(false)
{
    response_.prepare(512*1024);
    connect();
}

IrcClient::~IrcClient()
{

}

void IrcClient::connect()
{
    boost::asio::ip::tcp::resolver::query query(server_,port_);
 	boost::shared_ptr<boost::asio::ip::tcp::resolver>  resolver(new boost::asio::ip::tcp::resolver(io_service));

	boost::async_connect(socket_, query, boost::bind(&IrcClient::handle_connect_request, this,
            boost::asio::placeholders::error) );
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

void IrcClient::oper(const std::string& user,const std::string& pwd)
{
    send_request("OPER "+user+" "+pwd);
}

void IrcClient::chat(const std::string& whom,const std::string& msg)
{
    send_request("PRIVMSG "+whom+" :"+msg);
}

void IrcClient::login(const privmsg_cb &cb)
{
    cb_=cb;
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

    std::istream is(&buf);
    is.unsetf(std::ios_base::skipws);
    std::string longreg(last_buf_);
    longreg.append(std::istream_iterator<char>(is), std::istream_iterator<char>());

#ifdef DEBUG
    std::cout << longreg;
#endif

    size_t leave=longreg.rfind("\r\n")+2;

    if (leave!=longreg.length())
    {
        last_buf_=longreg.substr(leave,longreg.length()-leave);
        longreg=longreg.substr(0,leave);
    }else
        last_buf_.clear();

    std::vector<std::string> vec;
    boost::split(vec,longreg,boost::algorithm::is_any_of("\r\n"),boost::algorithm::token_compress_on);

    BOOST_FOREACH (std::string& req,vec)
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
    if (!err)
    {
        process_request(response_);

        boost::asio::async_read_until(socket_, response_, "\r\n",
            boost::bind(&IrcClient::handle_read_request, this,
            boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
    }
    else if (err != boost::asio::error::eof)
    {
        relogin();
#ifdef DEBUG
        std::cout << "Error: " << err.message() << "\n";
#endif
    }

}

void IrcClient::handle_write_request(const boost::system::error_code& err, std::size_t bytewrited)
{
    if (!err)
	{
        request_.consume(bytewrited);		
        if (request_.size())			
            boost::asio::async_write(socket_,
            request_,
            boost::bind(&IrcClient::handle_write_request, this,boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)); 
    }    
    else    
    {
        relogin();
#ifdef DEBUG        
        std::cout << "Error: " << err.message() << "\n";
#endif
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

#ifdef DEBUG
        std::cout << "Error: " << err.message() << "\n";
#endif
    }
}

void IrcClient::send_command(const std::string& cmd)
{
    send_request(cmd);
}

void IrcClient::send_data(const char* data,const size_t len)
{
    std::string msg;
    msg.append(data,len);

    std::ostream request_stream(&request_);
    request_stream << msg;

    boost::asio::async_write(socket_, request_,
        boost::bind(&IrcClient::handle_write_request, this,
        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));

}
void IrcClient::send_request(const std::string& msg)
{
    std::string data=msg+"\r\n";
    send_data(data.c_str(),data.length());
}