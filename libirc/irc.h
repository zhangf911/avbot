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

#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/async_coro_queue.hpp>

#include "avproxy.hpp"
#include "boost/timedcall.hpp"

namespace irc{
struct irc_msg
{
	std::string whom;
	std::string from;
	std::string locate;
	std::string msg;
};

typedef boost::function<void( irc_msg msg )> privmsg_cb;

class client
{
public:
	client( boost::asio::io_service &_io_service, const std::string& user, const std::string& user_pwd = "", const std::string& server = "irc.freenode.net", const std::string& port = "6667", const unsigned int max_retry_count = 1000 )
		: io_service( _io_service ), cb_( 0 ), socket_( io_service ), user_( user ), pwd_( user_pwd ), server_( server ), port_( port ), retry_count_( max_retry_count ), c_retry_cuont( max_retry_count ), login_( false ), insending( false )
	{
		io_service.post(boost::bind(&client::connect, this));
	}

public:
	void login( const privmsg_cb &cb )
	{
		cb_ = cb;
	}

	void join( const std::string& ch, const std::string &pwd = "" )
	{
		std::string msg;

		pwd.empty() ? msg = "JOIN " + ch : msg = "JOIN " + ch + " " + pwd;

		if( !login_ )
			msg_queue_.push_back( msg );
		else
		{
			send_request( msg );
			join_queue_.push_back( msg );
		}
	}

	void chat( const std::string whom, const std::string msg )
	{
		std::vector<std::string> msgs;
		boost::split( msgs, msg, boost::is_any_of( "\r\n" ) );
		BOOST_FOREACH( std::string _msg,  msgs )
		{
			if( _msg.length() > 0 )
				send_request( "PRIVMSG " + whom + " :" + _msg );
		}
	}

	void send_command( const std::string& cmd )
	{
		send_request( cmd );
	}

	void oper( const std::string& user, const std::string& pwd )
	{
		send_request( "OPER " + user + " " + pwd );
	}

private:
	void handle_read_request( const boost::system::error_code& err, std::size_t readed )
	{
		if( err )
		{
			response_.consume( response_.size() );
			request_.consume( request_.size() );
			relogin();
#ifdef DEBUG
			std::cout << "Error: " << err.message() << "\n";
#endif
		}
		else
		{
			boost::asio::async_read_until( socket_, response_, "\r\n",
										   boost::bind( &client::handle_read_request, this, _1, _2 )
										 );

			process_request( response_ );
		}
	}


	void handle_write_request( const boost::system::error_code& err, std::size_t bytewrited, boost::asio::coroutine coro )
	{
		std::istream  req( &request_ );
		line.clear();

		reenter( &coro )
		{
			if( !err )
			{
				if( request_.size() )
				{
					std::getline( req, line );

					if( line.length() > 1 )
					{

						line.append( "\n" );

						yield  boost::asio::async_write( socket_, boost::asio::buffer( line ),
														  boost::bind( &client::handle_write_request, this, _1, _2, coro )
														);

						boost::delayedcallms( io_service, 450, boost::bind( &client::handle_write_request, this, boost::system::error_code(), 0,  boost::asio::coroutine() ) );

					}
					else
					{
						yield boost::delayedcallms( io_service, 5, boost::bind( &client::handle_write_request, this, boost::system::error_code(), 0, coro ) );
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

	void handle_connect_request( const boost::system::error_code& err )
	{
		if( !err )
		{
			connected();

			boost::asio::async_read_until( socket_, response_, "\r\n",
										   boost::bind( &client::handle_read_request, this,
												   boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred ) );
		}
		else if( err != boost::asio::error::eof )
		{
			io_service.post( boost::bind( &client::relogin, this ) );

#ifdef DEBUG
			std::cerr << "irc: connect error: " << err.message() << std::endl;
#endif
		}
	}

	void send_request( const std::string& msg )
	{
		std::string data = msg + "\r\n";
		send_data( data.c_str(), data.length() );
	}

	void send_data( const char* data, const size_t len )
	{
		std::string msg;
		msg.append( data, len );

		std::ostream request_stream( &request_ );
		request_stream << msg;

		if( !insending )
		{
			insending = true;

			boost::asio::async_write( socket_, boost::asio::null_buffers(),
									  boost::bind( &client::handle_write_request, this,
												   boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred, boost::asio::coroutine() ) );
		}
	}

	void process_request( boost::asio::streambuf& buf )
	{
		std::string req;

		std::istream is( &buf );
		is.unsetf( std::ios_base::skipws );

		while( req.clear(), std::getline( is, req ), ( !is.eof() && ! req.empty() ) )
		{
#ifdef DEBUG
            std::cout << req << std::endl;
#endif
            //add auto modify a nick
            if( req.find( "Nickname is already in use." )!=std::string::npos)
            {
                user_+="_";
                send_request( "NICK " + user_ );
                send_request( "USER " + user_ + " 0 * " + user_ );

                BOOST_FOREACH( std::string & str, join_queue_ )
                    send_request( str );

                continue;
            }
			if( req.substr( 0, 4 ) == "PING" )
			{
				send_request( "PONG " + req.substr( 6, req.length() - 8 ) );
				continue;
			}

			size_t pos = req.find( " PRIVMSG " ) + 1;

			if( !pos )
				continue;

			std::string msg = req;
			irc_msg m;

			pos = msg.find( "!" ) + 1;

			if( !pos )
				continue;

			m.whom = msg.substr( 1, pos - 2 );

			msg = msg.substr( pos );

			pos = msg.find( " PRIVMSG " ) + 1;

			if( !pos )
				continue;

			m.locate = msg.substr( 0, pos - 1 );

			msg = msg.substr( pos );

			pos = msg.find( "PRIVMSG " ) + 1;

			if( !pos )
				continue;

			msg = msg.substr( strlen( "PRIVMSG " ) );

			pos = msg.find( " " ) + 1;

			if( !pos )
				continue;

			m.from = msg.substr( 0, pos - 1 );

			msg = msg.substr( pos );

			pos = msg.find( ":" ) + 1;

			if( !pos )
				continue;

			m.msg = msg.substr( pos, msg.length() - pos );

			cb_( m );
		}
	}

	void connect()
	{
		using namespace boost::asio::ip;
		avproxy::async_proxy_connect( avproxy::autoproxychain( socket_, tcp::resolver::query( server_, port_ ) ),
									  boost::bind( &client::handle_connect_request, this, boost::asio::placeholders::error ) );
	}

	void relogin()
	{

		login_ = false;
		boost::system::error_code ec;
		socket_.close( ec );
		retry_count_--;

		if( retry_count_ <= 0 )
		{
			std::cout << "Irc Server has offline!!!" <<  std::endl;;
			return;
		}

		std::cout << "irc: retry in 10s..." <<  std::endl;
		socket_.close();

		boost::delayedcallsec( io_service, 10, boost::bind( &client::relogin_delayed, this ) );
	}

	void relogin_delayed()
	{
		msg_queue_.clear();
		BOOST_FOREACH( std::string & str, join_queue_ )
		msg_queue_.push_back( str );
		join_queue_.clear();
		connect();
	}
	void connected()
	{
		if( !pwd_.empty() )
			send_request( "PASS " + pwd_ );

		send_request( "NICK " + user_ );
		send_request( "USER " + user_ + " 0 * " + user_ );

		login_ = true;
		retry_count_ = c_retry_cuont;

		BOOST_FOREACH( std::string & str, msg_queue_ )
		{
			join_queue_.push_back( str );
			send_request( str );
		}

		msg_queue_.clear();

	}

private:
	boost::asio::io_service &       io_service;
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

	std::string line;

};

}
