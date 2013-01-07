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

#ifndef __XMPP_IMPL_H
#define __XMPP_IMPL_H

#include <boost/asio.hpp>
#include <gloox/client.h>
#include <gloox/messagehandler.h>
#include <gloox/connectionlistener.h>
#include <gloox/mucroomhandler.h>
#include <boost/scoped_ptr.hpp>
#include <boost/signal.hpp>

namespace XMPP {

class xmpp_impl : private gloox::MessageHandler, gloox::ConnectionListener, gloox::MUCRoomHandler
{
public:
	xmpp_impl(boost::asio::io_service & asio, std::string xmppuser, std::string xmpppasswd);
	void join(std::string roomjid);
	void on_room_message(boost::function<void (std::string xmpproom, std::string who, std::string message)> cb);
	void send_room_message(std::string xmpproom, std::string message);

private:
    virtual void handleMessage(const gloox::Message& msg, gloox::MessageSession* session = 0);

    virtual void onConnect();
    virtual void onDisconnect(gloox::ConnectionError e);
    virtual bool onTLSConnect(const gloox::CertInfo& info);

    virtual void handleMUCMessage(gloox::MUCRoom* room, const gloox::Message& msg, bool priv);
    virtual void handleMUCParticipantPresence(gloox::MUCRoom* room, const gloox::MUCRoomParticipant participant, const gloox::Presence& presence);
    virtual void handleMUCSubject(gloox::MUCRoom* room, const std::string& nick, const std::string& subject);
    virtual void handleMUCError(gloox::MUCRoom* room, gloox::StanzaError error);
    virtual void handleMUCInfo(gloox::MUCRoom* room, int features, const std::string& name, const gloox::DataForm* infoForm);
    virtual void handleMUCInviteDecline(gloox::MUCRoom* room, const gloox::JID& invitee, const std::string& reason);

    virtual void handleMUCItems(gloox::MUCRoom* room, const gloox::Disco::ItemList& items);
    virtual bool handleMUCRoomCreation(gloox::MUCRoom* room);

	void cb_handle_asio_read(const boost::system::error_code & error);
private:
	boost::asio::io_service & m_asio;
	std::string hostname;                                      // the host to connect
	std::string user, password;
	gloox::JID m_jid;
	gloox::Client m_client;
	std::vector<boost::shared_ptr<gloox::MUCRoom> >	m_rooms;
	boost::scoped_ptr<boost::asio::ip::tcp::socket> m_asio_socket;
	
	boost::signal <void (std::string xmpproom, std::string who, std::string message)> m_sig_room_message;
};

}
#endif // __XMPP_IMPL_H

