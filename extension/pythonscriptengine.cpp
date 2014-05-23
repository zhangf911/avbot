#include "pythonscriptengine.hpp"
#include <boost/property_tree/json_parser.hpp>
#include <Python.h>
#include <boost/python.hpp>

namespace py = boost::python;

boost::system::error_code no_module;
boost::system::error_code no_handle_func;

bool python_init = false;

class PythonScriptEngine
{
public:
	PythonScriptEngine(asio::io_service& io, boost::function < void(std::string)> sender)
		: io_(io)
		, sender_(sender)
	{
		if (!python_init)
		{
			Py_Initialize();
			python_init = true;
		}

		module_ = py::import("avbot");
		py::object send_message = py::make_function(sender_);
		py::def("send_message", send_message);
	}

	~PythonScriptEngine()
	{

	}

	void operator()(const boost::property_tree::ptree& msg)
	{
		std::stringstream ss;
		boost::property_tree::json_parser::write_json(ss, msg);
		module_.attr("on_message")(ss.str());
	}

private:
	asio::io_service& io_;
	boost::function < void(std::string)> sender_;
	py::object module_;
};


avbot_extension make_python_script_engine(asio::io_service& io, std::string channel_name, boost::function<void(std::string)> sender)
{
	return avbot_extension(
		channel_name,
		PythonScriptEngine(io, sender)
		);
}