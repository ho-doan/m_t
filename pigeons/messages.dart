import 'package:pigeon/pigeon.dart';

@ConfigurePigeon(
  PigeonOptions(
    dartPackageName: 'local_push_connectivity',
    dartOut: 'lib/src/messages.g.dart',
    swiftOut:
        'ios/local_push_connectivity/Sources/local_push_connectivity/Messages.g.swift',
    swiftOptions: SwiftOptions(),
    kotlinOut:
        'android/src/main/kotlin/com/hodoan/local_push_connectivity/Messages.g.kt',
    kotlinOptions: KotlinOptions(package: 'com.hodoan.local_push_connectivity'),
    cppHeaderOut: 'windows/messages.g.h',
    cppOptions: CppOptions(namespace: 'local_push_connectivity'),
    cppSourceOut: 'windows/messages.g.cpp',
  ),
)
enum ConnectionType { tcp, tcpTls, ws, wss }

class TCPModePigeon {
  String host;
  int port;
  ConnectionType connectionType;

  /// Path for ws and wss
  String? path;

  /// for tcpTls
  String? publicHasKey;

  /// for tcpTls & Platform windows
  String? cnName;

  /// for tcpTls & Platform windows
  String? dnsName;

  TCPModePigeon({
    required this.host,
    required this.port,
    required this.connectionType,
    this.path,
    this.publicHasKey,
    this.cnName,
    this.dnsName,
  });
}

class AndroidSettingsPigeon {
  String icon;
  String channelNotification;

  AndroidSettingsPigeon({
    required this.icon,
    required this.channelNotification,
  });
}

class WindowsSettingsPigeon {
  String displayName;
  String bundleId;
  String icon;
  String iconContent;

  WindowsSettingsPigeon({
    required this.displayName,
    required this.bundleId,
    required this.icon,
    required this.iconContent,
  });
}

class IosSettingsPigeon {
  List<String>? ssids;

  IosSettingsPigeon({this.ssids});
}

class UserPigeon {
  String connectorID;
  String connectorTag;

  UserPigeon({required this.connectorID, required this.connectorTag});
}

@HostApi()
abstract class LocalPushConnectivityPigeonHostApi {
  @async
  bool initialize({
    required int systemType,
    AndroidSettingsPigeon? android,
    WindowsSettingsPigeon? windows,
    // WebSettingsPigeon? web,
    IosSettingsPigeon? ios,
    required TCPModePigeon mode,
  });
  @async
  void flutterApiReady();
  @async
  bool config(TCPModePigeon mode, [List<String>? ssids]);
  @async
  bool registerUser(UserPigeon user);
  @async
  String deviceID();
  @async
  bool requestPermission();
  @async
  bool start();
  @async
  bool stop();
}

class NotificationPigeon {
  String title;
  String body;

  NotificationPigeon({required this.title, required this.body});
}

class MessageResponsePigeon {
  NotificationPigeon notification;
  String mPayload;

  MessageResponsePigeon({required this.mPayload, required this.notification});
}

class MessageSystemPigeon {
  bool fromNotification;
  MessageResponsePigeon mrp;

  MessageSystemPigeon({required this.fromNotification, required this.mrp});
}

class RegisterMessagePigeon {
  String messageType;
  String sendConnectorID;
  String sendDeviceId;
  int systemType;

  RegisterMessagePigeon({
    required this.messageType,
    required this.sendConnectorID,
    required this.systemType,
    required this.sendDeviceId,
  });
}

class PluginSettingsPigeon {
  String? host;
  String? deviceId;
  String? connectorID;
  int? systemType;
  String? iconNotification;
  int? port;
  String? channelNotification;

  bool? wss;
  //default for ws path
  String? wsPath;
  bool? useTcp;
  String? publicKey;
  String? connectorTag;

  PluginSettingsPigeon({
    required this.host,
    required this.deviceId,
    required this.connectorID,
    required this.systemType,
    required this.iconNotification,
    required this.port,
    required this.channelNotification,
    required this.wss,
    required this.wsPath,
    required this.useTcp,
    required this.publicKey,
  });
}

@FlutterApi()
abstract class LocalPushConnectivityPigeonFlutterApi {
  @async
  void onMessage(MessageSystemPigeon mrp);
}
