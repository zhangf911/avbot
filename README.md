avboost
=======

some templates that's very very useful during development.

avboost 是一些在开发 avplayer 的项目过程中编写的极其有用的方便的小模板工具库的集合. 是对 Boost 库的一个补充.

包括但不限于


*    base64 编解码
*    cfunction 封装 boost::funcion 为 C 接口的回调.
*    async_coro_queue 协程使用的异步列队.
*    async_foreach 协程并发的 for_each 版本
*    async_dir_walk 利用协程并发的 for_each 版本实现的异步文件夹遍历.
*    acceptor_server 一个简单的 accepter 用于接受TCP连接.
*    avloop 为 asio 添加 idle 回调. 当 io_service 没事情干的时候调用回调.
*    timedcall 简单的 asio  deadline_timer 封装. 用于延时执行一个任务.
*    hash 来自 boost.sandbox 的 哈希编码, 支持 SHA1 MD5
*    multihandler 一个回调封装, 用于 指定的回调发生N次后调用真正的回调.
*    json_create_escapes_utf8.hpp 在使用 boost 的 json 编码器的时候, 包含这个头文件, 可以让 boost 的 json 编码器输出未经过转码的utf8文本
*    urlencode.hpp 用于 url 的编码, 将非允许字符使用 % 编码.
*    consolestr.hpp 用于将UTF-8字符串转码为本地控制台的字体编码以输出不乱码. 因 linux 和 windows 的控制台编码不一样而引入
    
  

