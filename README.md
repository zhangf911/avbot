#  avbot = 聊天记录机器人

avbot 连通 IRC、XMPP 和  QQ群，并能实时记录聊天信息。每日自动生成新的日志文件。

## 功能介绍

### 登录QQ，记录群消息
### 登录IRC，记录IRC消息
### 登录XMPP，记录XMPP聊天室消息
### 将群消息转发到IRC和XMPP聊天室
### 将IRC消息转发到QQ群和XMPP聊天室
### 将XMPP聊天室消息转发到QQ群和IRC
### QQ图片转成 url 链接给 IRC和XMPP聊天室

# 编译办法

项目使用 cmake 编译。编译办法很简单

	mkdir build
	cd build
	cmake [qqbot的路径]
	make -j8
# 编译依赖

依赖 boost。 boost 要 1.48 以上。

gloox 已经通过 bundle 的形式包含了，不需要外部依赖了。

# 使用

读取配置文件 /etc/qqbotrc

配置文件的选项就是去掉 -- 的命令行选项。比如命令行接受 --qqnum 
配置文件就写

	qqnum=qq号码

就可以了。
命令行选项看看 --help 输出

## IRC频道

频道名不带 \# 比如

	--ircrooms=ubuntu-cn,gentoo-cn,fedora-zh

逗号隔开

## XMPP 聊天室

	--xmpprooms=linuxcn@im.linuxapp.org,avplayer@im.linuxapp.org

也是逗号隔开

## 频道组

使用 --map 功能将频道和QQ群绑定成一组。被绑定的组内消息互通。

用法：  --map=qq:123456,irc:avplayer;qq:3344567,irc:otherircchannel,xmpp:linuxcn

频道名不带 \# , XMPP 聊天室不带 @ 后面的服务器地址。

也可以在 /etc/qqbotrc 或者 ~/.qqbotrc 写，每行一个，不带 --。
如 map=qq:123456,irc:avplayer;qq:3344567,irc:otherircchannel,xmpp:linuxcn


频道组用 ; 隔开。组成份间用,隔离。

# 获得帮助

我们在 IRC（irc.freenode.net ） 的 \#avplayer 频道。 QQ群 3597082 还有 XMPP聊天室 avplayer@im.linuxapp.org

# thanks

谢谢 神话群群主提供的代码和建议；Youku的谢总(女)贡献的IRC代码。
还有 pidgin-lwqq 解析的 WebQQ 协议。

