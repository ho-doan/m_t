import 'dart:developer';
import 'dart:io';

import 'package:flutter/material.dart';
import 'dart:async';

import 'package:local_push_connectivity/local_push_connectivity.dart';
import 'timer_app.dart';

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
          host: '10.8.0.2',
          port: 4040,
          connectionType: ConnectionType.ws,
          path: '/ws/',
        ),
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
  int _selectedIndex = 0;

  // Pages
  final List<Widget> _pages = [const MainPage(), TimerApp()];

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
        appBar: AppBar(
          title: const Text('Plugin example app'),
          backgroundColor: Colors.blue,
        ),
        body: _pages[_selectedIndex],
        bottomNavigationBar: BottomNavigationBar(
          currentIndex: _selectedIndex,
          onTap: (index) {
            setState(() {
              _selectedIndex = index;
            });
          },
          items: const [
            BottomNavigationBarItem(icon: Icon(Icons.home), label: 'Main'),
            BottomNavigationBarItem(icon: Icon(Icons.timer), label: 'Timer'),
          ],
        ),
      ),
    );
  }
}

class MainPage extends StatelessWidget {
  const MainPage({super.key});

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(Icons.home, size: 100, color: Colors.blue),
          SizedBox(height: 20),
          Text(
            'Local Push Connectivity',
            style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
          ),
          SizedBox(height: 10),
          Text(
            'Main Page',
            style: TextStyle(fontSize: 18, color: Colors.grey[600]),
          ),
          SizedBox(height: 30),
          ElevatedButton.icon(
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
            },
            icon: Icon(Icons.send),
            label: Text('Send Config'),
            style: ElevatedButton.styleFrom(
              backgroundColor: Colors.blue,
              foregroundColor: Colors.white,
              padding: EdgeInsets.symmetric(horizontal: 20, vertical: 15),
            ),
          ),
        ],
      ),
    );
  }
}
