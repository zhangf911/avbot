
#include <boost/lambda/lambda.hpp>

#include "messagegroup.hpp"
#include "logger.hpp"

extern qqlog logfile;			// 用于记录日志文件.

static std::vector<messagegroup>	messagegroups;

// ---------------------------

// 查找同组的其他聊天室和qq群.
messagegroup * find_group(std::string id)
{
	BOOST_FOREACH(messagegroup & g ,  messagegroups)
	{
		if (g.in_group(id))
			return &g;
	}
	return NULL;
}

//从命令行或者配置文件读取组信息.
void build_group(std::string chanelmapstring, webqq & qqclient, xmpp& xmppclient, IrcClient &ircclient, mx::mx& mxclient)
{
	std::vector<std::string> gs;
	boost::split(gs, chanelmapstring, boost::is_any_of(";"));
	BOOST_FOREACH(std::string  pergroup, gs)
	{
		std::vector<std::string> group;
	 	boost::split(group, pergroup, boost::is_any_of(","));
		messagegroup g(&qqclient,&xmppclient,&ircclient,&mxclient);
		g.add_channel(group);
		messagegroups.push_back(g);
	}
}

void messagegroup::forwardmessage(std::string from, std::string message) {
	if (pimf){
		boost::get<std::string>(pimf->body) += message;
	}
    BOOST_FOREACH(std::string chatgroupmember, channels)
    {
        if (chatgroupmember == from)
            continue;

        if (chatgroupmember.substr(0,3) == "irc")
        {
            irc_->chat(std::string("#") + chatgroupmember.substr(4), message);
        }
        else if (chatgroupmember.substr(0,4) == "xmpp")
        {
            //XMPP
            xmpp_->send_room_message(chatgroupmember.substr(5), message);
        } else if (chatgroupmember.substr(0,2)=="qq" )
        {

            std::string qqnum = chatgroupmember.substr(3);
            logfile.add_log(qqnum, message);
            if(qq_->get_Group_by_qq(qqnum))
                qq_->send_group_message(*qq_->get_Group_by_qq(qqnum), message, boost::lambda::constant(0));
        }
    }
}

void forwardmessage(std::string from, std::string message)
{
	BOOST_FOREACH(messagegroup & g ,  messagegroups)
	{
		if (g.in_group(from))
		{
			g.forwardmessage(from, message);
		}
	}
}

