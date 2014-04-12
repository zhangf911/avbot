
#pragma once

#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>
#include <boost/type_traits/function_traits.hpp>

namespace boost {

/*
 * cfunction 用于将 boost::function 拆分为一个 C 函数指针和一个 void * user_data 指针.
 *
 * 只要接受 一个 C 函数指针 + void * user_data 的地方, 就可以利用 cfunction 封装 boost::function,
 * 而 boost::function 又可以继续封装 boost::bind, boost::bind 的强大就无需多说了吧.
 *
 * 例子在是这样的, 相信这个例子一看大家全明白了, 呵呵
 *

static void * my_thread_func(int a, int b,  int c)
{
	std::cout <<  a <<  b <<  c <<  std::endl;
	return NULL;
}

int main(int, char*[])
{
	cfunction<void *(*) (void *),  void*()> func = boost::bind(&my_thread_func, 1, 2, 3);

	pthread_t new_thread;

	pthread_create(&new_thread, NULL, func.c_func_ptr(), func.c_void_ptr());

	pthread_join(new_thread, NULL);
    return 0;
}

 *
 */
template<typename CFuncType, typename ClosureSignature>
class cfunction
{
public:
	typedef typename boost::function_traits<ClosureSignature>::result_type return_type;
	typedef boost::function<ClosureSignature> closure_type;

	cfunction(const closure_type &bindedfuntor)
	{
		m_wrapped_func.reset(new closure_type(bindedfuntor));
	}

	cfunction(const cfunction<CFuncType, ClosureSignature> & other)
	{
		m_wrapped_func.reset(new closure_type(* other.m_wrapped_func));
	}

	template<typename T>
	cfunction(T other)
	{
		m_wrapped_func.reset(new closure_type(other));
	}

	cfunction& operator = (const closure_type& bindedfuntor)
	{
		m_wrapped_func.reset(new closure_type(bindedfuntor));
		return *this;
	}

	cfunction& operator = (const cfunction& other)
	{
		m_wrapped_func.reset(new closure_type(* other.m_wrapped_func));
		return *this;
	}

	void * c_void_ptr()
	{
		return new closure_type(*m_wrapped_func);
	}

	CFuncType c_func_ptr()
	{
		return (CFuncType)wrapperd_callback;
	}

private: // 这里是一套重载, 被 c_func_ptr() 依据 C 接口的类型挑一个出来并实例化.
	static return_type wrapperd_callback(void* user_data)
	{
		boost::scoped_ptr<closure_type> wrapped_func(
					reinterpret_cast<closure_type *>(user_data));

		return (*wrapped_func)();
	}

	template<typename ARG1>
	static return_type wrapperd_callback(ARG1 arg1, void* user_data)
	{
		boost::scoped_ptr<closure_type> wrapped_func(
					reinterpret_cast<closure_type*>(user_data));

		return (*wrapped_func)(arg1);
	}

	template<typename ARG1, typename ARG2>
	static return_type wrapperd_callback(ARG1 arg1, ARG2 arg2, void* user_data)
	{
		boost::scoped_ptr<closure_type> wrapped_func(
					reinterpret_cast<closure_type*>(user_data));

		return (*wrapped_func)(arg1, arg2);
	}

	template<typename ARG1, typename ARG2, typename ARG3>
	static return_type wrapperd_callback(ARG1 arg1, ARG2 arg2, ARG3 arg3, void* user_data)
	{
		boost::scoped_ptr<closure_type> wrapped_func(
					reinterpret_cast<closure_type*>(user_data));

		return (*wrapped_func)(arg1, arg2, arg3);
	}

	template<typename ARG1, typename ARG2, typename ARG3, typename ARG4>
	static return_type wrapperd_callback(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, void* user_data)
	{
		boost::scoped_ptr<closure_type> wrapped_func(
					reinterpret_cast<closure_type*>(user_data));

		return (*wrapped_func)(arg1, arg2, arg3, arg4);
	}

	template<typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
	static return_type wrapperd_callback(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, void* user_data)
	{
		boost::scoped_ptr<closure_type> wrapped_func(
					reinterpret_cast<closure_type*>(user_data));

		return (*wrapped_func)(arg1, arg2, arg3, arg4, arg5);
	}

	template<typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
	static return_type wrapperd_callback(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, void* user_data)
	{
		boost::scoped_ptr<closure_type> wrapped_func(
					reinterpret_cast<closure_type*>(user_data));

		return (*wrapped_func)(arg1, arg2, arg3, arg4, arg5, arg6);
	}

	template<typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
	static return_type wrapperd_callback(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, void* user_data)
	{
		boost::scoped_ptr<closure_type> wrapped_func(
					reinterpret_cast<closure_type*>(user_data));

		return (*wrapped_func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
	}

	template<typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7, typename ARG8>
	static return_type wrapperd_callback(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8, void* user_data)
	{
		boost::scoped_ptr<closure_type> wrapped_func(
					reinterpret_cast<closure_type*>(user_data));

		return (*wrapped_func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
	}

	template<typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7, typename ARG8, typename ARG9>
	static return_type wrapperd_callback(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8, ARG9 arg9, void* user_data)
	{
		boost::scoped_ptr<closure_type> wrapped_func(
					reinterpret_cast<closure_type*>(user_data));

		return (*wrapped_func)(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
	}

private:
	boost::scoped_ptr<closure_type> m_wrapped_func;
};

} // namespace boost
