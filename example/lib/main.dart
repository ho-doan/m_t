import 'dart:developer';
import 'dart:io';

import 'package:flutter/material.dart';
import 'dart:async';

import 'package:local_push_connectivity/local_push_connectivity.dart';

class MyHttpOverrides extends HttpOverrides {
  @override
  HttpClient createHttpClient(SecurityContext? context) {
    return super.createHttpClient(context)
      ..badCertificateCallback =
          (X509Certificate cert, String host, int port) => true;
  }
}

void main() async {
  HttpOverrides.global = MyHttpOverrides();
  runZonedGuarded(
    () async {
      WidgetsFlutterBinding.ensureInitialized();
      await LocalPushConnectivity.instance.requestPermission();
      await LocalPushConnectivity.instance.initialize(
        systemType: 0,
        windows: WindowsSettingsPigeon(
          displayName: 'Local Push Sample',
          bundleId: 'com.hodoan.local_push_connectivity_example',
          icon: r'assets\favicon.png',
          iconContent: r'assets\info.svg',
        ),
        android: AndroidSettingsPigeon(
          icon: '@mipmap/ic_launcher',
          channelNotification:
              'com.hodoan.local_push_connectivity_example.notification',
        ),
        ios: IosSettingsPigeon(ssids: ['TPSSmartoffice']),
        mode: TCPModePigeon(
          // host: 'ho-doan.com',
          host: '10.8.0.2',
          port: 4040,
          connectionType: ConnectionType.ws,
          // wss: false,
          path: '/ws/',
        ),
        // mode: const ConnectModeTCP(
        //   host: '10.50.80.172',
        //   port: 4041,
        // ),
        // systemType: 77,
      );
      runApp(const MyApp());
    },
    (e, s) {
      print(e);
      print(s);
      log(e.toString(), stackTrace: s);
    },
  );
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  String messages = '';
  @override
  void initState() {
    super.initState();
    initPlatformState();
  }

  StreamSubscription<MessageSystemPigeon>? _onMessage;

  @override
  void dispose() {
    _onMessage?.cancel();
    super.dispose();
  }

  // Platform messages are asynchronous, so we initialize in an async method.
  Future<void> initPlatformState() async {
    // Platform messages may fail, so we use a try/catch PlatformException.
    // We also handle the message potentially returning null.
    try {
      _onMessage = LocalPushConnectivity.instance.message.listen((event) {
        setState(() {
          messages = event.mrp.mPayload;
        });
      });
      await LocalPushConnectivity.instance.config(
        TCPModePigeon(
          host: '10.8.0.2',
          port: 4040,
          connectionType: ConnectionType.ws,
          path: '/ws/',
        ),
      );
      await LocalPushConnectivity.instance.registerUser(
        UserPigeon(connectorID: '1234567890', connectorTag: 'test'),
      );
    } catch (e, s) {
      print(e);
      print(s);
      log(e.toString(), stackTrace: s);
    }
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(title: const Text('Plugin example app')),
        body: Center(child: Text('Running: $messages')),
        floatingActionButton: FloatingActionButton(
          onPressed: () {
            if (Platform.isWindows) {
              LocalPushConnectivity.instance.config(
                TCPModePigeon(
                  host: '10.8.0.2',
                  port: 4040,
                  connectionType: ConnectionType.ws,
                  path: '/ws/',
                ),
              );
            }
            // LocalPushConnectivity.instance.stop();
          },
          child: const Icon(Icons.send),
        ),
      ),
    );
  }
}
