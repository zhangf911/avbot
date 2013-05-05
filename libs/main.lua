
--  使用 send_channel_message 将消息发回频道.
--  send_channel_message('test')


-- 每当频道有消息发生, 就调用这个函数.
-- 注意, 不要在这里阻塞, 会导致整个 avbot 都被卡住
-- 因为 avbot 是单线程程序.
testjson=[[
{
    "protocol": "qq",
    "channel": "8734129",
    "room":
    {
        "code": "3964816318",
        "groupnumber": "8734129",
        "name": "Ballache"
    },
    "op": "0",
    "who":
    {
        "code": "3890512126",
        "nick": "Aiephen",
        "name": "Aiephen",
        "qqnumber": "24237081",
        "card": "Bearkid Broodmother"
    },
    "preamble": "qq(Bearkid Broodmother) :",
    "message":
    {
        "text": "2\u00E4\u00BB\u00A3\u00E5\u008A\u009F\u00E8\u0083\u00BD\u00E6\u009B\u00B4\u00E5\u00BC\u00BA\u00E4\u00BA\u0086\u00EF\u00BC\u008C\u00E4\u00B8\u008D\u00E8\u00BF\u0087\u00E5\u009B\u00A0\u00E4\u00B8\u00BA\u00E7\u009B\u00B4\u00E6\u008E\u00A5\u00E6\u0098\u00AF1\u00E4\u00BB\u00A3\u00E7\u00A7\u00BB\u00E6\u00A4\u008D\u00E8\u00BF\u0087\u00E5\u008E\u00BB\u00E7\u009A\u0084\u00EF\u00BC\u008C\u00E9\u0087\u008C\u00E9\u009D\u00A2\u00E6\u009C\u0089\u00E4\u00B8\u008D\u00E5\u00B0\u0091\u00E6\u0097\u00A7\u00E6\u0097\u00B6\u00E4\u00BB\u00A3\u00E9\u0081\u0097\u00E7\u0095\u0099\u00E4\u00BA\u00A7\u00E5\u0093\u0081 "
    }
}
]]
package.path="lua_libraries/?.lua;?.lua"
package.cpath="lua_libraries/?.so;lua_libraries/?.dll"
require('json')
--msg_table=json.decode(testjson)
--print(msg_table.who.nick)
--print(msg_table.message.text)

function channel_message(jsonmsg)
	print(jsonmsg)
end
