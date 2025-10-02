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
#include <atomic>

#include "messages.g.h"
#include "win_toast.h"
#include "utils.h"
#include "process.h"
#include "named_pipe_communication.h"

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

    // Global variables (must be declared before any methods that use them)
    std::unique_ptr<LocalPushConnectivityPigeonFlutterApi> LocalPushConnectivityPlugin::_flutterApi = NULL;
    std::wstring LocalPushConnectivityPlugin::_title = L"";
    bool LocalPushConnectivityPlugin::_flutterApiReady = false;
    std::wstring LocalPushConnectivityPlugin::_title_notification = L"";
    
    // Global Named Pipe server for parent process
    std::unique_ptr<NamedPipeServer> g_parentPipeServer;
    
    // Flag to prevent multiple process creation
    std::atomic<bool> g_creatingProcess{ false };
    
    // Flag to prevent multiple initialization
    std::atomic<bool> g_initialized{ false };
    
    // Counter to prevent infinite process creation
    std::atomic<int> g_processCreationCount{ 0 };

    LocalPushConnectivityPlugin::LocalPushConnectivityPlugin() {
        write_log(L"[DEBUG] ", L"LocalPushConnectivityPlugin constructor called");
    }

    LocalPushConnectivityPlugin::~LocalPushConnectivityPlugin() {
        write_log(L"[DEBUG] ", L"LocalPushConnectivityPlugin destructor called");
        // Cleanup when plugin is destroyed
        if (g_parentPipeServer) {
            write_log(L"[DEBUG] ", L"Stopping parent pipe server");
            g_parentPipeServer->Stop();
            g_parentPipeServer.reset();
        }
        g_initialized.store(false);
        g_creatingProcess.store(false);
        g_processCreationCount.store(0);
        write_log(L"[Plugin] ", L"Plugin destroyed, cleaned up resources");
        write_log(L"[DEBUG] ", L"Plugin cleanup completed");
    }
    int LocalPushConnectivityPlugin::RegisterProcess(std::wstring title,
        _In_ wchar_t* command_line) {
        write_log(L"[DEBUG] ", (L"RegisterProcess called with title: " + title).c_str());
        MyProcess p(title, command_line);
        write_log(L"[DEBUG] ", (L"RegisterProcess returning mode: " + std::to_wstring(p.mode)).c_str());
        return p.mode;
    }

    std::function<void(ErrorOr<bool> reply)> _result = NULL;
    
    // Message handler for parent process Named Pipe
    void HandleParentPipeMessage(const PipeMessage& message) {
        try {
            switch (message.command) {
            case PROTOCOL_HELLO: {
                write_log(L"[Plugin] ", L"Received HELLO from child process via Named Pipe");
                write_log(L"[DEBUG] ", (std::wstring(L"HELLO JSON: ") + utf8_to_wide(message.data)).c_str());
                
                // Send HELLO_ACK response (matching diagram)
                HelloAckMessage ackMsg;
                ackMsg.ok = true;
                ackMsg.server_time = getCurrentTimeMs();
                
                std::string ackJson = toJson(ackMsg);
                std::wstring pipeName = GetPipeName(utf8_to_wide(LocalPushConnectivityPlugin::gSetting().title));
                NamedPipeClient client(pipeName);
                
                if (client.Connect()) {
                    PipeMessage ackMessage(PROTOCOL_HELLO_ACK, ackJson);
                    if (client.SendMessage(ackMessage)) {
                        write_log(L"[Plugin] ", L"HELLO_ACK sent to child via Named Pipe");
                        write_log(L"[DEBUG] ", (std::wstring(L"HELLO_ACK JSON: ") + utf8_to_wide(ackJson)).c_str());
                    }
                    client.Disconnect();
                }
                
                if (_result) {
                    // Gửi settings ngay sau khi nhận được HELLO
                    LocalPushConnectivityPlugin::sendSettings();
                    _result(true);
                    _result = NULL;
                }
                break;
            }
            case PONG: {
                if (_result) {
                    write_log(L"[Plugin] ", L"Received PONG from child process via Named Pipe");
                    // Gửi settings ngay sau khi nhận được PONG
                    LocalPushConnectivityPlugin::sendSettings();
                    _result(true);
                    _result = NULL;
                }
                break;
            }
            case PIPE_CMD_UPDATE_SETTINGS: {
                write_log(L"[Plugin] ", L"Received UPDATE_SETTINGS command from child via Named Pipe");
                // This should not happen - parent should not receive UPDATE_SETTINGS from child
                // Child should only send PONG, not UPDATE_SETTINGS
                write_log(L"[Plugin] ", L"WARNING: Child sent UPDATE_SETTINGS to parent - this is unexpected");
                break;
            }
            case PROTOCOL_SOCKET_EVENT: {
                write_log(L"[Plugin] ", L"Received SOCKET_EVENT from child via Named Pipe");
                write_log(L"[DEBUG] ", (std::wstring(L"SOCKET_EVENT JSON: ") + utf8_to_wide(message.data)).c_str());
                
                if (LocalPushConnectivityPlugin::_flutterApi) {
                    NotificationPigeon n = NotificationPigeon("n", "n");
                    MessageResponsePigeon mR = MessageResponsePigeon(n, message.data);
                    MessageSystemPigeon m = MessageSystemPigeon(false, mR);
                    LocalPushConnectivityPlugin::_flutterApi->OnMessage(m,
                        []() {
                            write_log(L"[Plugin] ", L"Message sent ok");
                        },
                        [](const FlutterError& e) {
                            write_log(L"[Plugin] ", (L"Message send error: " + utf8_to_wide(e.code()) + L" - " + utf8_to_wide(e.message())).c_str());
                        });
                }
                break;
            }
            case PROTOCOL_PONG: {
                write_log(L"[Plugin] ", L"Received PONG from child via Named Pipe");
                write_log(L"[DEBUG] ", (std::wstring(L"PONG JSON: ") + utf8_to_wide(message.data)).c_str());
                break;
            }
            case 1: { // Legacy message from child (backward compatibility)
                if (LocalPushConnectivityPlugin::_flutterApi) {
                    NotificationPigeon n = NotificationPigeon("n", "n");
                    MessageResponsePigeon mR = MessageResponsePigeon(n, message.data);
                    MessageSystemPigeon m = MessageSystemPigeon(false, mR);
                    LocalPushConnectivityPlugin::_flutterApi->OnMessage(m,
                        []() {
                            write_log(L"[Plugin] ", L"Message sent ok");
                        },
                        [](const FlutterError& e) {
                            write_log(L"[Plugin] ", (L"Message send error: " + utf8_to_wide(e.code()) + L" - " + utf8_to_wide(e.message())).c_str());
                        });
                }
                break;
            }
            default:
                write_log(L"[Plugin] ", (L"Unknown command from child: " + std::to_wstring(message.command)).c_str());
                break;
            }
        }
        catch (const std::exception& ex) {
            write_error(ex, 1003);
        }
        catch (...) {
            write_error();
        }
    }

    void LocalPushConnectivityPlugin::HandleMessage(HWND window, UINT const message,
        LPARAM const lparam) {
        if (message == WM_COPYDATA) {
            auto cp_struct = reinterpret_cast<COPYDATASTRUCT*>(lparam);
            if (cp_struct->cbData == PONG) {
                if (_result) {
                    write_log(L"[Plugin] ", L"Received PONG from child process via WM_COPYDATA");
                    // Gửi settings ngay sau khi nhận được PONG
                    LocalPushConnectivityPlugin::sendSettings();
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
            
            // Try Named Pipe first - connect to child pipe
            std::wstring pipeName = GetPipeName(utf8_to_wide(settings.title)) + L"_child";
            NamedPipeClient client(pipeName);
            
            // Try to connect with timeout to avoid blocking
            bool connected = false;
            for (int i = 0; i < 3; i++) {
                if (client.Connect()) {
                    connected = true;
                    break;
                }
                Sleep(200); // Wait 200ms before retry to avoid pipe busy
            }
            
            if (connected) {
                // Create SET_URL message (matching diagram)
                SetUrlMessage setUrlMsg;
                setUrlMsg.id = "m1"; // Message ID
                setUrlMsg.url = settings.uri();
                setUrlMsg.opts = "{}"; // Empty options for now
                
                std::string setUrlJson = toJson(setUrlMsg);
                PipeMessage message(PROTOCOL_SET_URL, setUrlJson);
                
                if (client.SendMessage(message)) {
                    write_log(L"[SETTINGS] ", L"SET_URL sent via Named Pipe");
                    write_log(L"[DEBUG] ", (std::wstring(L"SET_URL JSON: ") + utf8_to_wide(setUrlJson)).c_str());
                    client.Disconnect();
                    return;
                } else {
                    write_log(L"[SETTINGS] ", L"Failed to send SET_URL via Named Pipe");
                }
                client.Disconnect();
            } else {
                write_log(L"[SETTINGS] ", L"Failed to connect via Named Pipe");
            }
            
            // Fallback to WM_COPYDATA (only if Named Pipe fails)
            write_log(L"[SETTINGS] ", L"Named Pipe failed, trying WM_COPYDATA fallback...");
            
            // Try to read PID with retry (child process might not have saved PID yet)
            HWND hwndChild = NULL;
            for (int i = 0; i < 5; i++) {
                hwndChild = read_pid();
                if (hwndChild) {
                    write_log(L"[SETTINGS] ", L"Found child process PID");
                    break;
                }
                write_log(L"[SETTINGS] ", L"Waiting for child process to save PID...");
                Sleep(200); // Wait 200ms for child to save PID
            }
            
            if (!hwndChild) {
                write_log(L"[ERROR SETTINGS] ", L"Process null - child process not found after retries");
                write_log(L"[ERROR SETTINGS] ", L"Please check if child process is running");
                return;
            }
            if (!IsWindow(hwndChild)) {
                write_log(L"[ERROR SETTINGS] ", L"Invalid hwndChild");
                return;
            }
            // Create legacy settings message for WM_COPYDATA fallback
            std::string commandLine = pluginSettingsToJson(settings);
            std::wstring c = utf8_to_wide(commandLine);
            COPYDATASTRUCT cds;
            cds.dwData = 100;
            cds.cbData = (DWORD)(wcslen(c.c_str()) + 1) * sizeof(wchar_t);
            cds.lpData = (PVOID)c.c_str();
            write_log(L"[SETTINGS] ", L"Sending via WM_COPYDATA...");
            DWORD_PTR result;
            LRESULT res = SendMessageTimeoutW(hwndChild, WM_COPYDATA, (WPARAM)GetCurrentProcessId(),
                (LPARAM)&cds, SMTO_NORMAL, 5000, &result);
            if (res == 0) {
                write_log(L"[SETTINGS] ", L"Send timeout...");
            }
            write_log(L"[SETTINGS] ", L"Sended via WM_COPYDATA...");
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
            write_log(L"[DEBUG] ", (L"Initialize called with system_type: " + std::to_wstring(system_type)).c_str());
            write_log(L"[DEBUG] ", (L"g_initialized.load(): " + std::to_wstring(g_initialized.load())).c_str());
            
            // Check if already initialized
            if (g_initialized.load()) {
                write_log(L"[DEBUG] ", L"Already initialized, skipping");
                write_log(L"[Plugin] ", L"Already initialized, skipping");
                result(true);
                return;
            }
            
            write_log(L"[DEBUG] ", L"Starting initialization...");
            write_log(L"[Plugin] ", L"Starting initialization...");
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
                
                // Start Named Pipe server for parent process (only if not already started)
                if (!g_parentPipeServer) {
                    write_log(L"[DEBUG] ", L"Creating new Named Pipe server");
                    std::wstring pipeName = GetPipeName(utf8_to_wide(settings.title));
                    write_log(L"[DEBUG] ", (L"Pipe name: " + pipeName).c_str());
                    g_parentPipeServer = std::make_unique<NamedPipeServer>(pipeName);
                    
                    if (g_parentPipeServer->Start(HandleParentPipeMessage)) {
                        write_log(L"[DEBUG] ", L"Parent Named Pipe server started successfully");
                        write_log(L"[Plugin] ", L"Parent Named Pipe server started");
                    } else {
                        write_log(L"[DEBUG] ", L"Failed to start Parent Named Pipe server");
                        write_log(L"[Plugin] ", L"Failed to start Parent Named Pipe server");
                    }
                } else {
                    write_log(L"[DEBUG] ", L"Parent Named Pipe server already running");
                    write_log(L"[Plugin] ", L"Parent Named Pipe server already running");
                }

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
                write_log(L"[DEBUG] ", L"Checking if child process is ready via Named Pipe...");
                
                // Check if child process is ready via Named Pipe instead of FindWindow
                std::wstring pipeName = GetPipeName(utf8_to_wide(settings.title));
                NamedPipeClient testClient(pipeName);
                
                // Try to connect with timeout to avoid blocking
                bool connected = false;
                for (int i = 0; i < 3; i++) {
                    if (testClient.Connect()) {
                        connected = true;
                        break;
                    }
                    Sleep(100); // Wait 100ms before retry
                }
                
                if (!connected) {
                    write_log(L"[DEBUG] ", L"No child process found");
                    write_log(L"[DEBUG] ", (L"g_creatingProcess.load(): " + std::to_wstring(g_creatingProcess.load())).c_str());
                    
                    // Extract counter from command_line and increment
                    std::wstring commandLine = GetCommandLineW();
                    int currentCount = 0;
                    
                    // Try to extract counter from command line (format: "app.exe child settings $cout:5")
                    size_t coutPos = commandLine.find(L"$cout:");
                    if (coutPos != std::wstring::npos) {
                        size_t startPos = coutPos + 6; // Skip "$cout:"
                        size_t endPos = commandLine.find(L" ", startPos);
                        if (endPos == std::wstring::npos) endPos = commandLine.length();
                        
                        std::wstring countStr = commandLine.substr(startPos, endPos - startPos);
                        try {
                            currentCount = std::stoi(countStr);
                        } catch (...) {
                            currentCount = 0;
                        }
                    }
                    
                    currentCount++; // Increment counter
                    write_log(L"[DEBUG] ", (L"Process creation count: " + std::to_wstring(currentCount) + L" $cout").c_str());
                    
                    if (currentCount >= 10) {
                        write_log(L"[ERROR] ", L"Maximum process creation limit reached (10), throwing error $cout");
                        throw std::runtime_error("Maximum process creation limit reached (10)");
                    }
                    
                    // Check if we're already creating a process
                    if (g_creatingProcess.load()) {
                        write_log(L"[DEBUG] ", L"Process creation already in progress, skipping");
                        write_log(L"[Plugin] ", L"Process creation already in progress, skipping");
                        result(true);
                        return;
                    }
                    
                    write_log(L"[DEBUG] ", L"Creating new child process...");
                    write_log(L"[Plugin] ", L"No child process found, creating new one");
                    g_creatingProcess.store(true);
                    auto status = LocalPushConnectivityPlugin::createBackgroundProcess(currentCount);
                    write_log(L"[DEBUG] ", (L"createBackgroundProcess returned: " + std::to_wstring(status)).c_str());
                    
                    if (status == -8) {
                        write_log(L"[DEBUG] ", L"Waiting for child process to be ready...");
                        // Chờ child process khởi tạo xong với timeout
                        _result = result;
                        LocalPushConnectivityPlugin::waitForChildProcessReady();
                        g_creatingProcess.store(false);
                        return;
                    }
                    g_creatingProcess.store(false);
                    result(true);
                    return;
                }
                else {
                    write_log(L"[DEBUG] ", L"Child process already exists via Named Pipe, sending settings");
                    write_log(L"[Plugin] ", L"Child process already exists, sending settings");
                    LocalPushConnectivityPlugin::sendSettings();
                    // Reset counter when child process exists
                    g_processCreationCount.store(0);
                    testClient.Disconnect();
                }

                g_initialized.store(true);
                result(true);
            }
            else {
                result(false);
            }
        }
        catch (const std::exception& ex) {
            write_error(ex, 475);
            write_log(L"[ERROR] ", (L"Exception in Initialize: " + utf8_to_wide(ex.what()) + L" $cout").c_str());
            g_initialized.store(false);
            g_processCreationCount.store(0);
            result(false);
        }
        catch (...) {
            write_error();
            write_log(L"[ERROR] ", L"Unknown exception in Initialize $cout");
            g_initialized.store(false);
            g_processCreationCount.store(0);
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
            PluginSetting settings = gSetting();
            std::string logoutMsg = "logout";
            
            // Try Named Pipe first
            std::wstring pipeName = GetPipeName(utf8_to_wide(settings.title));
            NamedPipeClient client(pipeName);
            
            if (client.Connect()) {
                PipeMessage message(CMD_STOP_SERVICE, logoutMsg);
                if (client.SendMessage(message)) {
                    write_log(L"[STOP] ", L"Sent via Named Pipe");
                    result(true);
                    return;
                } else {
                    write_log(L"[STOP] ", L"Failed to send via Named Pipe");
                }
            } else {
                write_log(L"[STOP] ", L"Failed to connect via Named Pipe");
            }
            
            // Fallback to WM_COPYDATA
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
            write_log(L"[STOP] ", L"Sent via WM_COPYDATA");
            result(true);
        }
        catch (...) {
            write_error();
            result(false);
        }
    }

    PluginSetting LocalPushConnectivityPlugin::_settings{};

    int LocalPushConnectivityPlugin::createBackgroundProcess(int counter) {
        try {
            write_log(L"[DEBUG] ", (L"createBackgroundProcess called with counter: " + std::to_wstring(counter) + L" $cout").c_str());
            auto oldPid = read_pid();
            write_log(L"[DEBUG] ", (L"oldPid: " + std::to_wstring(reinterpret_cast<uintptr_t>(oldPid))).c_str());
            
            if (oldPid && IsWindow(oldPid)) {
                write_log(L"[DEBUG] ", L"Old process still exists, killing it first");
                write_log(L"[Plugin] ", L"Old process still exists, killing it first");
                // Kill old process
                DWORD processId;
                GetWindowThreadProcessId(oldPid, &processId);
                write_log(L"[DEBUG] ", (L"Killing process ID: " + std::to_wstring(processId)).c_str());
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
                if (hProcess) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                    write_log(L"[DEBUG] ", L"Old process terminated");
                    write_log(L"[Plugin] ", L"Old process terminated");
                    // Wait a bit for cleanup
                    Sleep(1000);
                }
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
            std::wstring commandLine = L"\"" + std::wstring(execuablePath) + L"\" child \"" + wideSettings + L"\" $cout:" + std::to_wstring(counter);

            write_log(L"[DEBUG] ", (L"Creating process with command: " + commandLine + L" $cout").c_str());
            std::wcout << "exe service notification: " << commandLine << L"\n";
            if (CreateProcess(NULL, commandLine.data(), NULL, NULL,
                FALSE, 0, NULL, NULL, &si, &pi)) {
                write_log(L"[DEBUG] ", (L"Process created successfully with PID: " + std::to_wstring(pi.dwProcessId) + L" $cout").c_str());
                std::wcout << L"create process ok";

                auto noti_title = utf8_to_wide(settings.title_notification);

                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                return -8;
            }
            else {
                DWORD error = GetLastError();
                write_log(L"[DEBUG] ", (L"CreateProcess failed with error: " + std::to_wstring(error)).c_str());
                std::wcerr << L"create process error" << error << std::endl;
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

    void LocalPushConnectivityPlugin::waitForChildProcessReady() {
        try {
            write_log(L"[DEBUG] ", L"waitForChildProcessReady called");
            // Chờ tối đa 10 giây để child process khởi tạo xong
            const int timeoutMs = 10000;
            const int checkIntervalMs = 100;
            int elapsedMs = 0;
            
            write_log(L"[DEBUG] ", L"Starting to wait for child process...");
            write_log(L"[Plugin] ", L"Starting to wait for child process...");
            
            while (elapsedMs < timeoutMs) {
                write_log(L"[DEBUG] ", (L"Checking for child process via Named Pipe, elapsed: " + std::to_wstring(elapsedMs) + L"ms").c_str());
                // Kiểm tra xem child process đã sẵn sàng chưa qua Named Pipe
                PluginSetting settings = gSetting();
                std::wstring pipeName = GetPipeName(utf8_to_wide(settings.title));
                NamedPipeClient testClient(pipeName);
                
                // Try to connect with timeout to avoid blocking
                bool connected = false;
                for (int i = 0; i < 3; i++) {
                    if (testClient.Connect()) {
                        connected = true;
                        break;
                    }
                    Sleep(200); // Wait 200ms before retry to avoid pipe busy
                }
                
                if (connected) {
                    write_log(L"[DEBUG] ", L"Child process ready via Named Pipe, sending settings $cout");
                    write_log(L"[Plugin] ", L"Child process ready, sending settings");
                    LocalPushConnectivityPlugin::sendSettings();
                    // Reset counter when child process is ready
                    g_processCreationCount.store(0);
                    testClient.Disconnect();
                    return;
                }
                testClient.Disconnect();
                
                // Add delay to avoid continuous connection attempts
                Sleep(500); // Wait 500ms before next attempt
                
                // Thêm logging để debug
                if (elapsedMs % 1000 == 0) { // Log mỗi giây
                    write_log(L"[Plugin] ", L"Still waiting for child process...");
                }
                
                // Chờ một chút rồi kiểm tra lại
                Sleep(checkIntervalMs);
                elapsedMs += checkIntervalMs;
            }
            
            // Nếu timeout, gửi lỗi về app và cleanup
            write_log(L"[Plugin] ", L"Timeout waiting for child process - cleaning up");
            g_creatingProcess.store(false);
            if (_result) {
                _result(false);
                _result = NULL;
            }
        }
        catch (const std::exception& ex) {
            write_error(ex, 500);
            g_creatingProcess.store(false);
            if (_result) {
                _result(false);
                _result = NULL;
            }
        }
        catch (...) {
            write_error();
            g_creatingProcess.store(false);
            if (_result) {
                _result(false);
                _result = NULL;
            }
        }
    }
}  // namespace local_push_connectivity