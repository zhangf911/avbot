/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2012  微蔡 <microcai@fedoraproject.org>
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

#ifndef XMPP_H
#define XMPP_H
#include <boost/asio.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <iksemel.h>

enum xmpp_state {
	XMPP_SATE_DISCONNTECTED,
	XMPP_SATE_CONNTECTED,
	XMPP_SATE_REQTLS,
	XMPP_SATE_REQEDTLS,
	XMPP_SATE_AUTHED,
};

class xmpp
{
	xmpp_state m_xmppstate;
public:
	xmpp(boost::asio::io_service & asio, std::string xmppuser, std::string xmpppasswd);

private:
	void cb_resolved(boost::shared_ptr<boost::asio::ip::tcp::resolver> resolver, boost::shared_ptr<boost::asio::ip::tcp::resolver::query> query, const boost::system::error_code & er, boost::asio::ip::tcp::resolver::iterator iterator);
	void cb_connected(const boost::system::error_code & er);
	void handle_firstread(const boost::system::error_code & er,size_t );
	void handle_tlsprocessed(const boost::system::error_code & er,size_t );
	void handle_tlshandshake(const boost::system::error_code & er);
	void handle_tlswrite(const boost::system::error_code & er,size_t n);
	void handle_tlsread(const boost::system::error_code & er,size_t n);
	
	int cb_iks_hook(int type, iks *node);
private:
	boost::asio::io_service & m_asio;
	boost::asio::ssl::context	m_sslcontext;
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_socket;// the socket to the host
	boost::asio::streambuf	m_readbuf;
	boost::asio::streambuf	m_writebuf;

	std::string hostname;                                      // the host to connect
	std::string user, password;
	std::string m_jabber_sid;
	iksparser* m_prs;
	friend int cb_iks_hook(void *user_data, int type, iks *node);
};

#endif // XMPP_H
