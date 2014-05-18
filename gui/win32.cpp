
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <atomic>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif
#if defined(_WIN32)
#include <direct.h>

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"
#endif

#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <boost/log/trivial.hpp>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <windows.h>

class auto_handle
{
	boost::shared_ptr<void> m_handle;
public:
	explicit auto_handle(HANDLE handle)
		: m_handle(handle, CloseHandle)
	{
	}

	operator HANDLE ()
	{
		return (HANDLE)m_handle.get();
	}
};


#include "fsconfig.ipp"

struct avbot_dlg_settings
{
	std::string& qqnumber;
	std::string& qqpwd;

	std::string& ircnick;
	std::string& ircpwd;
	std::string& ircroom;

	std::string& xmppuser;
	std::string& xmpppwd;
	std::string& xmpproom;
};

static void InitDlgSettings(HWND hwndDlg, avbot_dlg_settings & in)
{
	// 设定 窗口的文字
	SetDlgItemTextA(hwndDlg, IDC_EDIT_USER_NAME, in.qqnumber.c_str());
	SetDlgItemTextA(hwndDlg, IDC_EDIT_PWD, in.qqpwd.c_str());

	if (!in.ircnick.empty())
	{
		CheckDlgButton(hwndDlg, IDC_CHECK_IRC, 1);
		// 设定文字
		SetDlgItemTextA(hwndDlg, IDC_EDIT_IRC_CHANNEL, in.ircroom.c_str());
		SetDlgItemTextA(hwndDlg, IDC_EDIT_IRC_NICK, in.ircnick.c_str());
		SetDlgItemTextA(hwndDlg, IDC_EDIT_IRC_PWD, in.ircpwd.c_str());
	}
	else
	{
		CheckDlgButton(hwndDlg, IDC_CHECK_IRC, 0);
	}

	if (!in.xmppuser.empty())
	{
		CheckDlgButton(hwndDlg, IDC_CHECK_XMPP, 1);
		// 设定文字
		SetDlgItemTextA(hwndDlg, IDC_EDIT_XMPP_CHANNEL, in.xmpproom.c_str());
		SetDlgItemTextA(hwndDlg, IDC_EDIT_XMPP_NICK, in.xmppuser.c_str());
		SetDlgItemTextA(hwndDlg, IDC_EDIT_XMPP_PWD, in.xmpppwd.c_str());
	}
	else
	{
		CheckDlgButton(hwndDlg, IDC_CHECK_XMPP, 0);
	}

	if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_XMPP) != BST_CHECKED) {
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_CHANNEL), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_NICK), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_PWD), FALSE);
	}
	if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_IRC) != BST_CHECKED) {
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_IRC_CHANNEL), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_IRC_NICK), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_IRC_PWD), FALSE);
	}
}

static void ExtractDlgSettings(HWND hDlg, avbot_dlg_settings & out)
{
	// 看起来avbot用的是多字节,std::string not std::wstring
	TCHAR temp[MAX_PATH];
	bool use_xmpp = false;
	bool use_irc = false;
	// qq setting
	GetDlgItemText(hDlg, IDC_EDIT_USER_NAME, temp, MAX_PATH);
	out.qqnumber = temp;

	GetDlgItemText(hDlg, IDC_EDIT_PWD, temp, MAX_PATH);
	out.qqpwd = temp;

	// irc setting
	if (IsDlgButtonChecked(hDlg, IDC_CHECK_IRC) == BST_CHECKED)
	{
		GetDlgItemText(hDlg, IDC_EDIT_IRC_CHANNEL, temp, MAX_PATH);
		out.ircroom = temp;

		GetDlgItemText(hDlg, IDC_EDIT_IRC_NICK, temp, MAX_PATH);
		out.ircnick = temp;

		GetDlgItemText(hDlg, IDC_EDIT_IRC_PWD, temp, MAX_PATH);
		out.ircpwd = temp;
		use_irc = true;
	}

	// xmpp setting
	if (IsDlgButtonChecked(hDlg, IDC_CHECK_XMPP) == BST_CHECKED)
	{
		GetDlgItemText(hDlg, IDC_EDIT_XMPP_CHANNEL, temp, MAX_PATH);
		out.xmpproom = temp;

		GetDlgItemText(hDlg, IDC_EDIT_XMPP_NICK, temp, MAX_PATH);
		out.xmppuser = temp;

		GetDlgItemText(hDlg, IDC_EDIT_XMPP_PWD, temp, MAX_PATH);
		out.xmpppwd = temp;

		use_xmpp = true;
	}

	// save data to config file
	try {
		fs::path config_file;
		if (exist_config_file()) {
			config_file = configfilepath();
		}
		else {
			std::cerr << "config file not exist." << std::endl;
			TCHAR file_path[MAX_PATH];
			GetModuleFileName(NULL, file_path, MAX_PATH);
			config_file = fs::path(file_path).parent_path() / "qqbotrc";
		}

		std::ofstream file(config_file.string().c_str());

		file << "# qq config" << std::endl;
		file << "qqnum=" << out.qqnumber << std::endl;
		file << "qqpwd=" << out.qqpwd << std::endl;
		file << std::endl;

		if (use_irc)
		{
			file << "# irc config" << std::endl;
			file << "ircnick=" << out.ircnick << std::endl;
			file << "ircpwd=" << out.ircpwd << std::endl;
			file << "ircrooms=" << out.ircroom << std::endl;
			file << std::endl;
		}

		if (use_xmpp)
		{
			file << "# xmpp config" << std::endl;
			file << "xmppuser=" << out.xmppuser << std::endl;
			file << "xmpppwd=" << out.xmpppwd << std::endl;
			file << "xmpprooms=" << out.xmpproom << std::endl;
			file << std::endl;
		}
	}
	catch (...) {
		MessageBox(hDlg, TEXT("error while handle config file"), TEXT("avbot faital error"), MB_OK);
		exit(1);
	}
}

