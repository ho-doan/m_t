#include "local_push_connectivity_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <sstream>

#include "messages.g.h"
#include "win_toast.h"
#include "utils.h"
#include "process.h"

#include <cstdlib>

#include <TlHelp32.h>

namespace local_push_connectivity {
    // static
    void LocalPushConnectivityPlugin::RegisterWithRegistrar(
        flutter::PluginRegistrarWindows* registrar) {
        try {
            auto plugin = std::make_unique<LocalPushConnectivityPlugin>();
            _flutterApi = std::make_unique<LocalPushConnectivityPigeonFlutterApi>(registrar->messenger());
            LocalPushConnectivityPigeonHostApi::SetUp(registrar->messenger(), plugin.get());
            registrar->AddPlugin(std::move(plugin));
        }
        catch (const std::exception& ex) {
            write_error(ex, 57);
            write_log(L"[Plugin error]", L"err ex");
        }
        catch (...) {
            write_error();
            write_log(L"[Plugin error]", L"err uk");
        }
    }

    LocalPushConnectivityPlugin::LocalPushConnectivityPlugin() {}

    LocalPushConnectivityPlugin::~LocalPushConnectivityPlugin() {}
    std::unique_ptr<LocalPushConnectivityPigeonFlutterApi> LocalPushConnectivityPlugin::_flutterApi = NULL;
    std::wstring LocalPushConnectivityPlugin::_title = L"";
    bool LocalPushConnectivityPlugin::_flutterApiReady = false;
    std::wstring LocalPushConnectivityPlugin::_title_notification = L"";
    int LocalPushConnectivityPlugin::RegisterProcess(std::wstring title,
        _In_ wchar_t* command_line) {
        MyProcess p(title, command_line);
        return p.mode;
    }

    std::function<void(ErrorOr<bool> reply)> _result = NULL;

    void LocalPushConnectivityPlugin::HandleMessage(HWND window, UINT const message,
        LPARAM const lparam) {
        if (message == WM_COPYDATA) {
            auto cp_struct = reinterpret_cast<COPYDATASTRUCT*>(lparam);
            if (cp_struct->cbData == PONG) {
                if (_result) {
                    _result(true);
                    _result = NULL;
                }
            }
            else {
                try {
                    size_t len = cp_struct->cbData / sizeof(wchar_t);
                    if (len > 0 && ((wchar_t*)cp_struct->lpData)[len - 1] == L'\0') {
                        len -= 1; // bỏ null-terminator
                    }
                    std::wstring str((wchar_t*)cp_struct->lpData, len);
                    //std::wstring str(data, cp_struct->cbData / sizeof(wchar_t));
                    if (_flutterApi) {
                        NotificationPigeon n = NotificationPigeon("n", "n");
                        MessageResponsePigeon mR = MessageResponsePigeon(n, wide_to_utf8(str));
                        MessageSystemPigeon m = MessageSystemPigeon(false, mR);
                        _flutterApi->OnMessage(m,
                            []() {
                                std::cout << "Message sent ok";
                            },
                            [](const FlutterError& e) {
                                std::cout << "Message send error: " << e.code()
                                    << " - " << e.message();
                            });
                    }
                }
                catch (const std::exception& ex) {
                    write_error(ex, 142);
                    throw;
                }
                catch (...) {
                    write_error();
                    throw;
                }
            }
        }
    }

    void LocalPushConnectivityPlugin::sendSettings() {
        try {
            PluginSetting settings = gSetting();
            std::string commandLine = pluginSettingsToJson(settings);
            HWND hwndChild = read_pid();
            if (!hwndChild) {
                write_log(L"[ERROR SETTINGS] ", L"Process null");
                MessageBoxA(NULL, commandLine.c_str(), "Child exe notification is null", MB_OK);
                return;
            }
            if (!IsWindow(hwndChild)) {
                write_log(L"[ERROR SETTINGS] ", L"Invalid hwndChild");
                return;
            }
            std::wstring c = utf8_to_wide(commandLine);
            COPYDATASTRUCT cds;
            cds.dwData = 100;
            cds.cbData = (DWORD)(wcslen(c.c_str()) + 1) * sizeof(wchar_t);
            cds.lpData = (PVOID)c.c_str();
            write_log(L"[SETTINGS] ", L"Sending...");
            DWORD_PTR result;
            LRESULT res = SendMessageTimeoutW(hwndChild, WM_COPYDATA, (WPARAM)GetCurrentProcessId(),
                (LPARAM)&cds, SMTO_NORMAL, 5000, &result);
            if (res == 0) {
                write_log(L"[SETTINGS] ", L"Send timeout...");
            }
            write_log(L"[SETTINGS] ", L"Sended...");
        }
        catch (const std::exception& ex) {
            write_error(ex, 253);
            write_log(L"[ERROR SETTINGS] ", L"Send error ex");
            return;
        }
        catch (...) {
            write_error();
            write_log(L"[ERROR SETTINGS] ", L"Send error unknown");
            return;
        }
    }

