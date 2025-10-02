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

inline constexpr wchar_t NOTIFICATION_SERVICE[] = L"_notification";

static DWORD WINAPI ThreadFunction(LPVOID lpParam) {
	std::unique_ptr<PluginSetting> param(reinterpret_cast<PluginSetting*>(lpParam));
	write_log(L"[DEBUG] ", L"ThreadFunction started for child process");
	write_log(L"[Plugin] ", L"create hidden window");

	// No need for hidden window with Named Pipe communication
	// Just create a dummy HWND for PID tracking (backward compatibility)
	HWND hwnd = (HWND)0x12345678; // Dummy handle for compatibility
	write_log(L"[DEBUG] ", L"Using Named Pipe communication - no hidden window needed");
	write_log(L"[Plugin] ", L"Named Pipe mode - skipping hidden window");
	write_pid(hwnd); // Still save PID for WM_COPYDATA fallback

    // Child process does not need its own Named Pipe server
    // It will receive settings via the parent pipe connection
    write_log(L"[DEBUG] ", L"Child process - no Named Pipe server needed");
    write_log(L"[Plugin] ", L"Child process - will receive settings via parent pipe");
    
    // Send HELLO message to parent via Named Pipe (matching diagram)
    std::wstring parentPipeName = GetPipeName(utf8_to_wide(param->title));
    NamedPipeClient parentClient(parentPipeName);
    
    // Wait a bit for parent pipe to be ready
    Sleep(500);
    
    write_log(L"[DEBUG] ", (L"Attempting to connect to parent pipe: " + parentPipeName).c_str());
    write_log(L"[Plugin] ", (L"Child trying to connect to parent pipe: " + parentPipeName).c_str());
    
    // Try to connect with retry
    bool connected = false;
    for (int i = 0; i < 5; i++) {
        if (parentClient.Connect()) {
            connected = true;
            write_log(L"[SUCCESS] ", L"Child successfully connected to parent pipe!");
            break;
        }
        write_log(L"[DEBUG] ", (L"Connection attempt " + std::to_wstring(i + 1) + L" failed, retrying...").c_str());
        Sleep(200);
    }
    
    if (connected) {
        write_log(L"[DEBUG] ", L"Connected to parent pipe, sending HELLO message");
        
        // Create HELLO message (matching diagram)
        HelloMessage helloMsg;
        helloMsg.pid = GetCurrentProcessId();
        helloMsg.version = "1.0";
        
        std::string helloJson = toJson(helloMsg);
        PipeMessage message(PROTOCOL_HELLO, helloJson);
        
        if (parentClient.SendMessage(message)) {
            write_log(L"[DEBUG] ", L"HELLO message sent to parent via Named Pipe");
            write_log(L"[Plugin] ", L"sent HELLO message to parent via Named Pipe");
            write_log(L"[DEBUG] ", (std::wstring(L"HELLO JSON: ") + utf8_to_wide(helloJson)).c_str());
        } else {
            write_log(L"[DEBUG] ", L"Failed to send HELLO message via Named Pipe");
            write_log(L"[Plugin] ", L"failed to send HELLO message via Named Pipe");
        }
    } else {
        write_log(L"[DEBUG] ", L"Failed to connect to parent via Named Pipe");
        write_log(L"[Plugin] ", L"failed to connect to parent via Named Pipe");
        write_log(L"[ERROR] ", L"Child process cannot connect to parent pipe - will use WM_COPYDATA fallback");
        
        // Fallback: try WM_COPYDATA
        std::wstring testMsg = L"child ready";
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