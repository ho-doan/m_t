#include "include/local_push_connectivity/local_push_connectivity_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "local_push_connectivity_plugin.h"

void LocalPushConnectivityPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  local_push_connectivity::LocalPushConnectivityPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}

int LocalPushConnectivityRegisterProcess(
    std::wstring title, wchar_t* command_line) {
    return local_push_connectivity::LocalPushConnectivityPlugin::RegisterProcess(title, command_line);
}
void LocalPushConnectivityPluginHandleMessage(
    HWND const window, UINT const message, LPARAM const lparam) {
    return local_push_connectivity::LocalPushConnectivityPlugin::HandleMessage(window, message, lparam);
}