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
#include "xmpp.h"

using namespace boost::asio;

xmpp::xmpp(boost::asio::io_service& asio, std::string xmppuser, std::string xmpppasswd)
  :password(xmpppasswd), m_asio(asio)
{
	std::vector<std::string> splited;
	boost::split(splited, xmppuser, boost::is_any_of("@"));
	user = splited[0];
	hostname = splited[1];
	//解析 xmppuser 获得服务器
// 	boost::asio::ip::resolver_service<boost::asio::ip::v4()> resover;
	ip::tcp::resolver	resolver(asio);
	ip::tcp::resolver::query query(hostname);
	


}
