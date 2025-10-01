#ifndef FLUTTER_PLUGIN_LOCAL_PUSH_CONNECTIVITY_PLUGIN_C_API_H_
#define FLUTTER_PLUGIN_LOCAL_PUSH_CONNECTIVITY_PLUGIN_C_API_H_

#include <flutter_plugin_registrar.h>
#include <Windows.h>
#include <string>

#ifdef FLUTTER_PLUGIN_IMPL
#define FLUTTER_PLUGIN_EXPORT __declspec(dllexport)
#else
#define FLUTTER_PLUGIN_EXPORT __declspec(dllimport)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

FLUTTER_PLUGIN_EXPORT void LocalPushConnectivityPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar);
FLUTTER_PLUGIN_EXPORT int LocalPushConnectivityRegisterProcess(
    std::wstring title, wchar_t *command_line);
FLUTTER_PLUGIN_EXPORT void LocalPushConnectivityPluginHandleMessage(
    HWND const window, UINT const message, LPARAM const lparam);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // FLUTTER_PLUGIN_LOCAL_PUSH_CONNECTIVITY_PLUGIN_C_API_H_
