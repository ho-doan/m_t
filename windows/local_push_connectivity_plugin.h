#ifndef FLUTTER_PLUGIN_LOCAL_PUSH_CONNECTIVITY_PLUGIN_H_
#define FLUTTER_PLUGIN_LOCAL_PUSH_CONNECTIVITY_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include "messages.g.h"

#include <memory>

#include "nlohmann/json.hpp"

#include "model.h"
#include "socket_control.h"

using json = nlohmann::json;

namespace local_push_connectivity {
    class LocalPushConnectivityPlugin : public flutter::Plugin, public LocalPushConnectivityPigeonHostApi {
    public:
        static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

        LocalPushConnectivityPlugin();

        virtual ~LocalPushConnectivityPlugin();

        static PluginSetting _settings;

        static int RegisterProcess(std::wstring title, _In_ wchar_t* command_line);
        static void HandleMessage(HWND window, UINT const message,
            LPARAM const lparam);

        void FlutterApiReady(std::function<void(std::optional<FlutterError> reply)> result) override;

        void Initialize(
            int64_t system_type,
            const AndroidSettingsPigeon* android,
            const WindowsSettingsPigeon* windows,
            const IosSettingsPigeon* ios,
            const TCPModePigeon& mode,
            std::function<void(ErrorOr<bool> reply)> result) override;
        void Config(
            const TCPModePigeon& mode,
            const flutter::EncodableList* ssids,
            std::function<void(ErrorOr<bool> reply)> result) override;
        void RequestPermission(std::function<void(ErrorOr<bool> reply)> result) override;
        void Start(std::function<void(ErrorOr<bool> reply)> result) override;
        void Stop(std::function<void(ErrorOr<bool> reply)> result) override;
        void RegisterUser(
            const UserPigeon& user,
            std::function<void(ErrorOr<bool> reply)> result) override;
        void DeviceID(std::function<void(ErrorOr<std::string> reply)> result) override;
    private:
        static bool _flutterApiReady;
        static std::optional<MessageSystemPigeon> _m;
        static std::wstring _title;
        static std::wstring _title_notification;
        static std::wstring  title() { return _title; }
        static std::wstring  title_notification() { return _title_notification; }
        static PluginSetting gSetting() { return _settings; }
        static void set_title(std::wstring title) {
            _title = title;
        }
        static void set_title_notification(std::wstring title) {
            _title_notification = title;
        }
        static void saveSetting(PluginSetting settings) {
            _settings = settings;
        }
        static std::unique_ptr<LocalPushConnectivityPigeonFlutterApi> _flutterApi;
        static void startThreadInChildProcess();
        static void sendSettings();
        static int createBackgroundProcess();
        static void sendMessageFromNoti(MessageSystemPigeon m);
        static void waitForChildProcessReady();
    };
}  // namespace local_push_connectivity

#endif  // FLUTTER_PLUGIN_LOCAL_PUSH_CONNECTIVITY_PLUGIN_H_
