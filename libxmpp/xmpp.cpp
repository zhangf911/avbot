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
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include "xmpp.h"

using namespace boost::asio;

void xmpp::cb_resolved(boost::shared_ptr<ip::tcp::resolver> resolver,
	boost::shared_ptr<ip::tcp::resolver::query>	query,
	const boost::system::error_code& er, ip::tcp::resolver::iterator iterator)
{
	iterator->endpoint().port();
	if (!er)
		async_connect(m_socket.lowest_layer(),  iterator, boost::bind(&xmpp::cb_connected, this, placeholders::error) );
}

void xmpp::cb_connected(const boost::system::error_code& er)
{
	if (!er){
		m_socket.async_handshake(ssl::stream_base::client, boost::bind(&xmpp::cb_ssl_connected, this, placeholders::error));
	}
}

void xmpp::cb_ssl_connected(const boost::system::error_code& er)
{

}


xmpp::xmpp(boost::asio::io_service& asio, std::string xmppuser, std::string xmpppasswd)
  :password(xmpppasswd), m_asio(asio),  m_socket(asio, m_sslcontext), m_sslcontext(ssl::context::sslv23_client)
{
	std::vector<std::string> splited;
	boost::split(splited, xmppuser, boost::is_any_of("@"));
	user = splited[0];
	hostname = splited[1];
	//解析 xmppuser 获得服务器
	boost::shared_ptr<ip::tcp::resolver> resolver(new ip::tcp::resolver(asio));
	boost::shared_ptr<ip::tcp::resolver::query> query(new ip::tcp::resolver::query(hostname, "5222"));
	resolver->async_resolve(*query, boost::bind(&xmpp::cb_resolved, this, resolver, query,  placeholders::error, boost::asio::placeholders::iterator));
	


}
