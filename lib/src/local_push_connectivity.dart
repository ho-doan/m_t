import 'dart:async';
import 'dart:io';

import 'package:flutter/widgets.dart';

import 'messages.g.dart';

extension _VStream<T> on ValueNotifier<T> {
  Stream<T> asStream() async* {
    yield value;
    final controller = StreamController<T>();
    void listener() => controller.add(value);
    addListener(listener);
    yield* controller.stream;
    controller.onCancel = () => removeListener(listener);
  }
}

class LocalPushConnectivity extends LocalPushConnectivityPigeonHostApi
    implements LocalPushConnectivityPigeonFlutterApi {
  LocalPushConnectivity._()
    : super(binaryMessenger: null, messageChannelSuffix: '') {
    LocalPushConnectivityPigeonFlutterApi.setUp(this);
    flutterApiReady();
    message = _notifier.asStream().where((e) => e != null).map((e) {
      if (Platform.isWindows) {
        return MessageSystemPigeon(
          fromNotification: e!.fromNotification,
          mrp: MessageResponsePigeon(
            notification: e.mrp.notification,
            mPayload: e.mrp.mPayload.replaceFirst(RegExp(r'\[\]$'), ''),
          ),
        );
      }
      return e!;
    });
  }

  static final LocalPushConnectivity instance = LocalPushConnectivity._();

  final _notifier = ValueNotifier<MessageSystemPigeon?>(null);

  late final Stream<MessageSystemPigeon> message;

  @override
  Future<void> onMessage(MessageSystemPigeon message) async {
    _notifier.value = message;
    return;
  }

  @override
  Future<bool> stop() async {
    if (Platform.isWindows) {
      return await super.stop();
    }
    return false;
  }

  void dispose() {}
}
