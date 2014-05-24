
#include "pythonscriptengine.hpp"
#include "boost/json_parser_write.hpp"
#include <boost/property_tree/json_parser.hpp>
#include <Python.h>
#include <boost/python.hpp>

namespace py = boost::python;

boost::system::error_code no_module;
boost::system::error_code no_handle_func;

bool python_init = false;

struct MessageSender
{
    boost::function<void(std::string)> sender_;

    void send_message(std::string msg)
    {
        sender_(msg);
    }
};

class PythonScriptEngine
{
public:
	PythonScriptEngine(asio::io_service& io, boost::function < void(std::string)> sender)
		: io_(io)
		, sender_(sender)
	{
        try {
            if (!python_init)
            {
                Py_Initialize();
                python_init = true;
                py::scope module_ = py::import("avbot");
                global_ = module_.attr("__dict__");

                py::class_<MessageSender>("MessageSender")
                    .def("send_message", &MessageSender::send_message);
            }
            py::object pysender = global_["MessageSender"]();
            py::extract<MessageSender&>(pysender)().sender_ = sender_;
            pyhandler_ = global_["MessageHandler"]();
            pyhandler_.attr("send_message") = pysender.attr("send_message");
        }
        catch(...)
        {
            PyErr_Print();
            throw;
        }
	}

	~PythonScriptEngine()
	{

	}

	void operator()(const boost::property_tree::ptree& msg)
	{
        try {
            std::stringstream ss;
            boost::property_tree::json_parser::write_json(ss, msg);
            pyhandler_.attr("on_message")(ss.str());
        }
        catch(...)
        {
            PyErr_Print();
        }
	}

private:
	asio::io_service& io_;
	boost::function < void(std::string)> sender_;
    static py::object global_;
    py::object pyhandler_;
};

py::object PythonScriptEngine::global_;


avbot_extension make_python_script_engine(asio::io_service& io, std::string channel_name, boost::function<void(std::string)> sender)
{
	return avbot_extension(
		channel_name,
		PythonScriptEngine(io, sender)
	);
}
