
#pragma once

#include <string>
#include <boost/asio/io_service.hpp>

#include "libavbot/avbot.hpp"

void new_channel_set_extension(boost::asio::io_service &io_service, avbot & mybot , std::string channel_name);
