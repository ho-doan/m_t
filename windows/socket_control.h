#pragma once
#include "websocket_client.h"
#include <mutex>
#include <iostream>

#include "model.h"
#include "nlohmann/json.hpp"
#include "utils.h"
#include "win_toast.h"

#include <winrt/windows.system.threading.h>

// Command IDs cho WM_COPYDATA
#define CMD_UPDATE_SETTINGS   100
#define CMD_UPDATE_REGISTER   200
#define CMD_STOP_SERVICE      300
#define CMD_RECONNECT         400

using namespace winrt;
using namespace Windows::System::Threading;

using json = nlohmann::json;

struct WebSocketControl
{
    WebSocketClient client;
    std::wstring uri;
    std::wstring registerStr = L"";
    PluginSetting settings;
    bool stopFlag{ false };
    std::mutex lock;
    std::atomic<int64_t> lastPong{ 0 };
    std::atomic<bool> reconnecting{ false };
    
    ThreadPoolTimer heartbeatTimer{ nullptr };

    std::atomic<bool> firstPong{ false };

    ThreadPoolTimer reconnectTimer{ nullptr };
    std::mutex reconnectMutex;

    WebSocketControl() = default;

    WebSocketControl(std::wstring const& url) {
        updateUri(url);
    }

    void stopHeartbeat() {
        if (heartbeatTimer) {
            heartbeatTimer.Cancel();
            heartbeatTimer = nullptr;
        }
    }

    void startHeartbeat() {
        stopHeartbeat();

        lastPong = now();
        heartbeatTimer = ThreadPoolTimer::CreatePeriodicTimer(
            [this](ThreadPoolTimer const& timer) {
                if (stopFlag) {
                    timer.Cancel();
                    return;
                }

                if (client.isConnected()) {
                    json j = PingModel{ "ping" };
                    try {
                        send(utf8_to_wide(j.dump()));
                    }
                    catch (...) {
                        write_log(L"[HEARTBEAT] send failed");
                    }

                    int64_t diff = now() - lastPong.load();
                    if (diff > 15000 * 3) { // >45s không có pong
                        write_log(L"[HEARTBEAT] ", L"no pong → reconnect");
                        client.disconnect();
                        reconnect();
                    }
                }
                else {
                    write_log(L"[HEARTBEAT] ", L"no connection");
                }
            },
            std::chrono::seconds(5) // tick mỗi 5s
        );
    }

    void start()
    {
        stopFlag = false;
        connect();
    }

    void stop()
    {
        std::scoped_lock lk(reconnectMutex);
        write_log(L"[CONNECTION DISCONNECTING] ", L"Cancel by user");
        stopFlag = true;
        stopHeartbeat();
        if (reconnectTimer) {
            reconnectTimer.Cancel();
            reconnectTimer = nullptr;
            write_log(L"[RECONNECT] reset timer (debounce)");
        }

        client.disconnect();
    }

    void _reveive(std::wstring msg) {
        HWND hwnd = FindWindow(NULL, utf8_to_wide(settings.title).c_str());
        DWORD pidApp = 0;
        GetWindowThreadProcessId(hwnd, &pidApp);
        //long long pidAppLong = static_cast<long long>(pidApp);
        //bool appActive = settings.appPid == pidAppLong;
        if (hwnd && IsWindow(hwnd)) { //  && appActive
            COPYDATASTRUCT cds;
            cds.dwData = 1;
            cds.cbData = (DWORD)(wcslen(msg.c_str()) + 1) * sizeof(wchar_t);
            cds.lpData = (PVOID)msg.c_str();
            SendMessageW(hwnd, WM_COPYDATA, (WPARAM)GetCurrentProcessId(), (LPARAM)&cds);
            write_log(L"[App Actived]", msg);
            return;
        }
        if (msg == L"reconnect") {
            write_log(L"[Sen Reconnect Miss] ", uri);
            return;
        }
        MessageResponse p;
        try {
            json j = json::parse(msg);
            p = j.get<MessageResponse>();
        }
        catch (const std::exception& ex) {
            write_log(L"[Received] std::exception: ", winrt::hstring(utf8_to_wide(ex.what())));
            return;
        }
        catch (...) {
            write_log(L"[Received]", L"Unknown parse error");
            return;
        }
        auto notification = p.notification;
        if (!notification) {
            return;
        }
        auto notify = *notification;

        write_log(L"[App UnActived]", msg);

        DesktopNotificationManagerCompat::sendToastProcess(
            utf8_to_wide(settings.appBundle),
            utf8_to_wide(settings.iconContent),
            utf8_to_wide(notify.title),
            utf8_to_wide(notify.body),
            msg
        );
    }

