#include "irc.h"

IrcClient::IrcClient(boost::asio::io_service &io_service,const std::string& user,const std::string& user_pwd,const std::string& server, const std::string& port,const unsigned int max_retry):
cb_(0),resolver_(io_service),socket_(io_service),
user_(user),pwd_(user_pwd),
server_(server),port_(port),
retry_count_(max_retry),
c_retry_cuont(max_retry),
login_(false)
{
    connect();
}

IrcClient::~IrcClient()
{

}

void IrcClient::connect()
{
    boost::asio::ip::tcp::resolver::query query(server_,port_);
    boost::system::error_code ec;
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query,ec);

    if (!ec)
    {
        boost::asio::async_connect(socket_, endpoint_iterator,
            boost::bind(&IrcClient::handle_connect_request, this,
            boost::asio::placeholders::error));
    }
    else
    {
        relogin();
#ifdef DEBUG
        std::cout << "Error: " << ec.message() << "\n";
#endif
    }
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
    socket_.cancel();
    socket_.close();
    retry_count_--;

    if (retry_count_<=0)
    {
        std::cout<<"Irc Server has offline!!!";
        return;
    }

    boost::thread::sleep(boost::get_system_time() + boost::posix_time::seconds(10));

    std::vector<std::string>::iterator it=join_queue_.begin();
    for (it;it!=join_queue_.end();it++)
        msg_queue_.push_back(*it);
    join_queue_.clear();

    connect();

}

void IrcClient::process_request(boost::asio::streambuf& buf)
{

    std::istream is(&buf);
    is.unsetf(std::ios_base::skipws);
    std::string longreg;
    longreg.append(std::istream_iterator<char>(is), std::istream_iterator<char>());

#ifdef DEBUG
    std::cout << longreg;
#endif

    std::vector<std::string> vec;
    std::vector<char> split;
    split.push_back('\n');
    boost::split(vec,longreg,boost::algorithm::is_any_of(split));
    
    std::vector<std::string>::iterator it=vec.begin();

    for (it;it!=vec.end();it++)
    {
        std::string req=*it;

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

        m.msg=msg.substr(pos,msg.length()-2-pos);

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
        if (!pwd_.empty())
            send_request("PASS "+pwd_);

        send_request("NICK "+user_);
        send_request("USER "+user_+ " 0 * "+user_);

        login_=true;
        retry_count_=c_retry_cuont;
        if (msg_queue_.size())
        {
            if (msg_queue_.size())
            {
                std::vector<std::string>::iterator it=msg_queue_.begin();
                for (it;it!=msg_queue_.end();it++)
                {
                    join_queue_.push_back(*it);  
                    send_request(*it);
                }
            }
            msg_queue_.clear();
        }
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

void IrcClient::send_command(const std::string& cmd)
{
    send_request(cmd);
}

void IrcClient::send_request(const std::string& msg)
{
    std::ostream request_stream(&request_);
    request_stream << msg+"\r\n";

    boost::asio::async_write(socket_, request_,
        boost::bind(&IrcClient::handle_write_request, this,
        boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}