
#include "boost/process.hpp"
extern std::string execpath;

inline void re_exec_self()
{
	boost::process::context ctx;
	ctx.environment = boost::process::self::get_environment();

	std::vector<std::string> args;
	args.push_back("avbot");
	args.push_back("-d");
	std::cout <<  "segfault ,  reexecing " <<  execpath <<  std::endl;
	boost::process::launch(execpath, args, ctx);
	exit(0);
}
