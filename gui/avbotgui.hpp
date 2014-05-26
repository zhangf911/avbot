
#include <boost/function.hpp>
#include <boost/asio/io_service.hpp>

#ifdef _WIN32
#include <windows.h>
#endif


#ifdef _WIN32
HWND async_input_box_get_input_with_image(boost::asio::io_service & io_service, std::string imagedata, boost::function<void(boost::system::error_code, std::string)> donecallback);
#endif

void show_dialog(std::string & qqnumber, std::string & qqpwd,
	std::string & ircnick, std::string & ircroom, std::string & ircpwd,
		std::string &xmppuser, std::string & xmppserver,std::string & xmpppwd, std::string & xmpproom, std::string &xmppnick);