// 这里是真正的消息回调函数，支持闭包！
bool dlgproc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam, avbot_dlg_settings & settings)
{
	BOOL fError;

	switch (message)
	{
	case WM_INITDIALOG:
		InitDlgSettings(hwndDlg, settings);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			// 通知主函数写入数据并重启
		{
			ExtractDlgSettings(hwndDlg, settings);
			EndDialog(hwndDlg, (INT_PTR)&settings);
		}
			return TRUE;
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			// 退出消息循环
			return TRUE;
		case IDC_CHECK_XMPP:{
			BOOL enable = FALSE;
			if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_XMPP) == BST_CHECKED) enable = TRUE;

			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_CHANNEL), enable);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_NICK), enable);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_XMPP_PWD), enable);
		}return TRUE;
		case IDC_CHECK_IRC:{
			BOOL enable = FALSE;
			if (IsDlgButtonChecked(hwndDlg, IDC_CHECK_IRC) == BST_CHECKED) enable = TRUE;

			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_IRC_CHANNEL), enable);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_IRC_NICK), enable);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_IRC_PWD), enable);
		}return TRUE;
		}
	}

	return FALSE;
}

typedef boost::function<bool(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)> av_dlgproc_t;

namespace detail {
static BOOL CALLBACK internal_clusure_dlg_proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
}

void show_dialog(std::string & qqnumber, std::string & qqpwd, std::string & ircnick,
					std::string & ircroom, std::string & ircpwd,
					std::string &xmppuser, std::string & xmppserver,std::string & xmpppwd, std::string & xmpproom, std::string &xmppnick)
{
	// windows下面弹出选项设置框框.
	HMODULE hIns = GetModuleHandle(NULL);


	// 开启个模态对话框
	avbot_dlg_settings out = { qqnumber, qqpwd, ircnick, ircpwd, ircroom, xmppuser, xmpppwd, xmpproom };
	av_dlgproc_t * real_proc = new av_dlgproc_t(boost::bind(&dlgproc, _1, _2, _3, _4, boost::ref(out)));

	INT_PTR ret = DialogBoxParam(hIns, MAKEINTRESOURCE(IDD_AVSETTINGS_DIALOG), NULL, (DLGPROC)detail::internal_clusure_dlg_proc, (LPARAM)real_proc);

	// 处理返回值，消息循环里应该调用 	EndDialog() 退出模块对话框
	if (!ret)
	{
		// 退出 avbot
		exit(0);
	}
	assert(ret == reinterpret_cast<INT_PTR>(&out));
	// 返回值其实是一个指针，指向一块牛逼的内存。存放对话框记录的东西
}

namespace detail {

// 消息回调封装，间接调用闭包版
static BOOL CALLBACK internal_clusure_dlg_proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL ret = FALSE;

	if (message == WM_INITDIALOG)
	{
		SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
	}
	auto cb_ptr = GetWindowLongPtr(hwndDlg, DWLP_USER);
	av_dlgproc_t * real_callback_ptr = reinterpret_cast<av_dlgproc_t*>(cb_ptr);
	if (cb_ptr)
	{
		ret = (*real_callback_ptr)(hwndDlg, message, wParam, 0);
	}
	if (message == WM_DESTROY)
	{
		SetWindowLongPtr(hwndDlg, DWLP_USER, 0);
		if (real_callback_ptr)
			boost::checked_delete(real_callback_ptr);
	}
	return ret;
}

} // namespace detail

#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
