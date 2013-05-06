
#include <boost/date_time.hpp>

#include "bulletin.hpp"

void bulletin::load_settings()
{
	boost::filesystem::path settingsfile = boost::filesystem::current_path() / m_channel_name / "bulletin_setting" ;
	if (boost::filesystem::exists(settingsfile))
	{
		std::ifstream bulletin_setting(settingsfile.string().c_str());

		std::string line;
		std::getline(bulletin_setting, line);

		m_settings->push_back(line);
	}
}

// 在这里调度一下下一次启动.
void bulletin::schedule_next() const
{
	// 获取当前时间,  以 分为单位步进
	// 直到获得一个能满足要求的时间值
	// 或则超过 24h
	// 如果这样就直接设定一天以后再重新跑
	// 避免 cpu 无限下去
	boost::posix_time::ptime now =  boost::posix_time::from_time_t(std::time(0));

}

void bulletin::operator()( boost::property_tree::ptree message ) const
{
	// 其实主要是为了响应 .qqbot bulletin 命令.

}
