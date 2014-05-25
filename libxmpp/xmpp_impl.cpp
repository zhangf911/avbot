/*
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
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <gloox/message.h>
#include <gloox/mucroom.h>

#include <boost/lexical_cast.hpp>

#include "boost/timedcall.hpp"
#include "boost/avproxy.hpp"

#include "xmpp_impl.hpp"

using namespace gloox;

namespace xmppimpl{

gloox::ConnectionError xmpp_asio_connector::connect()
{
	boost::system::error_code ec;
	m_state = gloox::StateConnecting;

	avproxy::async_proxy_connect(
		avproxy::autoproxychain( m_socket, m_query ),
		//boost::bind( &xmpp_asio_connector::cb_handle_connecting, this, _1 )
		(*m_yield_context)[ec]
	);

	// 然后到这里就完成连接了
	if (!ec)
	{
		m_state = gloox::StateConnected;
		m_handler->handleConnect(this);
		return gloox::ConnNoError;
	}
	else
	{
		m_state = gloox::StateDisconnected;
		std::cout << ec.message() << std::endl;
		return gloox::ConnConnectionRefused;
	}
}

void xmpp_asio_connector::disconnect()
{
	m_state = gloox::StateDisconnected;
	boost::system::error_code ec;
	m_socket.close( ec );
}

gloox::ConnectionError xmpp_asio_connector::receive()
{
	while (recv(-1) == ConnNoError)
	{
	}
	m_handler->handleDisconnect(this, ConnNoError);
	return ConnNoError;
}

gloox::ConnectionError xmpp_asio_connector::recv( int timeout )
{
	boost::system::error_code ec;
	std::size_t bytes_transferred = m_socket.async_read_some(boost::asio::buffer(m_readbuf), (*m_yield_context)[ec]);
	// should not be called
	if (!ec)
	{
		std::string data(m_readbuf.begin(), bytes_transferred);

		this->m_handler->handleReceivedData(this, data);
		return ConnNoError;
	}
	return ConnIoError;
}

bool xmpp_asio_connector::send( const std::string& data )
{
	boost::system::error_code ec;
	boost::asio::async_write(m_socket, boost::asio::buffer(data), boost::asio::transfer_all(), (*m_yield_context)[ec]);
	return ! ec;
}

xmpp_asio_connector::xmpp_asio_connector(boost::shared_ptr<xmpp> _xmpp,
	gloox::ConnectionDataHandler* cdh, boost::asio::ip::tcp::resolver::query _query)
	: m_xmpp(_xmpp)
	, gloox::ConnectionBase( cdh )
	, io_service(_xmpp->get_ioservice())
	, m_socket( io_service )
	, m_query( _query )
{
}

void xmpp_asio_connector::cleanup()
{
	boost::system::error_code ec;
	// 清理资源
	this->m_socket.close(ec);
	m_xmpp.reset();
}

void xmpp_asio_connector::coroutine_start(boost::asio::yield_context this_coro_context)
{
	// 协程在这里，开始连接！
	this->m_yield_context = &this_coro_context;
	this->m_xmpp->m_client.connect(true);
	// 连接木有了

}

xmpp::xmpp( boost::asio::io_service& asio, std::string xmppuser, std::string xmpppasswd, std::string xmppserver, std::string xmppnick )
	: io_service( asio ), m_jid( xmppuser + "/" + xmppnick ), m_client( m_jid, xmpppasswd ), m_xmppnick( xmppnick )
{
	m_client.registerConnectionListener( this );
	m_client.registerMessageHandler( this );

	if( !xmppserver.empty() ) {
		std::vector<std::string> splited;
		// 设定服务器.
		boost::split( splited, xmppserver, boost::is_any_of( ":" ) );
		m_client.setServer( splited[0] );

		if( splited.size() == 2 )
			m_client.setPort( boost::lexical_cast<int>( splited[1] ) );
	}
}

void xmpp::start()
{
	std::string xmppclientport ;

	if( m_client.port() == -1 )
		xmppclientport = "xmpp-client";
	else
		xmppclientport = boost::lexical_cast<std::string>( m_client.port() );

	avproxy::proxy::tcp::query query( m_client.server(), xmppclientport );
	xmpp_asio_connector * new_tcp_connector = new xmpp_asio_connector(shared_from_this(), &m_client, query);
	m_client.setConnectionImpl(new_tcp_connector);
	boost::asio::spawn(io_service, boost::bind(&xmpp_asio_connector::coroutine_start, new_tcp_connector, _1));
}

void xmpp::join( std::string roomjid )
{
	gloox::JID roomnick( roomjid + "/" + m_xmppnick ); //"avplayer@im.linuxapp.org";
	boost::shared_ptr<gloox::MUCRoom> room( new  gloox::MUCRoom( &m_client, roomnick, this ) );
	m_rooms.push_back( room );
}

void xmpp::on_room_message( boost::function<void ( std::string xmpproom, std::string who, std::string message )> cb )
{
	m_sig_room_message.connect( cb );
}

void xmpp::send_room_message( std::string xmpproom, std::string message )
{
	//查找啊.
	BOOST_FOREACH( boost::shared_ptr<gloox::MUCRoom> room, m_rooms ) {
		if( room->name() == xmpproom ) {
			room->send( message );
		}
	}
}

void xmpp::handleMessage( const gloox::Message& stanza, gloox::MessageSession* session )
{
	gloox::Message msg( gloox::Message::Chat , stanza.from(), "hello world" );
	m_client.send( msg );
}

void xmpp::handleMUCMessage( gloox::MUCRoom* room, const gloox::Message& msg, bool priv )
{
	if( !msg.from().resource().empty() && msg.from().resource() != room->nick() )
		m_sig_room_message( room->name(), msg.from().resource(), msg.body() );
}

bool xmpp::onTLSConnect( const gloox::CertInfo& info )
{
	return true;
}

void xmpp::onConnect()
{
	BOOST_FOREACH( boost::shared_ptr<gloox::MUCRoom> room,  m_rooms ) {
		room->join();
		room->getRoomInfo();
		room->getRoomItems();
	}
}

void xmpp::onDisconnect( gloox::ConnectionError e )
{
	std::cerr << "xmpp disconnected: " <<  e << "reconnectiong..." <<  std::endl;
	boost::delayedcallsec( io_service, 10 + rand() % 10, boost::bind( &xmpp::start, this ) );
	return ;
}

static std::string randomname( std::string m_xmppnick )
{
	return boost::str( boost::format( "%s%X" ) % m_xmppnick % rand() );
}

void xmpp::handleMUCError( gloox::MUCRoom* room, gloox::StanzaError error )
{
	if( error == gloox::StanzaErrorConflict ) {
		// 出现名字冲突，使用一个随机名字.
		room->setNick( randomname( m_xmppnick ) );
		room->join();
	}
}

void xmpp::handleMUCInfo( gloox::MUCRoom* room, int features, const std::string& name, const gloox::DataForm* infoForm )
{
}

void xmpp::handleMUCInviteDecline( gloox::MUCRoom* room, const gloox::JID& invitee, const std::string& reason )
{
}

void xmpp::handleMUCItems( gloox::MUCRoom* room, const gloox::Disco::ItemList& items )
{
}


void xmpp::handleMUCParticipantPresence( gloox::MUCRoom* room, const gloox::MUCRoomParticipant participant, const gloox::Presence& presence )
{
}

bool xmpp::handleMUCRoomCreation( gloox::MUCRoom* room )
{
	return true;
}

void xmpp::handleMUCSubject( gloox::MUCRoom* room, const std::string& nick, const std::string& subject )
{
}

}
