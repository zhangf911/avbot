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
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <gloox/message.h>
#include <gloox/mucroom.h>
#include <gloox/connectiontcpclient.h>

#include "xmpp_impl.h"

using namespace XMPP;

xmpp_impl::xmpp_impl(boost::asio::io_service& asio, std::string xmppuser, std::string xmpppasswd)
	:password(xmpppasswd), m_asio(asio), m_jid(xmppuser+"/avqqbot"), m_client(m_jid, xmpppasswd)
{
	std::vector<std::string> splited;
	boost::split(splited, xmppuser, boost::is_any_of("@"));
	user = splited[0];
	hostname = splited[1];

	m_client.registerConnectionListener(this);
	m_client.registerMessageHandler(this);
	m_client.connect(false);
	gloox::ConnectionTCPClient* con = static_cast<gloox::ConnectionTCPClient*>(m_client.connectionImpl());

	m_asio_socket.reset( new boost::asio::ip::tcp::socket(m_asio,boost::asio::ip::tcp::v4(), con->socket()));
	//use asio :)
	//boost::asio::async_read();
	m_asio_socket->async_read_some(boost::asio::null_buffers(), 
		boost::bind(&xmpp_impl::cb_handle_asio_read, this, boost::asio::placeholders::error)
	);
}

void xmpp_impl::join(std::string roomjid)
{
	gloox::JID roomnick(roomjid+"/qqbot");//"avplayer@im.linuxapp.org";
	boost::shared_ptr<gloox::MUCRoom> room( new  gloox::MUCRoom(&m_client, roomnick, this));
	m_rooms.push_back(room);
}

void xmpp_impl::cb_handle_asio_read(const boost::system::error_code& error)
{
	m_client.recv(0);
	std::cout << "on read" <<  std::endl;
	m_asio_socket->async_read_some(boost::asio::null_buffers(), 
		boost::bind(&xmpp_impl::cb_handle_asio_read, this, boost::asio::placeholders::error)
	);
}

void xmpp_impl::handleMessage(const gloox::Message& stanza, gloox::MessageSession* session)
{
	std::cout <<  __func__ <<  std::endl;
	gloox::Message msg( gloox::Message::Chat , stanza.from(), "hello world" );
    m_client.send( msg );
}

bool xmpp_impl::onTLSConnect(const gloox::CertInfo& info)
{
	return true;
}

void xmpp_impl::onConnect()
{
	BOOST_FOREACH(boost::shared_ptr<gloox::MUCRoom> room,  m_rooms)
	{
		room->join();
		room->getRoomInfo();
		room->getRoomItems();
	}
}

void xmpp_impl::onDisconnect(gloox::ConnectionError e)
{
}

void xmpp_impl::handleMUCError(gloox::MUCRoom* room, gloox::StanzaError error)
{
	error;
	std::cout <<  __func__ <<  std::endl;
}

void xmpp_impl::handleMUCInfo(gloox::MUCRoom* room, int features, const std::string& name, const gloox::DataForm* infoForm)
{
	std::cout <<  __func__ <<  std::endl;

}

void xmpp_impl::handleMUCInviteDecline(gloox::MUCRoom* room, const gloox::JID& invitee, const std::string& reason)
{
	std::cout <<  __func__ <<  std::endl;

}

void xmpp_impl::handleMUCItems(gloox::MUCRoom* room, const gloox::Disco::ItemList& items)
{
	std::cout <<  __func__ <<  std::endl;

}


void xmpp_impl::handleMUCMessage(gloox::MUCRoom* room, const gloox::Message& msg, bool priv)
{
	std::cout <<  " -- " <<  room->name() <<  " -- "<<  room->nick() <<  " -- "
	 <<  __func__ <<  "  " <<   msg.from().resource() <<  "  "  << 
   msg.body()<<  std::endl;
}

void xmpp_impl::handleMUCParticipantPresence(gloox::MUCRoom* room, const gloox::MUCRoomParticipant participant, const gloox::Presence& presence)
{
	std::cout <<  __func__ <<  std::endl;

}

bool xmpp_impl::handleMUCRoomCreation(gloox::MUCRoom* room)
{
	std::cout <<  __func__ <<  std::endl;
	return false;
}

void xmpp_impl::handleMUCSubject(gloox::MUCRoom* room, const std::string& nick, const std::string& subject)
{
	std::cout <<  __func__ <<  std::endl;

}

