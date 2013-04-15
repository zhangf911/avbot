#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/format.hpp>
#include "avbot.hpp"
#include "libwebqq/url.hpp"

avbot::avbot( boost::asio::io_service& io_service )
  : m_io_service(io_service), fetch_img(false)
{
	base_image_url = "http://w.qq.com/cgi-bin/get_group_pic?pic=";

}

void avbot::callback_on_irc_message( irc::IrcMsg pMsg )
{
	using boost::property_tree::ptree;
	// formate irc message to JSON and call on_message

	ptree message;
	message.put("protocol", "irc");
	message.put("room", pMsg.from);
	message.put("who", pMsg.whom);

	ptree textmsg;
	textmsg.add("text", pMsg.msg);
	message.add_child("message", textmsg);

	// TODO 将 识别的 URL 进行转化.

	on_message(message);
}

void avbot::callback_on_qq_group_message( std::string group_code, std::string who, const std::vector< qqMsg >& msg )
{
	using boost::property_tree::ptree;
	ptree message;
	message.put("protocol", "qq");
	ptree ptreee_group;
	ptreee_group.add("code", group_code);

	qqGroup_ptr group = m_qq_account->get_Group_by_gid( group_code );
	std::string groupname = group_code;

	if( group ){
		groupname = group->name;
		ptreee_group.add("groupnumber", group->qqnum);
		ptreee_group.add("name", group->name);
	}

	message.add_child("room", ptreee_group);

	ptree ptree_who;
	ptree_who.add("code", who);

	qqBuddy *buddy = NULL;

	buddy = group ? group->get_Buddy_by_uin( who ) : NULL;
	if (buddy){
		ptree_who.add("name", buddy->nick);
		ptree_who.add("qqnumber", buddy->qqnum);
		ptree_who.add("card", buddy->card);
	}

	message.add_child("who", ptree_who);
	ptree textmsg;

	// 解析 qqMsg
	BOOST_FOREACH( qqMsg qqmsg, msg )
	{
		std::string buf;

		switch( qqmsg.type ) {
			case qqMsg::LWQQ_MSG_TEXT:
			{
				textmsg.add("text", qqmsg.text);
			}
			break;
			case qqMsg::LWQQ_MSG_CFACE:
			{
				if (fetch_img){
					// save to disk
					// 先检查这样的图片本地有没有，有你fetch的P啊.
					if (!fs::exists(std::string("images/") + qqmsg.cface))
					{
						if (!fs::exists("images"))
							fs::create_directories("images");
						m_qq_account->async_fetch_cface(qqmsg.cface, boost::bind(&avbot::callback_save_qq_image, this, _1, _2, qqmsg.cface));
					}
				}
				textmsg.add("cface", qqmsg.cface);
			}
			break;
			case qqMsg::LWQQ_MSG_FACE:
			{
 				std::string url = boost::str( boost::format("http://0.web.qstatic.com/webqqpic/style/face/%d.gif" ) % qqmsg.face );
 				textmsg.add("url", url);
			} break;
		}
	}

}
