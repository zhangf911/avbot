
#include <boost/format.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/regex.hpp>
#include "internet_mail_format.hpp"
#include <boost/base64.hpp>

#include "smtp.hpp"

static void sended(const boost::system::error_code & ec)
{
	std::cout <<  ec.message() <<  std::endl;
}

int main()
{
	boost::asio::io_service io;
	smtp _smtp(io, "test@avplayer.org", "testpasswd123");
	InternetMailFormat imf;

	imf.header["from"] = "test@avplayer.org";
	imf.header["to"] = "test@avplayer.org";
	imf.header["subject"] = "test mail";
	imf.header["content-type"] = "text/plain";

	imf.have_multipart = false;
	imf.body = "test body";

	_smtp.async_sendmail(imf, sended);
	io.run();
	return 0;
}
