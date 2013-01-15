//
//  counter.h
//  qqbot
//
//  Created by hyq on 13-1-15.
//
//

#ifndef __qqbot__counter__
#define __qqbot__counter__

#include <iostream>
#include <fstream>
#include <map>
#include <boost/foreach.hpp>

class counter {
public:
    counter(std::string filename = "counter.db")
    : filename_(filename)
    {
        load();
    }
    
    
    
    ~counter()
    {
        save();
    }
    
    void save()
    {
        std::fstream out(filename_.c_str(), std::fstream::out);
        BOOST_FOREACH(auto iter, map_) {
            out << iter.first << "\t" << iter.second << "\n";
        }
    }
    
    void increace(std::string& qq)
    {
        map_[qq] ++ ;
    }
    
private:
    std::string filename_;
    std::map<std::string, std::size_t> map_;
    
    void load()
    {
        std::fstream in(filename_.c_str(), std::ios_base::in);
        std::string key;
        std::size_t count;
        if( in.good() ) {
            while(in >> key >> count)
            {
                map_[key] = count;
            }
        }
    }
    
};

#endif /* defined(__qqbot__counter__) */
