// GetIP.cpp : Defines the entry point for the console application.
//

#include <tchar.h>
#include <string>

#include <stdio.h>

#if 0
/**
 * 给定一个ip国家地区记录的偏移，返回一个IPLocation结构
 * @param offset 国家记录的起始偏移
 * @return IPLocation对象
 */
private IPLocation getIPLocation(long offset) {
	try {
		// 跳过4字节ip
		ipFile.seek(offset + 4);
		// 读取第一个字节判断是否标志字节
		byte b = ipFile.readByte();
		if(b == REDIRECT_MODE_1) {
			// 读取国家偏移
			long countryOffset = readLong3();
			// 跳转至偏移处
			ipFile.seek(countryOffset);
			// 再检查一次标志字节，因为这个时候这个地方仍然可能是个重定向
			b = ipFile.readByte();
			if(b == REDIRECT_MODE_2) {
				loc.country = readString(readLong3());
				ipFile.seek(countryOffset + 4);
			} else
				loc.country = readString(countryOffset);
			// 读取地区标志
			loc.area = readArea(ipFile.getFilePointer());
		} else if(b == REDIRECT_MODE_2) {
			loc.country = readString(readLong3());
			loc.area = readArea(offset + 8);
		} else {
			loc.country = readString(ipFile.getFilePointer() - 1);
			loc.area = readArea(ipFile.getFilePointer());
		}
		return loc;
	} catch (IOException e) {
		return null;
	}
}	

/**
 * 从offset偏移开始解析后面的字节，读出一个地区名
 * @param offset 地区记录的起始偏移
 * @return 地区名字符串
 * @throws IOException 地区名字符串
 */
private String readArea(long offset) throws IOException {
	ipFile.seek(offset);
	byte b = ipFile.readByte();
	if(b == REDIRECT_MODE_1 || b == REDIRECT_MODE_2) {
		long areaOffset = readLong3(offset + 1);
		if(areaOffset == 0)
			return LumaQQ.getString("unknown.area");
		else
			return readString(areaOffset);
	} else
		return readString(offset);
}

/**
 * 从offset位置读取3个字节为一个long，因为java为big-endian格式，所以没办法
 * 用了这么一个函数来做转换
 * @param offset 整数的起始偏移
 * @return 读取的long值，返回-1表示读取文件失败
 */
private long readLong3(long offset) {
	long ret = 0;
	try {
		ipFile.seek(offset);
		ipFile.readFully(b3);
		ret |= (b3[0] & 0xFF);
		ret |= ((b3[1] << 8) & 0xFF00);
		ret |= ((b3[2] << 16) & 0xFF0000);
		return ret;
	} catch (IOException e) {
		return -1;
	}
}	

/**
 * 从当前位置读取3个字节转换成long
 * @return 读取的long值，返回-1表示读取文件失败
 */
private long readLong3() {
	long ret = 0;
	try {
		ipFile.readFully(b3);
		ret |= (b3[0] & 0xFF);
		ret |= ((b3[1] << 8) & 0xFF00);
		ret |= ((b3[2] << 16) & 0xFF0000);
		return ret;
	} catch (IOException e) {
		return -1;
	}
}

/**
 * 从offset偏移处读取一个以0结束的字符串
 * @param offset 字符串起始偏移
 * @return 读取的字符串，出错返回空字符串
 */
private String readString(long offset) {
	try {
		ipFile.seek(offset);
		int i;
		for(i = 0, buf[i] = ipFile.readByte(); buf[i] != 0; buf[++i] = ipFile.readByte());
		if(i != 0) 
			return Utils.getString(buf, 0, i, "GBK");
	} catch (IOException e) {			
		log.error(e.getMessage());
	}
	return "";
}
#endif

static std::string readstring(size_t offset)
{
	try
	{

	}
	catch(...)
	{
	}

}


int _tmain()
{
	return 0;
}

