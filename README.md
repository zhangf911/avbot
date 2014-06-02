#  avbot = 聊天机器人（QQ云秘书）[![Build Status](https://travis-ci.org/avplayer/avbot.png?branch=master)](https://travis-ci.org/avplayer/avbot)

avbot 连通 IRC、XMPP 和  QQ群，并能实时记录聊天信息。每日自动生成新的日志文件。

see [社区维基的avbot介绍](http://wiki.avplayer.org/Avbot)

想了解 avbot 最重要的子模块 libwebqq 请点开 libwebqq 目录查看其 README.md

# 支持的系统 

cmake >= 2.8

## GCC 系

centos >= 6.4

ubuntu >= 12.04

debian >= 7

和其他一些 gcc >= 4.4.7 的系统。

centos6 请添加 atrpm 源安装 cmake 2.8 版本。自带的 cmake 2.6 太旧。

## MSVC 系

VisutalStudio 2013 (支持 Vista 以上系统)

VisutalStudio 2013 - vc120_xp toolset （支持 Windows XP 以上系统）

 启用步骤 

  > cmake -G "VisualStudio 12" -T "vc120_xp"

  > cmake -G "VisualStudio 12 Win64" -T "vc120_xp"

## icc 系
  icc >= 13
  
## clang 系
  clang >= 3.3

## mingw 系

mingw32 不支持，需要使用 mingw64

mingw64 >= 4.8

# 编译注意事项

请不要在源码文件夹里直接执行 cmake. 务必创建一个专用的文件夹存放编译中间文件，如建立个 build 文件夹。
然后在 build 文件夹里执行 cmake PATH_TO_AVBOT

因为 cmake 有很多时候，需要删除 build 文件夹重新执行，而在源码内部直接 cmake ，则因为文件夹混乱，不好清除中间文件

## boost 相关

boost 需要至少 1.55 版本。 

boost 请静态编译， gentoo 用户注意 USE=static-libs emerge boost

win 下, boost 请使用 link=static runtime-link=static 执行静态编译 (包括 mingw 下)。

linux 下如果必须自己编译 boost 的话，请使用参数 link=static runtime-link=shared --layout=system variant=release --prefix=/usr 执行编译。

link=static 表示编译为静态库， runtime-link=static 则表示，应用程序最终会使用静态链接的 C++ 运行时。这个在 windows 平台是必须的要求。因为 VC 的 C 和 C++ 运行时打包起来非常麻烦。（mingw 的也一样）

linux 那边 runtime-link=shared 表示使用动态链接的 libstdc++.so， libstdc++.so 无需静态链接，不是么 ;)

## mingw 相关
mingw 下编译务必选择 Mingw Makefiles 生成器。MSYS Makefiles 和 生成器虽然 cmake 阶段能过，但是编译阶段会失败。

如果 mingw 带的 make 不是 mingw32-make.exe 而是直接 make.exe （比如 STL 发现的编译版本）
需要在 cmake 里设定 CMAKE_MAKE_PROGRAM=make.exe的绝对路径。而不要因为找不到 make 去选择 MSYS Makefiles 这个生成器。

主要原因是 MSYS Makefiles 生成器让 cmake 里 if(WIN32) 部分条件指令为 FALSE 。导致设定出错。

如果不是，需要设定 BOOST_ROOT, 可以在 cmake-gui 里点 configure 按钮前，通过 "Add Enytry" 按钮添加。
STL 打包的 mingw 自带 boost, 可以设定 BOOST_ROOT=c:/mingw 即可。
不过 STL 打包的 mingw 没有编译 context 和 coroutine 库，可以自己用 b2 toolset=gcc 编译了，把 libboost_context.a 和 libboost_coroutine.a 拷贝到 c:/mingw/lib

注意， 添加 --layout=system variant=release 才能编译出 libbosot_context.a 这样的不带各种后缀的库版本。

## MSVC 相关

理论上 2012 版本也是支持的，不过没有测试过。

cmake 生成好 VC 工程然后打开 avbot.sln 即可。

如果 boost 在 c:/boost 则无需额外设置
如果不是，需要设定 BOOST_ROOT, 可以在 cmake-gui 里点 configure 按钮前，通过 "Add Enytry" 按钮添加。