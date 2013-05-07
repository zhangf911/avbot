
#include <ctime>
#include <boost/date_time.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

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

static bool is_match(std::string cronline_field, int field)
{
	// 计划通,  呵呵,  检查 "*"
	if (cronline_field == "*")
		return true;
	return (boost::lexical_cast<int>(cronline_field) == field);
}

// 在这里调度一下下一次启动.
void bulletin::schedule_next() const
{
	// 获取当前时间,  以 分为单位步进
	// 直到获得一个能满足要求的时间值
	// 或则超过 24h
	// 如果这样就直接设定一天以后再重新跑
	// 避免 cpu 无限下去.
	boost::posix_time::ptime now =  boost::posix_time::from_time_t(std::time(0));

	boost::posix_time::ptime end = now  + boost::posix_time::hours(25);

	// 比对 年月日.
	boost::posix_time::ptime::date_type date = now.date();

	boost::regex ex("[0-9\\*]\\-[0-9\\*]\\-[0-9\\*]\\-[0-9\\*]\\-[0-9\\*]");
	boost::cmatch what;

	// 以一分钟为步进.
	for (	boost::posix_time::time_iterator titr(now, boost::posix_time::minutes(1));
			titr < end;
			++ titr )
	{
		BOOST_FOREACH(std::string cronline, *m_settings)
		{
			// 使用 regex 提取 *-*-*-*-* 的各个数字.
			if (boost::regex_match(cronline.c_str(), what, ex))
			{
				// 然后为各个 filed 执行比较吧.
				what[1];
				what[2];
				what[3];
				what[4];
				what[5];

				if( is_match( what[1], titr->date().year())
					&& is_match( what[2], titr->date().month())
					&& is_match( what[3], titr->date().day())
					&& is_match( what[4], titr->time_of_day().hours())
					&& is_match( what[5], titr->time_of_day().minutes())
				)
				{
					// 设定 expires
					m_timer->expires_at();
				}
			}
		}
	}

	//

	// 设定下次的 expires

	//

}

void bulletin::operator()( boost::property_tree::ptree message ) const
{
	// 其实主要是为了响应 .qqbot bulletin 命令.

}
