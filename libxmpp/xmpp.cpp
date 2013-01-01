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
#include "iksemel.h"

using namespace boost::asio;

int cb_iks_hook(void *user_data, int type, iks *node)
{
	xmpp * this_ = reinterpret_cast<xmpp*>(user_data);
	return this_->cb_iks_hook(type, node);
}


void xmpp::cb_resolved(boost::shared_ptr<ip::tcp::resolver> resolver,
	boost::shared_ptr<ip::tcp::resolver::query>	query,
	const boost::system::error_code& er, ip::tcp::resolver::iterator iterator)
{
	iterator->endpoint().port();
	if (!er)
		async_connect(m_socket.lowest_layer(),  iterator, boost::bind(&xmpp::cb_connected, this, placeholders::error) );
}

int xmpp::cb_iks_hook(int type, iks* node)
{
	if (type == IKS_NODE_START){
		m_jabber_sid = iks_find_attrib (node, "id");
		std::cout << "server return jabber id" << m_jabber_sid << std::endl;
	}else if(type == IKS_NODE_NORMAL){
		
	}
	return 0;
}

void xmpp::cb_connected(const boost::system::error_code& er)
{
	int ret;
	m_prs = iks_stream_new(IKS_NS_CLIENT, this, &::cb_iks_hook);
	ret = iks_connect_fd(m_prs, m_socket.next_layer().native_handle());
	ret = iks_send_header(m_prs, hostname.c_str());
	
	m_xmppstate = XMPP_SATE_CONNTECTED;
	//接收服务器返回的第一波数据，主要是 sid
	m_socket.next_layer().async_read_some(m_readbuf.prepare(4096), boost::bind(&xmpp::handle_firstread,this,placeholders::error,placeholders::bytes_transferred));
}

void xmpp::handle_firstread ( const boost::system::error_code& er, size_t n)
{
	m_readbuf.commit(n);

	iks_parse(m_prs,buffer_cast<const char*>(m_readbuf.data()), n, 0);
	m_readbuf.consume(n);

	m_xmppstate = XMPP_SATE_REQTLS;
	iks_send_raw (m_prs, "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>");
	
	//处理 proceed, 然后就可以 ssl.async_handshake 了.
	m_socket.next_layer().async_read_some(m_readbuf.prepare(4096), boost::bind(&xmpp::handle_tlsprocessed,this,placeholders::error,placeholders::bytes_transferred));	
}

void xmpp::handle_tlsprocessed ( const boost::system::error_code& er, size_t n)
{
	// connect the recv function	m_socket.async_read_some(null_buffers(), boost::bind(iks_recv,m_prs, 0));
	//接收服务器返回的第二波数据，主要是 starttls 后的 proceed
	m_readbuf.commit(n);
	{
		std::string buf;
		buf.assign(buffer_cast<const char*>(m_readbuf.data()),n);
		std::cout << buf << std::endl;
	}
	iks_parse(m_prs,buffer_cast<const char*>(m_readbuf.data()), n, 0);
	m_readbuf.consume(n);
	
	m_socket.set_verify_mode(ssl::verify_none);
	
	m_socket.async_handshake(ssl::stream_base::client, boost::bind(&xmpp::handle_tlshandshake, this, placeholders::error));
}

void xmpp::handle_tlshandshake ( const boost::system::error_code& er)
{
	ikstack* iksstack =  iks_stack_new(1024, 4096);
	iksid * sid = iks_id_new(iksstack, "qqbot@linuxapp.org/qqbot");
	iks* node =  iks_make_auth(sid, password.c_str(), m_jabber_sid.c_str());
	
	std::string xmlstr = iks_string (iks_stack (node), node);
	iks_stack_delete(iksstack);
	iks_delete(node);

	// 发送 jabber 登录包.
	m_writebuf.sputn(xmlstr.data(),xmlstr.length());
	std::cout << xmlstr << std::endl;
	async_write(m_socket, m_writebuf, boost::bind(&xmpp::handle_tlswrite, this, placeholders::error, placeholders::bytes_transferred));
	m_socket.async_read_some(m_readbuf.prepare(4096), boost::bind(&xmpp::handle_tlsread, this, placeholders::error, placeholders::bytes_transferred));
}

void xmpp::handle_tlswrite ( const boost::system::error_code& er, size_t n )
{
	m_writebuf.consume(n);
	if(er)
		std::cout << er.message() << std::endl;
}

void xmpp::handle_tlsread ( const boost::system::error_code& er, size_t n )
{
	m_readbuf.commit(n);
	{
		std::string buf;
		buf.assign(buffer_cast<const char*>(m_readbuf.data()),n);
		std::cout << buf << std::endl;
	}
	iks_parse(m_prs,buffer_cast<const char*>(m_readbuf.data()), n, 0);
	m_readbuf.consume(n);
}

xmpp::xmpp(boost::asio::io_service& asio, std::string xmppuser, std::string xmpppasswd)
	:m_xmppstate(XMPP_SATE_DISCONNTECTED), password(xmpppasswd), m_asio(asio),  m_socket(asio,m_sslcontext),m_sslcontext(asio,ssl::context::sslv23_client)
{
	std::vector<std::string> splited;
	boost::split(splited, xmppuser, boost::is_any_of("@"));
	user = splited[0];
	hostname = splited[1];
	//解析 xmppuser 获得服务器.
	boost::shared_ptr<ip::tcp::resolver> resolver(new ip::tcp::resolver(asio));
	boost::shared_ptr<ip::tcp::resolver::query> query(new ip::tcp::resolver::query(hostname, "xmpp-client"));
	resolver->async_resolve(*query, boost::bind(&xmpp::cb_resolved, this, resolver, query,  placeholders::error, boost::asio::placeholders::iterator));
}
