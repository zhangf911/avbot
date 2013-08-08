cmake_minimum_required(VERSION 2.6)
project(libavbot)


add_library(libavbot STATIC avbot.cpp)

target_link_libraries(libavbot irc webqq xmpp mailexchange)

set_target_properties(libavbot
		PROPERTIES
		OUTPUT_NAME avbot
		PREFIX "lib"
		CLEAN_DIRECT_OUTPUT 1)