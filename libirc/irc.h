/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2012 InvXp <invidentssc@hotmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
RFC Protocol
http://www.irchelp.org/irchelp/rfc/rfc.html
*/

#pragma once
#include <vector>
#include <string>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>

#include "boost/coro/coro.hpp"

struct IrcMsg
{
    std::string whom;
    std::string from;
    std::string locate;
    std::string msg;
};

typedef boost::function<void(const IrcMsg& msg)> privmsg_cb;

class IrcClient
{
public:
    IrcClient(boost::asio::io_service &io_service,const std::string& user,const std::string& user_pwd="",const std::string& server="irc.freenode.net", const std::string& port="6667",const unsigned int max_retry_count=1000);

public:
    void login(const privmsg_cb &cb){
		cb_=cb;
	}
    void join(const std::string& ch,const std::string &pwd="");
    void chat(const std::string whom,const std::string msg);
    void send_command(const std::string& cmd){
		send_request(cmd);
	}
	void oper(const std::string& user,const std::string& pwd){
		send_request("OPER "+user+" "+pwd);
	}

private:
    void handle_read_request(const boost::system::error_code& err, std::size_t readed);
	void handle_write_request(const boost::system::error_code& err, std::size_t bytewrited, boost::coro::coroutine coro);
    void handle_connect_request(const boost::system::error_code& err);
    void send_request(const std::string& msg){
		std::string data=msg+"\r\n";
		send_data(data.c_str(),data.length());
	}
    void send_data(const char* data,const size_t len);
    void process_request(boost::asio::streambuf& buf);
    void connect();
    void relogin();
    void relogin_delayed();
    void connected();

private:
	boost::asio::io_service &io_service;
    boost::asio::ip::tcp::socket    socket_;
    boost::asio::streambuf          request_;
    boost::asio::streambuf          response_;
    privmsg_cb                      cb_;
    std::string                     user_;
    std::string                     pwd_;
    std::string                     server_;
    std::string                     port_;
    bool                            login_;
    std::vector<std::string>        msg_queue_;
    std::vector<std::string>        join_queue_;
    unsigned int                    retry_count_;
    const unsigned int              c_retry_cuont;
    bool insending;
};
