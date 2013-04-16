cmake_minimum_required(VERSION 2.6)
project(libavbot)


add_library(libavbot STATIC avbot.cpp)

target_link_libraries(libavbot webqq xmpp mailexchange)