    void connect()
    {
        std::scoped_lock g(lock);
        write_log(L"[CONNECT] ", uri);
        firstPong = false;
        client.connect(uri);
        try {
            client.send(registerStr);
            write_log(L"[register]", L"success");
        }
        catch (...) {
            write_log(L"[register]", L"failure");
        }

        client.onMessage = [this](std::wstring msg)
            {
                write_log(L"contrl received", msg);
                PongModel p = {};
                try {
                    json j = json::parse(msg);
                    p = j.get<PongModel>();
                }
                catch (const std::exception& ex) {
                    write_log(L"[onMessage] parse error", winrt::hstring(utf8_to_wide(ex.what())));
                }
                catch (...) {
                    write_log(L"[onMessage] parse error: unknown");
                }
                if (p.pong)
                {
                    if (!firstPong.exchange(true)) {
                        write_log(L"[HEARTBEAT] first pong received\n");
                        write_log(L"[HEARTBEAT] send reconnect\n");
                        _reveive(L"reconnect");
                    }
                    lastPong = now();
                    write_log( L"[HEARTBEAT] pong received\n");
                }
                else
                {
                    write_log(L"[RECV] ", msg);
                    _reveive(msg);
                }
            };

        client.onClosed = [this](uint16_t code, std::wstring reason)
            {
                std::wstringstream err;
                err << code << L" reason=" << reason;
                write_log(L"[CLOSED] code=", winrt::hstring(err.str()));
                // TODO(hodoan): handle this
                reconnect();
            };
        write_log(L"[CONNECTION CONNECTED] ", uri);

        startHeartbeat();
    }

    void disconnect()
    {
        std::scoped_lock g(lock);
        client.disconnect();
    }

    void reconnect()
    {
        if (stopFlag) return;

        if (reconnectTimer) {
            reconnectTimer.Cancel();
            reconnectTimer = nullptr;
            write_log(L"[RECONNECT] reset timer (debounce)");
        }

        stopHeartbeat();

        bool expected = false;
        if (!reconnecting.compare_exchange_strong(expected, true)) {
            write_log(L"[RECONNECT] reconnecting...");
            return;
        }

        // Lên lịch reconnect sau 3 giây
        reconnectTimer = winrt::Windows::System::Threading::ThreadPoolTimer::CreateTimer(
            [this](winrt::Windows::System::Threading::ThreadPoolTimer const& timer)
            {
                std::scoped_lock lk2(reconnectMutex);
                reconnectTimer = nullptr; // clear sau khi chạy

                if (stopFlag) {
                    write_log(L"[RECONNECT] canceled (stopFlag)");
                    return;
                }

                write_log(L"[RECONNECT] executing...");
                stopHeartbeat();
                connect();
            },
            std::chrono::seconds(3)
        );

        write_log(L"[RECONNECT] timer set 3s...");
    }

    void updateSettings(PluginSetting _settings) {
        std::scoped_lock g(lock);
        auto oldregisterStr = registerStr;
        auto oldUri= uri;
        json j = settings;
        json j2 = _settings;
        std::stringstream ss;
        ss << "oldSetting: " << j.dump() << "\nnewSetting: " << j2.dump() << "\n";
        auto sss = ss.str();
        write_log(L"[Change Settings] ", winrt::hstring(utf8_to_wide(sss)));
        settings = _settings;
        if (uri != oldUri && registerStr != oldregisterStr) {
            updateUri(settings.uri());
        }
    }

    void send(std::wstring const& msg)
    {
        client.send(msg);
    }

private:
    void updateUri(std::wstring const& newUri)
    {
        std::scoped_lock g(lock);
        write_log(L"[UPDATE URI] ", winrt::hstring(newUri));
        uri = newUri;
        client.disconnect();
        // TODO(hodoan): handle thís
        reconnect();
    }
    static int64_t now()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};

inline std::unique_ptr<WebSocketControl> m_control;

inline LRESULT CALLBACK CtlHiddenWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COPYDATA: {
        auto* pCDS = reinterpret_cast<PCOPYDATASTRUCT>(lParam);
        if (!pCDS) return FALSE;

        try {
            switch (pCDS->dwData) {
            case CMD_UPDATE_SETTINGS: {
                std::wstring receiver((wchar_t*)pCDS->lpData);
                write_log(L"[Service] receive settings: ", receiver);

                auto settings = pluginSettingsFromJson(wide_to_utf8(receiver));
                if (m_control) {
                    m_control->updateSettings(settings);
                }
                else {
                    write_log(L"[Service] control is null!", L"");
                }
                break;
            }
            case CMD_STOP_SERVICE: {
                write_log(L"[Service] Stop command received", L"");
                if (m_control) m_control->stop();
                break;
            }
            case CMD_RECONNECT: {
                write_log(L"[Service] Reconnect command", L"");
                if (m_control) m_control->reconnect();
                break;
            }
            default:
                write_log(L"[Service] Unknown command", winrt::hstring(std::to_wstring(pCDS->dwData)));
                break;
            }
        }
        catch (const std::exception& ex) {
            write_error(ex, 166);
            write_log(L"[Service] receive error: ", winrt::hstring(utf8_to_wide(ex.what())));
        }
        catch (...) {
            write_error();
            write_log(L"[Service] receive error: ", L"unknown");
        }
        return TRUE;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}