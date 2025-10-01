# local_push_connectivity

* [![local_push_connectivity version](https://img.shields.io/pub/v/local_push_connectivity?label=local_push_connectivity)](https://pub.dev/packages/local_push_connectivity)
[![local_push_connectivity size](https://img.shields.io/github/repo-size/ho-doan/local_push_connectivity)](https://github.com/ho-doan/local_push_connectivity)
[![local_push_connectivity issues](https://img.shields.io/github/issues/ho-doan/local_push_connectivity)](https://github.com/ho-doan/local_push_connectivity)
[![local_push_connectivity issues](https://img.shields.io/pub/likes/local_push_connectivity)](https://github.com/ho-doan/local_push_connectivity)

* a local network Apple Local Push Connectivity

## Futures

* initial Local Push Connectivity
* start, stop Local Push Connectivity
* use TCP | Secure TCP | WS | WSS

## Platform Support

|                    | Android | iOS | MacOS | Web | Linux | Windows |
|:------------------:|:-------:|:---:|:-----:|:---:|:-----:|:-------:|
|                    |    ✅    |  ✅  |   ❌   |  ✅  |   ❌   |    ✅    |
| Running Background |    ✅    |  ✅  |   ❌   |  ❌  |   ❌   |    ✅    |

## Getting Started

### Android

#### WS | WSS

`app/src/main/res/xml/network_security_config.xml`

```xml
<?xml version="1.0" encoding="utf-8"?>
<network-security-config>
    <domain-config cleartextTrafficPermitted="true">
        <!-- You can add other domains or IPs as needed -->
        <domain includeSubdomains="true">0.0.0.0</domain>
    </domain-config>
</network-security-config>
```

`AndroidManifest.xml`

```xml
<manifest xmlns:tools="http://schemas.android.com/tools"
    xmlns:android="http://schemas.android.com/apk/res/android">
    <application
        ...
        android:networkSecurityConfig="@xml/network_security_config"
        ...
    >
    </application>
</manifest>

```

### iOS

```swift
// AppDelegate.swift
import local_push_connectivity

@main
@objc class AppDelegate: FlutterAppDelegate {
    override func application(
        _ application: UIApplication,
        didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?
    ) -> Bool {
        // ...
        if let _ = Bundle.main.object(forInfoDictionaryKey: "NEAppPushBundleId") as? String{
            PushConfigurationManager.shared.initialize()
        }
        return super.application(application, didFinishLaunchingWithOptions: launchOptions)
    }
    
    override func applicationDidBecomeActive(_ application: UIApplication) {
        let settingManager = SettingManager(nil)
        var settings = settingManager.fetch()!
        settings.appKilled = false
        try? settingManager.set(settings: settings)
    }
    
    override func applicationDidEnterBackground(_ application: UIApplication) {
        let settingManager = SettingManager(nil)
        var settings = settingManager.fetch()!
        settings.appKilled = true
        try? settingManager.set(settings: settings)
    }
}
```

`Runner.entitlements`

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
 <key>com.apple.developer.networking.networkextension</key>
 <array>
  <string>app-push-provider</string>
 </array>
 <key>com.apple.security.application-groups</key>
 <array>
  <string>group.com.hodoan.localpush</string>
 </array>
</dict>
</plist>

```

`Info.plist`

```plist
<key>GroupNEAppPushLocal</key>
<string>group.com.hodoan.localpush</string>
<key>NEAppPushBundleId</key>
<string>com.hodoan.localpush.provider</string>
<key>NSAppTransportSecurity</key>
<dict>
    <key>NSAllowsArbitraryLoads</key>
    <true/>
</dict>
<key>UIBackgroundModes</key>
<array>
    <string>audio</string>
    <string>voip</string>
</array>
<key>NSLocalNetworkUsageDescription</key>
<string>SimplePush requires access to your local network in order to activate the Local Push Connectivity extension.</string>
```

* add `NetworkExtension` to work when app is killed

[how to add extensions](https://medium.com/@henribredtprivat/create-an-ios-share-extension-with-custom-ui-in-swift-and-swiftui-2023-6cf069dc1209)

```swift
import NetworkExtension
import local_push_connectivity

class FilterControlProvider: NEAppPushProvider, UserSettingsObserverDelegate {
    func userSettingsDidChange(settings: local_push_connectivity.Settings) {
        self.channel?.disconnect()
        self.channel = nil
        
        let channel = ISocManager.register(settings: settings)
        let retryWorkItem = DispatchWorkItem {
            print("Retrying to connect with update...")
            self.channel = channel
            if settings.user.uuid?.isEmpty ?? true {
                channel.disconnect()
            }
            else if !settings.pushManagerSettings.isEmptyInApp {
                channel.connect(settings: settings)
            } else {
                MessageManager.shared.showNotificationError(payload: "error ---- \(settings)")
            }
        }
        let dispatchQueue = DispatchQueue(label: "LocalPushConnectivityPlugin.dispatchQueue")
        dispatchQueue.asyncAfter(deadline: .now() + 6, execute: retryWorkItem)
    }
    
    private let dispatchQueue = DispatchQueue(label: "FilterControlProvider.dispatchQueue")
    private let messageManager = MessageManager.shared
    private var channel: ISocManager? = nil
    
    lazy var settingManager = SettingManager(self)
    
    override func start() {
        guard let _ = providerConfiguration?["host"] as? String else {
            self.messageManager.showNotificationError(payload: "providerConfiguration nill")
            return
        }
        
        if self.channel == nil {
            let settings = settingManager.fetch()!
            self.channel = ISocManager.register(settings: settings)
        } else {
            self.channel?.disconnect()
        }
        
        let retryWorkItem = DispatchWorkItem {
            print("Retrying to connect with update...")
            self.channel?.connect()
        }
        
        let dispatchQueue = DispatchQueue(label: "LocalPushConnectivityPlugin.dispatchQueue")
        dispatchQueue.asyncAfter(deadline: .now() + 6, execute: retryWorkItem)
        
    }
    
    override func stop(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
        self.channel?.disconnect()
        completionHandler()
    }
}
```

`Extension*.plist`

```plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
 <key>GroupNEAppPushLocal</key>
 <string>group.com.hodoan.localpush</string>
 <key>NSExtension</key>
 <dict>
  <key>NSExtensionPointIdentifier</key>
  <string>com.apple.networkextension.app-push</string>
  <key>NSExtensionPrincipalClass</key>
  <string>$(PRODUCT_MODULE_NAME).FilterControlProvider</string>
 </dict>
</dict>
</plist>
```

`Extension*.entitlements`

```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>com.apple.developer.networking.networkextension</key>
 <array>
  <string>app-push-provider</string>
 </array>
 <key>com.apple.security.application-groups</key>
 <array>
  <string>group.com.hodoan.localpush</string>
 </array>
</dict>
</plist>
```

#### **If you encounter an error while running the application, you can check**

![runner_spm](https://github.com/ho-doan/local_push_connectivity/raw/master/images/runner_spm.png)
![runner_build](https://github.com/ho-doan/local_push_connectivity/raw/master/images/runner_build.png)
![runner_build_lib](https://github.com/ho-doan/local_push_connectivity/raw/master/images/runner_build_lib.png)
![runner_sign](https://github.com/ho-doan/local_push_connectivity/raw/master/images/runner_sign.png)
[how to add extension flutter](https://docs.flutter.dev/platform-integration/ios/app-extensions)
[current bug extension flutter](https://github.com/flutter/flutter/issues/142136)
![kit](https://github.com/ho-doan/local_push_connectivity/raw/master/images/kit.png)

### Windows

* [if use WSS | Secure TCP](https://learn.microsoft.com/en-us/skype-sdk/sdn/articles/installing-the-trusted-root-certificate)

`main.cpp`

```cpp
#include "local_push_connectivity/local_push_connectivity_plugin_c_api.h"

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE prev,
                      _In_ wchar_t *command_line, _In_ int show_command)
{
  if (LocalPushConnectivityPluginCApiRegisterProcess(L"local_push_connectivity_example", command_line) == 0)
  {
    return 0;
  }
  // ...
  if (!window.Create(L"local_push_connectivity_example", origin, size))
  {
    return EXIT_FAILURE;
  }
  // ...
}
```

`win32_window.cpp`

```cpp
#include "local_push_connectivity/local_push_connectivity_plugin_c_api.h"

// static
LRESULT CALLBACK Win32Window::WndProc(HWND const window,
                                      UINT const message,
                                      WPARAM const wparam,
                                      LPARAM const lparam) noexcept
{
  LocalPushConnectivityPluginCApiHandleMessage(window, message, lparam);
  // ...
  return DefWindowProc(window, message, wparam, lparam);
}
```

### Usage

#### Initial

```dart
// ho-doan.com = 127.0.0.1
LocalPushConnectivity.instance.initial(
    widows: const WindowsSettings(
        displayName: 'Local Push Sample',
        bundleId: 'com.hodoan.local_push_connectivity_example',
        icon: r'assets\favicon.png',
        iconContent: r'assets\info.svg',
    ),
    android: const AndroidSettings(
        icon: '@mipmap/ic_launcher',
    ),
    ios: const IosSettings(
        ssid: 'HoDoanWifi',
    ),
    web: const WebSettings(),
    // use TCP
    mode: const ConnectModeTCP(
        host: 'ho-doan.com',
        port: 4041,
    ),
    // use WSS | WS
    // mode: const ConnectModeWebSocket(
    //   host: 'ho-doan.com',
    //   port: 4040,
    //   wss: false,
    //   // wss: false,
    //   part: '/ws/',
    // ),
    // use Secure TCP
    //   mode: const ConnectModeTCPSecure(
    //   host: 'ho-doan.com',
    //   cnName: 'SimplePushServer',
    //   dnsName: 'simplepushserver.example',
    //   port: 4042,
    //   publicHasKey: 'XTQSZGrHFDV6KdlHsGVhixmbI/Cm2EMsz2FqE2iZoqU=',
    // ),
      );
```

#### Request permission

```dart
LocalPushConnectivity.instance.requestPermission();
```

#### Stop Local Push

```dart
LocalPushConnectivity.instance.stop();
```

#### Config Device & Start

```dart
LocalPushConnectivity.instance.registerConnectorID(connectorID: 'abc-id-1');
// if Platform ios
LocalPushConnectivity.instance.configSSID('HoDoanWifi');
```

## Example

![android](https://github.com/ho-doan/local_push_connectivity/raw/master/images/ex.android.jpg)
![ios](https://github.com/ho-doan/local_push_connectivity/raw/master/images/ex.ios.png)
![web](https://github.com/ho-doan/local_push_connectivity/raw/master/images/ex.web.png)
![windows](https://github.com/ho-doan/local_push_connectivity/raw/master/images/ex.windows.png)
