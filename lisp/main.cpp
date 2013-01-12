#include "process.hpp"
#include <boost/bind.hpp>
#include <boost/thread.hpp>

void output(std::string str) 
{
    if(*str.begin() != '?') {
        std::cout << str <<std::endl;       
    }
}

int main() {
    boost::asio::io_service io;
    av::process p(io, boost::filesystem::path("/usr/lib64/clozurecl/lx86cl64"));
    p.start(boost::bind(output, _1));
    
    boost::thread t(boost::bind(&boost::asio::io_service::run, boost::ref(io)));
    
    while(true) {
        std::string cmd;
        std::getline(std::cin, cmd, '\n');
        p.run(cmd);
    }
}