    void LocalPushConnectivityPlugin::FlutterApiReady(
        std::function<void(std::optional<FlutterError> reply)> result) {
        LocalPushConnectivityPlugin::_flutterApiReady = true;
        /*if (_m) {
            sendMessageFromNoti(*_m);
        }*/
        result(std::nullopt);
    }

    std::optional<MessageSystemPigeon> LocalPushConnectivityPlugin::_m = std::nullopt;

    void LocalPushConnectivityPlugin::sendMessageFromNoti(MessageSystemPigeon m) {
        try {
            if (!_flutterApiReady) {
                _m = m;
                return;
            }
            if (_flutterApi) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                _flutterApi->OnMessage(m,
                    []() {
                        std::cout << "Message sent ok";
                    },
                    [](const FlutterError& e) {
                        std::stringstream err;
                        err << "send fail err: " << e.message();
                        MessageBoxA(NULL, err.str().c_str(), "sendMessageFromNoti", MB_OK);
                        std::cout << "Message send error: " << e.code()
                            << " - " << e.message();
                    });
                return;
            }
            else {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                sendMessageFromNoti(m);
            }
        }
        catch (const std::exception& ex) {
            write_error(ex, 392);
        }
        catch (...) {
            write_error();
        }
    }

    void LocalPushConnectivityPlugin::Initialize(int64_t system_type, const AndroidSettingsPigeon* android,
        const WindowsSettingsPigeon* windows,
        const IosSettingsPigeon* ios,
        const TCPModePigeon& mode,
        std::function<void(ErrorOr<bool> reply)> result) {
        try {
            //result(FlutterError("NNN", "detail"));
            std::string public_has_key = "-";
            std::string path = "-";
            if (mode.public_has_key() != nullptr) {
                public_has_key = wide_to_utf8(utf8_to_wide_2(mode.public_has_key()));
            }
            if (mode.path() != nullptr) {
                path = wide_to_utf8(utf8_to_wide_2(mode.path()));
            }
            long long pid = static_cast<long long>(GetCurrentProcessId());
            auto settings = PluginSetting{
                pid,
                wide_to_utf8(title()),
                wide_to_utf8(title_notification()),
                windows->bundle_id(),
                windows->display_name(),
                windows->icon(),
                windows->icon_content(),
                mode.host(),
                mode.port(),
                system_type,
                public_has_key,
                mode.connection_type() == ConnectionType::kWss,
                path,
                mode.connection_type() == ConnectionType::kTcp
            };
            LocalPushConnectivityPlugin::saveSetting(settings);
            if (windows != nullptr) {
                std::wstring pathIcon = get_current_path() + std::wstring(L"\\data\\flutter_assets\\") + utf8_to_wide(windows->icon());
                DesktopNotificationManagerCompat::Register(utf8_to_wide(windows->bundle_id()),
                    utf8_to_wide(windows->display_name()), pathIcon);

                settings.iconPath = wide_to_utf8(pathIcon);
                auto notification_title = utf8_to_wide(settings.title_notification);
                LocalPushConnectivityPlugin::saveSetting(settings);

                DesktopNotificationManagerCompat::OnActivated([this](
                    DesktopNotificationActivatedEventArgsCompat data) {
                        std::wstring tag = data.Argument();

                        std::string tagStr = base64_decode(wide_to_utf8(tag));

                        NotificationPigeon n = NotificationPigeon("n", "n");
                        MessageResponsePigeon mR = MessageResponsePigeon(n, tagStr);
                        MessageSystemPigeon m = MessageSystemPigeon(false, mR);
                        m.set_from_notification(true);

                        sendMessageFromNoti(m);
                    });
                HWND hwndChild = FindWindow(notification_title.c_str(), NULL);
                if (!hwndChild) {
                    auto status = LocalPushConnectivityPlugin::createBackgroundProcess();
                    if (status == -8) {
                        _result = result;
                        return;
                    }
                    result(true);
                    return;
                }
                else {
                    LocalPushConnectivityPlugin::sendSettings();
                }

                result(true);
            }
            else {
                result(false);
            }
        }
        catch (const std::exception& ex) {
            write_error(ex, 475);
            result(false);
        }
        catch (...) {
            write_error();
            result(false);
        }
    }

    void LocalPushConnectivityPlugin::RegisterUser(
        const UserPigeon& user,
        std::function<void(ErrorOr<bool> reply)> result) {
        try {
            PluginSetting settings = gSetting();
            settings.connector_id = user.connector_i_d();
            settings.connector_tag = user.connector_tag();
            saveSetting(settings);
            sendSettings();
            result(true);
        }
        catch (const std::exception& ex) {
            write_error(ex, 496);
            result(false);
        }
        catch (...) {
            write_error();
            result(false);
        }
    }
    void LocalPushConnectivityPlugin::DeviceID(std::function<void(ErrorOr<std::string> reply)> result) {
        try {
            result(wide_to_utf8(get_sys_device_id()));
        }
        catch (const std::exception& ex) {
            write_error(ex, 509);
            result(std::string(""));
        }
        catch (...) {
            write_error();
            result(std::string(""));
        }
    }

    void LocalPushConnectivityPlugin::Config(
        const TCPModePigeon& mode,
        const flutter::EncodableList* ssids,
        std::function<void(ErrorOr<bool> reply)> result) {
        try {
            PluginSetting settings = LocalPushConnectivityPlugin::gSetting();
            settings.host = mode.host();
            settings.port = mode.port();
            settings.publicHasKey = mode.public_has_key() == nullptr ? "-" :
                wide_to_utf8(utf8_to_wide_2(mode.public_has_key()));
            settings.wss = mode.connection_type() == ConnectionType::kWss;
            settings.path = mode.path() == nullptr ? "-" :
                wide_to_utf8(utf8_to_wide_2(mode.path()));
            saveSetting(settings);
            sendSettings();
            result(true);
        }
        catch (const std::exception& ex) {
            write_error(ex, 536);
            result(false);
            write_log(L"[ERROR CONFIG] ", L"Error...");
            return;
        }
        catch (...) {
            result(false);
            write_log(L"[ERROR CONFIG] ", L"Error u...");
            write_error();
            throw;
        }
    }

    void LocalPushConnectivityPlugin::RequestPermission(std::function<void(ErrorOr<bool> reply)> result) {
        result(true);
    }

    void LocalPushConnectivityPlugin::Start(std::function<void(ErrorOr<bool> reply)> result) {
        result(true);
    }

    void LocalPushConnectivityPlugin::Stop(std::function<void(ErrorOr<bool> reply)> result) {
        try {
            std::wstring c(L"logout");
            COPYDATASTRUCT cds;
            cds.dwData = 99;
            cds.cbData = (DWORD)(wcslen(c.c_str()) + 1) * sizeof(wchar_t);
            cds.lpData = (PVOID)c.c_str();
            HWND hwndChild = read_pid();
            if (!hwndChild) {
                result(true);
                return;
            }
            SendMessageW(hwndChild, WM_COPYDATA, (WPARAM)GetCurrentProcessId(),
                (LPARAM)&cds);
            result(true);
        }
        catch (...) {
            write_error();
            result(false);
        }
    }

    PluginSetting LocalPushConnectivityPlugin::_settings{};

    int LocalPushConnectivityPlugin::createBackgroundProcess() {
        try {
            auto oldPid = read_pid();
            if (oldPid && IsWindow(oldPid)) {
                return -1;
            }

            PROCESS_INFORMATION pi;
            STARTUPINFO si = { 0 };
            si.cb = sizeof(STARTUPINFO);
            wchar_t execuablePath[MAX_PATH];

            if (GetModuleFileName(NULL, execuablePath, MAX_PATH) == 0) {
                std::wcerr << L"get module file name error: " << GetLastError() << std::endl;
                return -1;
            }

            PluginSetting settings = LocalPushConnectivityPlugin::gSetting();
            std::string m_settings = pluginSettingsToJson(settings);

            std::wstring wideSettings = utf8_to_wide(m_settings);
            std::wstring commandLine = L"\"" + std::wstring(execuablePath) + L"\" child \"" + wideSettings + L"\"";

            std::wcout << "exe service notification: " << commandLine << L"\n";
            if (CreateProcess(NULL, commandLine.data(), NULL, NULL,
                FALSE, 0, NULL, NULL, &si, &pi)) {
                std::wcout << L"create process ok";

                auto noti_title = utf8_to_wide(settings.title_notification);

                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                return -8;
            }
            else {
                std::wcerr << L"create process error" << GetLastError() << std::endl;
            }
            return -1;
        }
        catch (const std::exception& ex) {
            write_error(ex, 299);
            return -1;
        }
        catch (...) {
            write_error();
            return -1;
        }
    }
}  // namespace local_push_connectivity