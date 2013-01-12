
#pragma once

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

namespace boost {
class process;
#ifndef WIN32
namespace posix{
	// fork and connect stdin/stdout, and excute the routine
	template<typename Hander>
	class fork : public boost::asio::local::stream_protocol::socket  {
 		int   fd_;
		pid_t pid_;
		friend class process;
		fork(boost::asio::io_service & io, Hander hander , bool redirectstderr = false)
		  :boost::asio::local::stream_protocol::socket(io)
		{
	  		int fd[2];
			socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
			pid_ = ::fork();
			if (pid_ == -1)
				throw std::bad_alloc();  //FIXME, use other?
			if (pid_ == 0 )
			{
				close(fd[0]);
				dup2(fd[1], 0);
				dup2(fd[1], 1);
				if (redirectstderr)
					dup2(fd[1], 2);
				close(fd[1]);
				hander();
			}
			else{
				close(fd[1]);
				fd_ = dup(fd_[0]);
				close(fd[0]);
				boost::asio::local::stream_protocol::socket::assign(AF_UNIX, fd_);
			}
		}
	};
}

class process{
public:
	process(boost::asio::io_service & io,const boost::filesystem::path & exefile);
	
	
};
	

#else

namespace win32{
	
}

class process{
	
};

#endif
	
}