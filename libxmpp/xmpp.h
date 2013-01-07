/*
 * Copyright (C) 2013  微蔡 <microcai@fedoraproject.org>
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

#include <boost/scoped_ptr.hpp>
#include <boost/asio.hpp>
#include <string>

namespace XMPP {
	class xmpp_impl;
}

class xmpp
{
public:
	xmpp(boost::asio::io_service & asio, std::string xmppuser, std::string xmpppasswd);
	void join(std::string roomjid);
	~xmpp();
private:
	boost::scoped_ptr<XMPP::xmpp_impl>		impl;
};

#endif // XMPP_H
