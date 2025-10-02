#pragma once

#include "model.h"
#include "socket_control.h"
#include "named_pipe_communication.h"

#include <utility>

#define PING 666
#define PONG 777

template <typename F>
class scope_exit {
	F func;
	bool active;
public:
	explicit scope_exit(F f) noexcept : func(std::move(f)), active(true) {}
	~scope_exit() noexcept { if (active) func(); }
	scope_exit(scope_exit&& other) noexcept : func(std::move(other.func)), active(other.active) {
		other.active = false;
	}
	scope_exit(const scope_exit&) = delete;
	scope_exit& operator=(const scope_exit&) = delete;
};

template <typename F>
scope_exit<F> make_scope_exit(F f) {
	return scope_exit<F>(std::move(f));
}

// Global list giữ lifetime cho class names đã đăng ký
inline std::list<std::wstring> g_registeredClasses;

inline HWND createHiddenWindow(HINSTANCE hInstance, const std::wstring& className) {
	try {
		WNDCLASSW wc = { 0 };
		wc.lpfnWndProc = CtlHiddenWndProc;
		wc.hInstance = hInstance;
		
		wc.lpszClassName = className.c_str();

		write_log(className);

		if (!RegisterClassW(&wc)) {
			DWORD err = GetLastError();
			if (err != ERROR_CLASS_ALREADY_EXISTS) {
				write_log(L"RegisterClassW failed");
				return NULL;
			}
		}

		HWND hwnd = CreateWindowExW(
			0,
			className.c_str(),
			className.c_str(), // window title
			0, 0, 0, 0, 0,
			HWND_MESSAGE, NULL, hInstance, NULL
		);

		if (hwnd == NULL) {
			write_log(L"Create windowExW failed");
			return NULL;
		}
		write_log(L"create window successfully");
		return hwnd;
	}
	catch (const std::exception& ex) {
		write_error(ex, 193);
		return NULL;
	}
	catch (...) {
		write_error();
		return NULL;
	}
}

inline constexpr wchar_t NOTIFICATION_SERVICE[] = L"_notification";

static DWORD WINAPI ThreadFunction(LPVOID lpParam) {
	std::unique_ptr<PluginSetting> param(reinterpret_cast<PluginSetting*>(lpParam));
	write_log(L"[Plugin] ", L"create hidden window");

	// copy chuỗi vào list để giữ lifetime
	std::wstring notification_title_ = utf8_to_wide(param->title_notification);
	g_registeredClasses.push_back(notification_title_);
	const std::wstring& classNameRef = g_registeredClasses.back();

	HWND hwnd = createHiddenWindow(GetModuleHandle(NULL), classNameRef);
	if (hwnd == NULL) {
		write_log(L"[Plugin] ", L"create hidden failed");
		return 1;
	}
	write_log(L"[Plugin] ", L"created hidden window");
	write_pid(hwnd);

    // Start Named Pipe server for child process
    std::wstring pipeName = GetPipeName(utf8_to_wide(param->title));
    g_pipeServer = std::make_unique<NamedPipeServer>(pipeName);
    
    if (g_pipeServer->Start(HandlePipeMessage)) {
        write_log(L"[Plugin] ", L"Named Pipe server started");
    } else {
        write_log(L"[Plugin] ", L"Failed to start Named Pipe server");
    }
    
    // Send "child ready" message to parent via Named Pipe
    std::wstring testMsg = L"child ready";
    std::wstring parentPipeName = GetPipeName(utf8_to_wide(param->title));
    NamedPipeClient parentClient(parentPipeName);
    
    if (parentClient.Connect()) {
        std::string msgUtf8 = wide_to_utf8(testMsg);
        PipeMessage message(PONG, msgUtf8);
        if (parentClient.SendMessage(message)) {
            write_log(L"[Plugin] ", L"sent ready message to parent via Named Pipe");
        } else {
            write_log(L"[Plugin] ", L"failed to send ready message via Named Pipe");
        }
    } else {
        write_log(L"[Plugin] ", L"failed to connect to parent via Named Pipe");
        
        // Fallback: try WM_COPYDATA
        COPYDATASTRUCT cds;
        cds.dwData = PONG;
        cds.cbData = (DWORD)((testMsg.size() + 1) * sizeof(wchar_t));
        cds.lpData = (PVOID)testMsg.c_str();

        // Tìm parent process bằng PID thay vì window title
        HWND hwndParent = NULL;
        DWORD parentPid = static_cast<DWORD>(param->appPid);
        
        struct EnumData {
            DWORD targetPid;
            HWND result;
        } enumData = { parentPid, NULL };
        
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            EnumData* data = (EnumData*)lParam;
            DWORD processId;
            GetWindowThreadProcessId(hwnd, &processId);
            if (processId == data->targetPid) {
                data->result = hwnd;
                return FALSE; // Dừng enum
            }
            return TRUE; // Tiếp tục enum
        }, (LPARAM)&enumData);
        
        hwndParent = enumData.result;
        
        if (hwndParent) {
            SendMessage(hwndParent, WM_COPYDATA, (WPARAM)hwnd, (LPARAM)&cds);
            write_log(L"[Plugin] ", L"sent ready message to parent via WM_COPYDATA");
        } else {
            write_log(L"[Plugin] ", L"parent not found");
        }
    }

	try {
		m_control = std::make_unique<WebSocketControl>();
		m_control->updateSettings(*param);
	}
	catch (const std::exception& ex) {
		write_error(ex, 209);
	}
	catch (...) {
		write_error();
	}
	try {
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			BOOL ret = GetMessage(&msg, NULL, 0, 0);
			if (ret <= 0) { // 0 = WM_QUIT, -1 = lỗi
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Cleanup Named Pipe server
		if (g_pipeServer) {
			g_pipeServer->Stop();
			g_pipeServer.reset();
		}
		
		write_log(L"[Plugin] ", L"end process");
		return 0;
	}
	catch (const std::exception&) {
		write_log(L"[Plugin] ", L"end process ex");
		throw;
	}
	catch (...) {
		write_log(L"[Plugin] ", L"end process uk");
		throw;
	}
}

