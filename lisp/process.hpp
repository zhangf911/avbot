/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  <hyq> <i@hyq.me>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef LISP_H
#define LISP_H
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/array.hpp>
#include "fork.hpp"

namespace av {
class process{
public:
    typedef boost::function<void(std::string)> OutpuHandler;
    process(boost::asio::io_service & io,const boost::filesystem::path & exefile) 
    : io_service_(io),
    exefile_(exefile),
    start_(false)
    {}
    void start(OutpuHandler handler) {
        handler_ = handler;
        process_.reset(new fork<boost::function<void()> >(io_service_, boost::bind(&process::atfork, this)));
        start_read();
        start_ = true;
    }
    void run(std::string cmd) {
        boost::asio::write(*process_, boost::asio::buffer(cmd));
    }
    bool hasStart() {
        return start_;
    }
private:
    void atfork() 
    {
        std::cout << exefile_.string() << std::endl;
        execlp("/usr/lib64/clozurecl/lx86cl64", NULL);
        perror("fuck you!!!!");
//         throw "fuck";
    }
    
    void start_read()
    {
        process_->async_read_some(boost::asio::buffer(buf_), 
                                 boost::bind(&process::handle_read, this, _1, _2));
    }
    
    void handle_read(const boost::system::error_code& ec, std::size_t n)
    {
        if(!ec) {
            std::string str;
            std::copy(buf_.begin(), buf_.begin()+n, std::back_inserter(str));
            handler_(str);
            start_read();
        }
    }
    
    boost::shared_ptr<av::fork<boost::function<void()> > > process_;
    boost::filesystem::path exefile_;
    boost::array<char, 1024> buf_;
    OutpuHandler handler_;
    bool start_ ;
    boost::asio::io_service & io_service_;
};
}
#endif // LISP_H
