
#include <boost/signals2.hpp>
#include <boost/asio.hpp>
#if !defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR) && !defined(BOOST_ASIO_HAS_WINDOWS_OBJECT_HANDLE)
#include <boost/thread.hpp>
#endif
#include <boost/stringencodings.hpp>
#include "input.hpp"

static boost::signals2::signal< void(std::string)> input_got_one_line;

#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
static void inputread(const boost::system::error_code & ec, std::size_t length,
	boost::shared_ptr<boost::asio::posix::stream_descriptor> stdin,
	boost::shared_ptr<boost::asio::streambuf> inputbuffer)
{
	std::istream input(inputbuffer.get());
	std::string line;
	std::getline(input,line);

	input_got_one_line(line);

	boost::asio::async_read_until(
		*stdin, *inputbuffer, '\n',
		boost::bind(inputread , _1,_2, stdin, inputbuffer)
	);
}
#else
#if defined(BOOST_ASIO_HAS_WINDOWS_OBJECT_HANDLE)

// 这个类使用 wait 和 ReadConsole 结合的办法，读取一行完整的输入
template<typename Handler>
struct console_read_line_op
{
	Handler m_handler;
	boost::asio::windows::object_handle& console_handle;
	std::vector<WCHAR> readbuf;
	DWORD m_savedmode;

	console_read_line_op(boost::asio::windows::object_handle& _console_handle, Handler handler)
		: console_handle(_console_handle)
		, m_handler(handler)
	{
		GetConsoleMode(console_handle.native_handle(), &m_savedmode);
		SetConsoleMode(
			console_handle.native_handle(),
			ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT
		);
		console_handle.async_wait(*this);
	}

	// async_wait 调用到这里
	void operator()(boost::system::error_code ec)
	{
		DWORD r =0;
		INPUT_RECORD ir;
		ReadConsoleInputW(console_handle.native_handle(), &ir, 1, &r);
		if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown && ir.Event.KeyEvent.uChar.UnicodeChar)
		{
			WCHAR c = ir.Event.KeyEvent.uChar.UnicodeChar;
			if (c == L'\b')
			{
				// 退格键啊，应该删了
//				WriteConsoleOutputW();
				if (!readbuf.empty())
				{
					WCHAR lc = readbuf.back();
					
					readbuf.pop_back();
					
					if (lc >= 256)
					{
						WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"\b \b \b", 5, &r, 0);
					}
					else
					{
						WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"\b \b", 3, &r, 0);
					}
				}
			}
			else
			{
				readbuf.push_back(c);
			}

			if (iswprint(c))
			{
				WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), &c, 1, &r, 0);
			}
			// 判定 是不是读取到了 "\n"
			// 是的话就可以调用 handler 了
			if (c == L'\r')
			{
				// 读取到行尾啦！回调吧
				std::string thisline = wide_to_utf8(std::wstring(readbuf.data(), readbuf.size()));
				readbuf.clear();
				WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"\n", 1, &r, 0);
				SetConsoleMode(console_handle.native_handle(), m_savedmode);
				m_handler(thisline);
				return;
			}
		}

		// 重复读取
		console_handle.async_wait(*this);
	}
};

template<typename Handler>
void async_console_read_line(boost::asio::windows::object_handle& object_handle, Handler handler)
{
	console_read_line_op<Handler> op(object_handle, handler);
}

static void console_read(std::string line,
	boost::shared_ptr<boost::asio::windows::object_handle> consolehandle)
{
	// now we use
	input_got_one_line(line);

	async_console_read_line(
		*consolehandle,
		boost::bind(&console_read, _1, consolehandle)
	);
}

#else
// workarround windows that can use posix stream for stdin
static void input_thread(boost::asio::io_service & io_service)
{
	while (!boost::this_thread::interruption_requested() && !std::cin.eof())
	{
		std::string line;
		std::getline(std::cin, line);
		io_service.post([line]{
			input_got_one_line(ansi_utf8(line));
		});
	}
}
#endif
#endif

void start_stdinput(boost::asio::io_service & io_service)
{
#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
		boost::shared_ptr<boost::asio::posix::stream_descriptor> stdin(
			new boost::asio::posix::stream_descriptor( io_service, 0 )
		);
		boost::shared_ptr<boost::asio::streambuf> inputbuffer(
			new boost::asio::streambuf
		);
		boost::asio::async_read_until(
			*stdin, *inputbuffer, '\n',
			boost::bind(inputread , _1, _2, stdin, inputbuffer)
		);
#elif defined(BOOST_ASIO_HAS_WINDOWS_OBJECT_HANDLE)

		boost::shared_ptr<boost::asio::windows::object_handle>
			consolehandle(new boost::asio::windows::object_handle(io_service, GetStdHandle(STD_INPUT_HANDLE)));

		async_console_read_line(
			*consolehandle,
			boost::bind(&console_read, _1, consolehandle)
		);

#else
		boost::thread(
			boost::bind(input_thread, boost::ref(io_service))
		);
#endif
}

void connect_stdinput(boost::function<void(std::string)> cb)
{
	input_got_one_line.connect(cb);
}
