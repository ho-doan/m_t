import 'dart:async';
import 'dart:convert';
import 'dart:developer';
import 'dart:js_interop';
import 'dart:math' as math;
import 'package:flutter/foundation.dart';
import 'package:web/web.dart' as web;

import 'package:flutter/services.dart';
import 'package:web_socket_channel/web_socket_channel.dart';

import 'messages.g.dart';
import 'model.dart';

const _timeDebug = 3;
const _time = 20;

const _durationHeartbeat = Duration(seconds: kDebugMode ? _timeDebug : _time);
const _durationHeartbeatCheck = kDebugMode ? _timeDebug * 2 : _time * 2;

class LocalPushConnectivity
    implements
        LocalPushConnectivityPigeonHostApi,
        LocalPushConnectivityPigeonFlutterApi {
  LocalPushConnectivity._() {
    LocalPushConnectivityPigeonFlutterApi.setUp(this);
    _controller = StreamController<MessageSystemPigeon>.broadcast();
    message = _controller.stream;
    _init();
  }

  late final StreamController<MessageSystemPigeon> _controller;

  static final LocalPushConnectivity instance = LocalPushConnectivity._();

  late final Stream<MessageSystemPigeon> message;

  PluginSettingsPigeon pluginSettings = PluginSettingsPigeon();

  bool _isFocus = false;

  @override
  Future<void> onMessage(MessageSystemPigeon message) async {
    _controller.add(message);
    return;
  }

  void dispose() {
    _controller.close();
  }

  void _init() {
    final isFocus = web.document.hasFocus();
    _isFocus = isFocus;
    web.window.addEventListener(
      'focus',
      ((web.Event _) {
        _isFocus = true;
        log('Window focused');
      }).toJS,
    );
    web.window.addEventListener(
      'blur',
      ((web.Event _) {
        _isFocus = false;
        log('Window blurred');
      }).toJS,
    );
  }

  @override
  Future<bool> config(TCPModePigeon mode, [List<String>? ssids]) async {
    pluginSettings.host = mode.host;
    pluginSettings.publicKey = mode.publicHasKey;
    pluginSettings.port = mode.port;
    pluginSettings.wsPath = mode.path;
    pluginSettings.wss = mode.connectionType == ConnectionType.wss;
    pluginSettings.useTcp = mode.connectionType == ConnectionType.tcp;
    pluginSettings.deviceId = getDeviceID();
    await start();
    return true;
  }

  @override
  Future<bool> initialize({
    required int systemType,
    AndroidSettingsPigeon? android,
    WindowsSettingsPigeon? windows,
    IosSettingsPigeon? ios,
    required TCPModePigeon mode,
  }) async {
    pluginSettings.host = mode.host;
    pluginSettings.publicKey = mode.publicHasKey;
    pluginSettings.port = mode.port;
    pluginSettings.wsPath = mode.path;
    pluginSettings.wss = mode.connectionType == ConnectionType.wss;
    pluginSettings.useTcp = mode.connectionType == ConnectionType.tcp;
    pluginSettings.systemType = systemType;
    pluginSettings.deviceId = getDeviceID();
    return true;
  }

  @override
  // ignore: non_constant_identifier_names
  BinaryMessenger? get pigeonVar_binaryMessenger => null;

  @override
  // ignore: non_constant_identifier_names
  String get pigeonVar_messageChannelSuffix => '';

  @override
  Future<bool> requestPermission() async {
    final permission = await web.Notification.requestPermission().toDart;
    if (permission.toDart == 'granted') {
      log('Permission granted');
      return true;
    } else {
      log('Permission denied');
      return false;
    }
  }

  WebSocketChannel? _webSocketChannel;
  StreamSubscription<dynamic>? _webSocketChannelSubscription;

  Timer? _heartbeatTimer;
  DateTime? _lastHeartbeatTime;

  void _closeHeartbeatTimer() {
    _heartbeatTimer?.cancel();
    _heartbeatTimer = null;
  }

  void _startHeartbeatTimer() {
    _closeHeartbeatTimer();
    _heartbeatTimer = Timer.periodic(_durationHeartbeat, (timer) {
      _webSocketChannel?.sink.add(jsonEncode(PingMessage()));
      final now = DateTime.now();
      if (now.difference(_lastHeartbeatTime ?? DateTime.now()).inSeconds >
          _durationHeartbeatCheck) {
        log('Heartbeat timed out');
        _reconnect();
      }
    });
  }

  @override
  Future<bool> start() async {
    await stop();
    _closeConnection = false;
    if (pluginSettings.connectorID == null) {
      return false;
    }
    try {
      log('Starting WebSocket connection');
      _webSocketChannel = WebSocketChannel.connect(
        Uri.parse(
          'ws://${pluginSettings.host}:${pluginSettings.port}${pluginSettings.wsPath}',
        ),
      );
      await _webSocketChannel?.ready;
      _webSocketChannel?.sink.add(
        jsonEncode(
          RegisterMessage(
            messageType: 'register',
            systemType: pluginSettings.systemType ?? 0,
            sender: Sender(
              connectorID: pluginSettings.connectorID ?? '',
              connectorTag: pluginSettings.connectorTag ?? '',
              deviceID: pluginSettings.deviceId ?? '',
            ),
            data: Data(),
          ).toJson(),
        ),
      );
      _startHeartbeatTimer();
      onMessage(
        MessageSystemPigeon(
          fromNotification: false,
          mrp: MessageResponsePigeon(
            notification: NotificationPigeon(
              title: 'Reconnect',
              body: 'Reconnect',
            ),
            mPayload: 'reconnect',
          ),
        ),
      );

      _webSocketChannelSubscription = _webSocketChannel?.stream.listen(
        (event) {
          PongMessage? pong;
          try {
            final pong_ = PongMessage.fromJson(jsonDecode(event.toString()));
            // final ping_ = PingMessage.fromJson(jsonDecode(event.toString()));
            // if (ping_.messageType == 'ping') return;

            if (pong_.pong != null) {
              pong = pong_;
              _lastHeartbeatTime = DateTime.now();
            }
          } catch (e) {
            log('WebSocket connection error: $e');
          }
          if (pong != null) {
            _lastHeartbeatTime = DateTime.now();
            return;
          }
          final message = MessageResponse.fromString(event.toString());
          if (!_isFocus) {
            _sendNotification(
              MessageResponse(
                notification: message.notification,
                mPayload: event.toString(),
              ),
              onNotificationClick: (value) {
                _controller.add(
                  MessageSystemPigeon(
                    fromNotification: true,
                    mrp: MessageResponsePigeon(
                      notification: NotificationPigeon(
                        title: message.notification.title,
                        body: message.notification.body,
                      ),
                      mPayload: value,
                    ),
                  ),
                );
              },
            );
          }
          _controller.add(
            MessageSystemPigeon(
              fromNotification: false,
              mrp: MessageResponsePigeon(
                notification: NotificationPigeon(
                  title: message.notification.title,
                  body: message.notification.body,
                ),
                mPayload: event.toString(),
              ),
            ),
          );
        },
        onDone: () {
          log('WebSocket connection closed');
          if (!_closeConnection) {
            _reconnect();
          }
        },
        onError: (error) {
          log('WebSocket connection error: $error');
          if (!_closeConnection) {
            _reconnect();
          }
        },
        cancelOnError: true,
      );
      return true;
    } catch (e) {
      log('WebSocket connection error: $e');
      _reconnect();
      return false;
    }
  }

  void _reconnect() {
    Future.delayed(Duration(seconds: 5), () {
      start();
    });
  }

  bool _closeConnection = false;

  @override
  Future<bool> stop() async {
    _closeConnection = true;
    _webSocketChannel?.sink.close();
    _webSocketChannelSubscription?.cancel();
    _webSocketChannelSubscription = null;
    _closeHeartbeatTimer();
    _webSocketChannel = null;
    return true;
  }

  bool _sendNotification(
    MessageResponse message, {
    ValueChanged<String>? onNotificationClick,
  }) {
    if (message.notification.title == '') return false;

    var notification = web.Notification(
      message.notification.title,
      web.NotificationOptions(body: message.notification.body),
    );

    notification.onclick =
        (web.Event event) {
          event.preventDefault();
          web.window.focus();
          notification.close();
          onNotificationClick?.call(message.mPayload ?? '');
        }.toJS;

    return true;
  }

  // Function to generate a UUID (v4)
  String generateUUID() {
    final random = math.Random();
    return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replaceAllMapped(
      RegExp(r'x|y'),
      (match) {
        final r = random.nextInt(16);
        final v = match[0] == 'x' ? r : (r & 0x3 | 0x8);
        return v.toRadixString(16);
      },
    );
  }

  /// Function to set a cookie
  void setCookie(String name, String value, int days) {
    DateTime now = DateTime.now();
    DateTime expiryDate = now.add(Duration(days: days));
    String expires = 'expires=${expiryDate.toUtc().toIso8601String()}';
    web.document.cookie = '$name=$value; $expires; path=/';
  }

  /// Function to retrieve a specific cookie
  String? getCookie(String name) {
    String nameEQ = '$name=';
    List<String> cookies = web.document.cookie.split(';');
    for (var cookie in cookies) {
      String trimmedCookie = cookie.trim();
      if (trimmedCookie.startsWith(nameEQ)) {
        return trimmedCookie.substring(nameEQ.length);
      }
    }
    return null;
  }

  /// Main function to get or create a device ID
  String getDeviceID() {
    String? deviceId = getCookie("deviceId");
    if (deviceId == null) {
      deviceId = generateUUID(); // Generate a UUID
      setCookie("deviceId", deviceId, 365); // Cookie expires in 365 days
    }
    return deviceId;
  }

  @override
  Future<bool> registerUser(UserPigeon user) async {
    pluginSettings.connectorID = user.connectorID;
    pluginSettings.connectorTag = user.connectorTag;

    await start();
    return true;
  }

  @override
  Future<String> deviceID() async {
    return getDeviceID();
  }

  @override
  Future<void> flutterApiReady() async {}

  //#endregion
}
