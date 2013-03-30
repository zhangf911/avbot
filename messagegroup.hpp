#pragma once

#include "libirc/irc.h"
#include "libwebqq/webqq.h"
#include "libwebqq/url.hpp"
#include "libxmpp/xmpp.h"
#include "libmailexchange/mx.hpp"

struct messagegroup {
	webqq*		qq_;
	xmpp*		xmpp_;
	IrcClient*	irc_;
	mx::mx*		mx_;
	// 组号.
	std::vector<std::string> channels;

	messagegroup(webqq * _qq, xmpp * _xmpp, IrcClient * _irc, mx::mx * _mx)
	:qq_(_qq),xmpp_(_xmpp),irc_(_irc), mx_(_mx){}

	void add_channel(std::string);
	void add_channel(std::vector<std::string> groups){
		//std::copy(channels,groups);
		channels = groups;
	}

	bool in_group(std::string ch){
		return std::find(channels.begin(),channels.end(),ch)!=channels.end();
	}

	void broadcast(std::string message){
		forwardmessage("",message);
	}

	void forwardmessage(std::string from, std::string message);
};

// ---------------------------

// 查找同组的其他聊天室和qq群.
messagegroup * find_group(std::string id);

// --------------------

// 从命令行或者配置文件读取组信息.
void build_group(std::string chanelmapstring, webqq & qqclient, xmpp& xmppclient, IrcClient &ircclient, mx::mx&);

void forwardmessage(std::string from, std::string message);
