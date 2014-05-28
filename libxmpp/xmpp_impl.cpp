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

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/invoke_wrapper.hpp>
#include "boost/timedcall.hpp"
#include "boost/avproxy.hpp"
#include "boost/logging.hpp"
#include "xmpp_impl.hpp"

using namespace gloox;

namespace xmppimpl{

template<class CoroQueue, class AsioAsyncStream, class Handler>
struct async_wait_pop_or_read_op
{
	async_wait_pop_or_read_op(CoroQueue & queue, AsioAsyncStream & stream, Handler handler)
		: m_queue(&queue)
		, m_stream(&stream)
		, m_handler(handler)
	{
	}

	void start()
	{
		boost::invoke_wrapper::invoke_once<void(boost::system::error_code, std::size_t)> handler_wrapper(*this);
		m_queue->async_wait(boost::bind(handler_wrapper, _1, 1));
		boost::asio::async_read(*m_stream, boost::asio::null_buffers(), handler_wrapper);
	}

	void operator()(boost::system::error_code ec, std::size_t bytes_transfered)
	{
		if (bytes_transfered == 1)
		{
			// 明显这个是 queue 的回调，呵呵
			m_handler(ec, 0);
		}
		else if (bytes_transfered == 0)
		{
			// 这个是 async_read 的回调
			m_handler(ec, 1);
		}
	}

	CoroQueue * m_queue;
	AsioAsyncStream * m_stream;
	Handler m_handler;
};

// async wait the coro_queue or the socket to became ready
// hander signature:  void handler(boost::system::error_code, std::size_t which)
template<class Handler>
inline BOOST_ASIO_INITFN_RESULT_TYPE(Handler,
	void(boost::system::error_code, std::size_t))
	async_wait_pop_or_read(xmpp_asio_connector::send_queue_type & queue, boost::asio::ip::tcp::socket & stream, Handler handler)
{
	using namespace boost::asio;

	BOOST_ASIO_READ_HANDLER_CHECK(Handler, handler) handler_check;

	boost::asio::detail::async_result_init<
		Handler, void(boost::system::error_code, std::size_t)> init(
		BOOST_ASIO_MOVE_CAST(Handler)(handler));

	async_wait_pop_or_read_op<
		xmpp_asio_connector::send_queue_type,
		boost::asio::ip::tcp::socket,
		BOOST_ASIO_HANDLER_TYPE(Handler, void(boost::system::error_code, std::size_t))
	> op(queue, stream, init.handler);
	op.start();
	return init.result.get();
}

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
		m_in_coro = 1;
		m_handler->handleConnect(this);
		m_in_coro = 0;
		return gloox::ConnNoError;
	}
	else
	{
		m_state = gloox::StateDisconnected;
		AVLOG_ERR <<  "xmpp: " << ec.message();
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
	boost::system::error_code ec;
	while (!ec)
	{
		int which = async_wait_pop_or_read(m_send_queue, m_socket, (*m_yield_context)[ec]);
		if (ec == boost::asio::error::operation_aborted)
			return ConnNoError;			
		if (which == 0)
		{
			// 发送数据
			std::string data_to_be_send = m_send_queue.async_pop((*m_yield_context)[ec]);
			boost::asio::async_write(m_socket, boost::asio::buffer(data_to_be_send), boost::asio::transfer_all(), (*m_yield_context)[ec]);
		}
		else
		{
			// 读取数据
			if (recv(-1) != ConnNoError)
				break;
		}
	}

	m_in_coro = 1;
	m_handler->handleDisconnect(this, ConnNoError);
	m_in_coro = 0;
	return ConnIoError;
}

gloox::ConnectionError xmpp_asio_connector::recv( int timeout )
{
	boost::system::error_code ec;
	boost::asio::streambuf readbuf;
	std::size_t bytes_transferred = boost::asio::async_read(m_socket, readbuf, boost::asio::transfer_at_least(1), (*m_yield_context)[ec]);
	// should not be called
	if (!ec)
	{
		std::string data;
		data.resize(bytes_transferred);
		readbuf.sgetn(&data[0], bytes_transferred);

		m_in_coro = 1;
		this->m_handler->handleReceivedData(this, data);
		m_in_coro = 0;
		return ConnNoError;
	}
	AVLOG_ERR << "xmpp recv error:" <<  ec.message();
	return ConnIoError;
}

#ifdef _MSC_VER
#define memory_barrier() _mm_mfence()
#elif defined(__GNUC__)
#define memory_barrier() __sync_synchronize()
#else
#define memory_barrier() do{ int i;}while(0)
#endif

bool xmpp_asio_connector::send(const std::string& data_to_be_send)
{
	if (m_in_coro)
	{
		boost::system::error_code ec;
		boost::asio::async_write(m_socket, boost::asio::buffer(data_to_be_send),
			boost::asio::transfer_all(), (*m_yield_context)[ec]);
		return !ec;
	}
	else
	{
		memory_barrier();
		m_send_queue.push(data_to_be_send);
		memory_barrier();
	}
	return true;
}

xmpp_asio_connector::xmpp_asio_connector(boost::shared_ptr<xmpp> _xmpp,
	gloox::ConnectionDataHandler* cdh, boost::asio::ip::tcp::resolver::query _query)
	: m_xmpp(_xmpp)
	, gloox::ConnectionBase( cdh )
	, io_service(_xmpp->get_ioservice())
	, m_socket( io_service )
	, m_query( _query )
	, m_send_queue(io_service)
{
}

xmpp_asio_connector::~xmpp_asio_connector()
{

}

void xmpp_asio_connector::cleanup()
{
	boost::system::error_code ec;
	// 清理资源
	this->m_socket.close(ec);
	m_xmpp.reset();
	m_send_queue.clear();
}

void xmpp_asio_connector::coroutine_start(boost::asio::yield_context this_coro_context)
{
    try
    {
		// 协程在这里，开始连接！
		this->m_yield_context = &this_coro_context;
		this->m_xmpp->m_client.connect(true);
		// 连接木有了
    }
    catch (std::exception& e)
    {
    }
}

xmpp::xmpp( boost::asio::io_service& asio, std::string xmppuser, std::string xmpppasswd, std::string xmppserver, std::string xmppnick )
	: io_service( asio )
	, m_jid( xmppuser + "/" + xmppnick )
	, m_client( m_jid, xmpppasswd )
	, m_xmppnick( xmppnick )
	, xmpp_stand(asio)
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
	current_connector = new xmpp_asio_connector(shared_from_this(), &m_client, query);
	m_client.setConnectionImpl(current_connector);
	boost::asio::spawn(io_service, boost::bind(&xmpp_asio_connector::coroutine_start, current_connector, _1));
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
	BOOST_FOREACH(boost::shared_ptr<gloox::MUCRoom> room, m_rooms)
	{
		if (room->name() == xmpproom)
		{
			room->send(message);
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
	AVLOG_INFO << "xmpp: " << "conntected to server " << info.server << " verifyed by " << info.issuer << " with " << info.protocol;
	return true;
}

void xmpp::onConnect()
{
	BOOST_FOREACH( boost::shared_ptr<gloox::MUCRoom> room,  m_rooms )
	{
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
	AVLOG_INFO << "xmpp: " << "joined  " << name;
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
	AVLOG_ERR << "xmpp: " << "no room \"" << room->name() << "\" attempt to create on";
	return true;
}

void xmpp::handleMUCSubject( gloox::MUCRoom* room, const std::string& nick, const std::string& subject )
{
}

}
