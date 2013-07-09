
#include <windows.h>

// 重启qqbot
#define WM_RESTART_AV_BOT WM_USER + 5

// 选项设置框框的消息回调函数
BOOL CALLBACK DlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL fError;

	switch (message)
	{
	case WM_INITDIALOG:
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
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			// 通知主函数写入数据并重启
			PostMessage(NULL, WM_RESTART_AV_BOT, 0, 0);
			return TRUE;
		case IDCANCEL:
			DestroyWindow(hwndDlg);
			// 退出消息循环
			PostMessage(NULL, WM_QUIT, 0, 0);
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



void show_dialog(std::string & qqnumer, std::string & qqpwd, std::string & ircnick,
					std::string & ircroom, std::string & ircpwd,
					std::string &xmppuser, std::string & xmppserver,std::string & xmpppwd, std::string & xmpproom)
{
	::InitCommonControls();
	// windows下面弹出选项设置框框.
	HMODULE hIns = GetModuleHandle(NULL);
	HWND hDlg = NULL;

	hDlg = CreateDialog(hIns, MAKEINTRESOURCE(IDD_AVSETTINGS_DIALOG), NULL, (DLGPROC)DlgProc);

	ShowWindow(hDlg, SW_SHOW);

	// 启动windows消息循环.
	MSG msg;
	while (true)
	{
		if (GetMessage(&msg, NULL, 0, 0))
		{
			if (msg.message == WM_QUIT) exit(1);

			if (msg.message == WM_RESTART_AV_BOT)
			{
				// 看起来avbot用的是多字节,std::string not std::wstring
				TCHAR temp[MAX_PATH];
				bool use_xmpp = false;
				bool use_irc = false;
				// qq setting
				GetDlgItemText(hDlg, IDC_EDIT_USER_NAME, temp, MAX_PATH);
				qqnumber = temp;

				GetDlgItemText(hDlg, IDC_EDIT_PWD, temp, MAX_PATH);
				qqpwd = temp;

				// irc setting
				if (IsDlgButtonChecked(hDlg, IDC_CHECK_IRC) == BST_CHECKED)
				{
					GetDlgItemText(hDlg, IDC_EDIT_IRC_CHANNEL, temp, MAX_PATH);
					ircroom = temp;

					GetDlgItemText(hDlg, IDC_EDIT_IRC_NICK, temp, MAX_PATH);
					ircnick = temp;

					GetDlgItemText(hDlg, IDC_EDIT_IRC_PWD, temp, MAX_PATH);
					ircpwd = temp;
					use_irc = true;
				}

				// xmpp setting
				if (IsDlgButtonChecked(hDlg, IDC_CHECK_XMPP) == BST_CHECKED)
				{
					GetDlgItemText(hDlg, IDC_EDIT_XMPP_CHANNEL, temp, MAX_PATH);
					xmpproom = temp;

					GetDlgItemText(hDlg, IDC_EDIT_XMPP_NICK, temp, MAX_PATH);
					xmppnick = temp;

					GetDlgItemText(hDlg, IDC_EDIT_XMPP_PWD, temp, MAX_PATH);
					xmpppwd = temp;

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
						config_file = fs::path(file_path).parent_path();
						config_file += "\\qqbotrc";
					}

					fs::ofstream file(config_file);

					file << "# qq config" << std::endl;
					file << "qqnum=" << qqnumber << std::endl;
					file << "qqpwd=" << qqpwd << std::endl;
					file << std::endl;

					if(use_irc)
					{
						file << "# irc config" << std::endl;
						file << "ircnick=" << ircnick << std::endl;
						file << "ircpwd=" << ircpwd << std::endl;
						file << "ircrooms=" << ircroom << std::endl;
						file << std::endl;
					}

					if (use_xmpp)
					{
						file << "# xmpp config" << std::endl;
						file << "xmppuser=" << xmppnick << std::endl;
						file << "xmpppwd=" << xmpppwd << std::endl;
						file << "xmpprooms=" << xmpproom << std::endl;
						file << std::endl;
					}
				}
				catch (...) {
					BOOST_LOG_TRIVIAL(fatal) << "error while handle config file";
				}

				TCHAR file_path[MAX_PATH];
				GetModuleFileName(NULL, file_path, MAX_PATH);
				// now, create process
				STARTUPINFO si;
				PROCESS_INFORMATION pi;

				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				ZeroMemory(&pi, sizeof(pi));

				CreateProcess(file_path, NULL, NULL, NULL,
					FALSE, 0, NULL, NULL,
					&si, &pi);

				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				// now,parent process exit.
				exit(1);
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}
