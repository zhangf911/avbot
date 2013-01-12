
#pragma once

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace boost {
namespace process{
typedef boost::asio::local::stream_protocol::socket	stream;

#ifndef WIN32
namespace posix{
	// fork and connect stdin/stdout, and excute the routine
	class fork : public boost::asio::local::stream_protocol::socket, boost::asio::local::stream_protocol  {
 		int   fd_;
		pid_t pid_;
	public:
		fork(boost::asio::io_service & io, boost::function<void ()> hander , bool redirectstderr = false)
		  :boost::asio::local::stream_protocol::socket(io)
		{
	  		int fd[2];
			socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
			pid_ = ::fork();
			if (pid_ == -1)
				throw std::bad_alloc();  //FIXME, use other?
			if (pid_ == 0 )
			{
				::close(fd[0]);
				::dup2(fd[1], 0);
				::dup2(fd[1], 1);
				if (redirectstderr)
					dup2(fd[1], 2);
				::close(fd[1]);
				hander();
			}
			else{
				::close(fd[1]);
				fd_ = dup(fd[0]);
				::close(fd[0]);
				
				boost::asio::local::stream_protocol::socket* baseclass = ((boost::asio::local::stream_protocol::socket*)this);
				assign(*this,fd_);
			}
		}
		~fork(){
			int status;
			::waitpid(pid_,&status,0);
		}
	};
}

static void  run_exec(const boost::filesystem::path & exefile, std::vector< std::string > argv)
{
	char * cargv[argv.size()+1];
	for(int i = 0; i < argv.size() ; i ++)
	{
		cargv[i] = (char*)( argv[i].c_str());
	}
	::execv(exefile.c_str(),cargv);
}

boost::shared_ptr<boost::process::stream>  respawn_as_socket(boost::asio::io_service & io,const boost::filesystem::path & exefile, std::vector< std::string >  argv)
{
	boost::shared_ptr<boost::process::stream>	 ptr;
	if(access(exefile.c_str(),X_OK)<0)
		throw "execuable not found";
	posix::fork * forker = new boost::process::posix::fork(io,boost::bind(& run_exec, exefile,argv));
	ptr.reset(forker,boost::bind(&posix::fork::~fork,forker));
}

#else

namespace win32{
	
}

// not supported on windows
boost::process::stream  respawn_as_socket(boost::asio::io_service & io,const boost::filesystem::path & exefile, ...)
{
	
}

#endif
}
}