
#pragma once

#include <string>
#include <boost/function.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>

#include "libavbot/avbot.hpp"
#include "boost/stringencodings.hpp"

class avbotextension
{
protected:
	boost::asio::io_service &io_service;
	boost::function<void ( std::string ) > m_sender;
	std::string m_channel_name;

	template<class MsgSender>
	avbotextension( boost::asio::io_service &_io_service,  MsgSender sender, std::string channel_name )
		: io_service( _io_service ), m_sender( sender ), m_channel_name( channel_name )
	{
	}
public:
	typedef void result_type;
};


void new_channel_set_extension(boost::asio::io_service &io_service, avbot & mybot , std::string channel_name);

namespace detail{
class avbotexteison_interface
{
public:
	virtual void operator()(const boost::property_tree::ptree & msg) = 0;
};

template<class ExtensionType>
class avbotexteison_adapter :  public avbotexteison_interface
{
	boost::scoped_ptr<ExtensionType> m_pextension;
	void operator()(const boost::property_tree::ptree & msg)
	{
		(*m_pextension)(msg);
	}

public:
	avbotexteison_adapter(const ExtensionType & obj)
	{
		m_pextension.reset(new ExtensionType(obj));
	}

	avbotexteison_adapter(ExtensionType * obj)
	{
		m_pextension.reset(obj);
	}
};

} // namespace detail

// type erasure for avbot extension
class botextension
{
	boost::shared_ptr<detail::avbotexteison_interface> m_exteison_obj;
	std::string m_channel_name;

public:

	template<class ExtensionType>
	botextension(std::string channel_name, const ExtensionType & obj)
		: m_channel_name( channel_name )
	{
		m_exteison_obj.reset(new detail::avbotexteison_adapter<ExtensionType>(obj));
	}

	template<class ExtensionType>
	botextension & operator = (const ExtensionType & extensionobj)
	{
		m_exteison_obj.reset(new detail::avbotexteison_adapter<ExtensionType>(extensionobj));
		return *this;
	}

	template<class ExtensionType>
	botextension & operator = (ExtensionType * extensionobj)
	{
		m_exteison_obj.reset(new detail::avbotexteison_adapter<ExtensionType>(extensionobj));
		return *this;
	}

	botextension & operator = (const botextension & rhs)
	{
		m_exteison_obj = rhs.m_exteison_obj;
		m_channel_name = rhs.m_channel_name;
		return *this;
	}

	void operator()(const boost::property_tree::ptree & msg)
	{
		try{
			if (msg.get<std::string>("channel") != m_channel_name)
				return;
			// 调用实际的函数
			(*m_exteison_obj)(msg);
		}catch (const boost::property_tree::ptree_error&)
		{
            // error access the ptree
            // ignore it
        }
	}
	typedef void result_type;
};