struct MyProcess{
	PluginSetting settings{};
	std::wstring title_app{};
	std::wstring title_notification{};
	int mode = -1;

	MyProcess(std::wstring title,
		_In_ wchar_t* command_line) {
		try {
			write_log(L"[Plugin] ", L"register process");
			int numArgs = 0;
			LPWSTR* argv = CommandLineToArgvW(command_line, &numArgs);
			if (!argv) {
				write_log(L"[Plugin] ", L"CommandLineToArgvW failed");
				mode = -1;
				return;
			}

			auto free_argv = make_scope_exit([&]() { LocalFree(argv); });

			if (numArgs <= 1) {
				std::wstring title_ = title + std::wstring(NOTIFICATION_SERVICE);
				title_app = title;
				title_notification = title_;
				write_log(L"[Plugin] ", title);
				mode = 1;
				return;
			}

			// Kiểm tra tham số đầu tiên có phải "child"
			if (wcscmp(argv[1], L"child") != 0) {
				std::wstring title_ = title + std::wstring(NOTIFICATION_SERVICE);
				title_app = title;
				title_notification = title_;
				write_log(L"[Plugin] ", title);
				mode = 1;
				return;
			}

			// Nếu là child thì cần thêm tham số JSON
			if (numArgs <= 2) {
				write_log(L"[Plugin] ", L"child mode but missing JSON setting");
				mode = -1;
				return;
			}

			std::string setting_str = wide_to_utf8(argv[2]);
			auto settings_ = pluginSettingsFromJson(setting_str);

			settings = settings_;
			startThreadInChildProcess();
			mode = 0;
		}
		catch (const std::exception& ex) {
			write_error(ex, 109);
			mode = -1;
		}
		catch (...) {
			write_error();
			mode = -1;
		}
	}

	void startThreadInChildProcess() {
		try {
			auto* m_settings = new PluginSetting(settings);
			HANDLE hThread = CreateThread(
				NULL, 0, ThreadFunction, m_settings,
				0, NULL
			);

			if (hThread == NULL) {
				write_log(L"[Plugin] ", L"error create thread");
				std::wcerr << "Error create thread: " << GetLastError() << std::endl;
				return;
			}
            write_log(L"[Plugin] ", L"create thread ok");
			//WaitForSingleObject(hThread, INFINITE);
			CloseHandle(hThread);
		}
		catch (const std::exception& ex) {
			write_error(ex, 360);
		}
		catch (...) {
			write_error();
		}
	}
};