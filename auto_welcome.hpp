/**
 * @file
 * @author
 * @date
 * 
 * usage:
 * 	auto_question question;
 * 	auto_question::value_qq_list list;
 * 
 * 	list.push_back("lovey599");
 * 
 * 	questioin.add_to_list(list);
 * 	questioin.on_handle_message(group, qqclient);
 */

#ifndef __QQBOT_AUTO_QUESTION_H__
#define __QQBOT_AUTO_QUESTION_H__

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#include <boost/noncopyable.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/assign.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

#include "libwebqq/webqq.h"

class auto_welcome : public boost::noncopyable
{
public:
	typedef std::vector<std::string> value_qq_list;
	
	auto_welcome(std::string filename = "welcome.txt")
		: _filename(filename)
		, _is_processing(false)
	{
		try
		{
			load_question();
		}
		catch (std::exception& ex)
		{

			std::cerr << "load questioin error." << std::endl;
		}
	}
	
	void add_to_list(value_qq_list list)
	{
		if (_is_processing)
		{
			_is_processing = false;
		}
		
		BOOST_FOREACH(std::string item, list)
		{
			_process_count.push_back(item);
		}
	}
	
	template<class Msgsender>
	void on_handle_message(Msgsender msgsender)
	{
		handle_question(msgsender);
		_process_count.clear();
	}
	
protected:
	void load_question()
	{
		std::ifstream file(_filename.c_str());

		std::size_t fsize = boost::filesystem::file_size(_filename);
		_welcome_message.resize(fsize);
		file.read(&_welcome_message[0], fsize);
	}
	template<class Msgsender>
	void handle_question(Msgsender msgsender) const
	{
		std::string str_msg_body = build_message();
		
		BOOST_FOREACH(std::string item, _process_count)
		{
			std::string str_msg = boost::str(boost::format("@%1 %2\n") % item % str_msg_body);

			msgsender(str_msg);
		}
	}
	
	std::string build_message() const
	{
		return _welcome_message;
	}
	
private:
	std::string _filename;
	
	std::string _welcome_message;

	bool _is_processing;

	std::vector<std::string> _process_count;
};

#endif
