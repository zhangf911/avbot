
#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/date_time.hpp>

class qqlog : public boost::noncopyable
{
public:
	typedef boost::shared_ptr<std::ofstream> ofstream_ptr;
	typedef std::map<std::string, ofstream_ptr> loglist;

public:
	qqlog() 
	{}

	~qqlog()
	{}

public:
	std::string log_path(){
 		return  m_path.string() ;
	}
	// 设置日志保存路径.
	void log_path(const std::wstring &path)
	{
		m_path = path;
	}
	// 设置日志保存路径.
	void log_path(const std::string &path)
	{
		m_path = path;
	}
	// 添加日志消息.
	bool add_log(const std::string &groupid, const std::string &msg)
	{
		// 在qq群列表中查找已有的项目, 如果没找到则创建一个新的.
		ofstream_ptr file_ptr;
		loglist::iterator finder = m_group_list.find(groupid);
		if (m_group_list.find(groupid) == m_group_list.end())
		{
			// 创建文件.
			file_ptr = create_file(groupid);

			m_group_list[groupid] = file_ptr;
			finder = m_group_list.find(groupid);
		}

		// 如果没有找到日期对应的文件.
		if (!fs::exists(fs::path(make_filename(make_path(groupid)))))
		{
			// 创建文件.
			file_ptr = create_file(groupid);
			if (finder->second->is_open())
				finder->second->close();	// 关闭原来的文件.
			// 重置为新的文件.
			finder->second = file_ptr;
		}

		// 得到文件指针.
		file_ptr = finder->second;

		// 构造消息, 添加消息时间头.
		std::string data = "<p>" + current_time() + " " + msg + "</p>\n";
		// 写入聊天消息.
		file_ptr->write(data.c_str(), data.length());
		// 刷新写入缓冲, 实时写到文件.
		file_ptr->flush();

		file_ptr = m_lecture_file;
		if (file_ptr && m_lecture_groupid == groupid)
		{
			// 写入聊天消息.
			file_ptr->write(data.c_str(), data.length());
			// 刷新写入缓冲, 实时写到文件.
			file_ptr->flush();
		}

		return true;
	}

	// 开始讲座
	bool begin_lecture(const std::string &groupid, const std::string &title)
	{
		// 已经打开讲座, 乱调用API, 返回失败!
		if (m_lecture_file) return false;

		// 构造讲座文件生成路径.
		std::string save_path = make_path(groupid) + "/" + title + ".html";

		// 创建文件.
		m_lecture_file.reset(new std::ofstream(save_path.c_str(), 
			fs::exists(save_path) ? std::ofstream::app : std::ofstream::out));
		if (m_lecture_file->bad() || m_lecture_file->fail())
		{
			std::cerr << "create file " << save_path.c_str() << " failed!" << std::endl;
			return false;
		}

		// 保存讲座群的id.
		m_lecture_groupid = groupid;

		// 写入信息头.
		std::string info = "<head><meta http-equiv=\"Content-Type\" content=\"text/plain; charset=UTF-8\">\n";
		m_lecture_file->write(info.c_str(), info.length());

		return true;
	}

	// 讲座结束.
	void end_lecture()
	{
		// 清空讲座群id.
		m_lecture_groupid.clear();
		// 重置讲座文件指针.
		m_lecture_file.reset();
	}

protected:

	// 构造路径.
	std::string make_path(const std::string &groupid) const
	{
		return (m_path / groupid).string();
	}

	// 构造文件名.
	std::string make_filename(const std::string &p = "") const
	{
		std::ostringstream oss;
		boost::posix_time::time_facet* _facet = new boost::posix_time::time_facet("%Y%m%d");
		oss.imbue(std::locale(std::locale::classic(), _facet));
		oss << boost::posix_time::second_clock::local_time();
		std::string filename = (fs::path(p) / (oss.str() + ".html")).string();
		return filename;
	}

	// 创建对应的日志文件, 返回日志文件指针.
	ofstream_ptr create_file(const std::string &groupid) const
	{
		// 生成对应的路径.
		std::string save_path = make_path(groupid);

		// 如果不存在, 则创建目录.
		if (!fs::exists(fs::path(save_path)))
		{
			if (!fs::create_directory(fs::path(save_path)))
			{
				std::cerr << "create directory " << save_path.c_str() << " failed!" << std::endl;
				return ofstream_ptr();
			}
		}

		// 按时间构造文件名.
		save_path = make_filename(save_path);

		// 创建文件.
		ofstream_ptr file_ptr(new std::ofstream(save_path.c_str(), 
			fs::exists(save_path) ? std::ofstream::app : std::ofstream::out));
		if (file_ptr->bad() || file_ptr->fail())
		{
			std::cerr << "create file " << save_path.c_str() << " failed!" << std::endl;
			return ofstream_ptr();
		}

		// 写入信息头.
		std::string info = "<head><meta http-equiv=\"Content-Type\" content=\"text/plain; charset=UTF-8\">\n";
		file_ptr->write(info.c_str(), info.length());

		return file_ptr;
	}

	// 得到当前时间字符串, 对应printf格式: "%04d-%02d-%02d %02d:%02d:%02d"
	std::string current_time() const
	{
		std::ostringstream oss;
		boost::posix_time::time_facet* _facet = new boost::posix_time::time_facet("%Y-%m-%d %H:%M:%S%F");
		oss.imbue(std::locale(std::locale::classic(), _facet));
		oss << boost::posix_time::second_clock::local_time();
		std::string time = oss.str();
		return time;
	}

private:
	ofstream_ptr m_lecture_file;
	std::string m_lecture_groupid;
	loglist m_group_list;
	fs::path m_path;
};

