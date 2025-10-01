import 'dart:async';
import 'dart:convert';
import 'dart:developer';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

void main() async {
  await runZonedGuarded(
    () async {
      WidgetsFlutterBinding.ensureInitialized();
      // await LocalPushConnectivity.instance.initial(
      //   mode: const ConnectModeWebSocket(
      //     host: 'localhost',
      //     port: 4040,
      //     wss: false,
      //     part: '/ws/',
      //   ),
      // );

      runApp(const MyApp());
    },
    (e, s) {
      log(e.toString(), stackTrace: s);
    },
  );
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Demo',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        useMaterial3: true,
      ),
      home: const MyHomePage(title: 'Server Demo'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});

  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  Map<String, Map<String, WebSocket?>> clients =
      <String, Map<String, WebSocket?>>{};
  Map<String, Map<String, Socket?>> clientsTCP =
      <String, Map<String, Socket?>>{};
  Map<String, Map<String, SecureSocket?>> clientsTCPSecure =
      <String, Map<String, SecureSocket?>>{};
  @override
  void initState() {
    _initial();
    _initialTCP();
    _initialTCPSecure();
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        title: Text(widget.title),
      ),
      body: Center(
        child: ListView(
          children: <Widget>[
            for (final client in clients.entries)
              for (final soc in client.value.entries)
                Row(
                  children: [
                    Expanded(
                      child: Column(
                        children: [
                          Text('Client: ${client.key}'),
                          Text('DeviceId: ${soc.key}'),
                        ],
                      ),
                    ),
                    if (soc.value != null)
                      ElevatedButton(
                        onPressed: () {
                          final mess = {
                            'Notification': {
                              'Title': 'Hello from server',
                              'Body': 'This message send from server',
                            },
                            'Data': {
                              'ID':
                                  'bla ${DateTime.now().millisecondsSinceEpoch}',
                            },
                          };
                          soc.value?.add(jsonEncode(mess));
                        },
                        child: const Text('Send message'),
                      ),
                  ],
                ),
            for (final client in clientsTCP.entries)
              for (final soc in client.value.entries)
                Row(
                  children: [
                    Expanded(
                      child: Column(
                        children: [
                          Text('Client: ${client.key}'),
                          Text('DeviceId: ${soc.key}'),
                        ],
                      ),
                    ),
                    if (soc.value != null)
                      ElevatedButton(
                        onPressed: () {
                          final mess = {
                            'notification': {
                              'title': 'Hello from server',
                              'body': 'This message send from server',
                            },
                            'data': {'ID': 'bla'},
                          };
                          soc.value?.write(jsonEncode(mess));
                        },
                        child: const Text('Send message'),
                      ),
                  ],
                ),
            for (final client in clientsTCPSecure.entries)
              for (final soc in client.value.entries)
                Row(
                  children: [
                    Expanded(
                      child: Column(
                        children: [
                          Text('Client: ${client.key}'),
                          Text('DeviceId: ${soc.key}'),
                        ],
                      ),
                    ),
                    if (soc.value != null)
                      ElevatedButton(
                        onPressed: () {
                          final mess = {
                            'notification': {
                              'title': 'Hello from server',
                              'body': 'This message send from server',
                            },
                            'data': {'ID': 'bla'},
                          };
                          soc.value?.write(jsonEncode(mess));
                        },
                        child: const Text('Send message'),
                      ),
                  ],
                ),
          ],
        ),
      ),
    );
  }

  void _initial() async {
    //#region ssl
    // final certificateData = await rootBundle.load('assets/server.crt');
    // final privateKeyData = await rootBundle.load('assets/server.key');

    // var context = SecurityContext.defaultContext;
    // context.useCertificateChainBytes(certificateData.buffer.asUint8List());
    // context.usePrivateKeyBytes(privateKeyData.buffer.asUint8List());
    //#endregion
    var server = await HttpServer.bind(InternetAddress.anyIPv4, 4040);
    // var server =
    //     await HttpServer.bindSecure(InternetAddress.anyIPv4, 4040, context);
    log(
      'WebSocket server is running on ws://${server.address.host}:${server.port}',
    );
    log('WebSocket server is running on wss://ho-doan.com:${server.port}');

    // Listen for incoming connections.
    await for (var request in server) {
      if (request.uri.path == '/ws/' || request.uri.path == '/ws') {
        var websocket = await WebSocketTransformer.upgrade(request);
        log('WClient connected: ${websocket.hashCode}');

        websocket.listen(
          (message) {
            log('${message.runtimeType} $message');
            if (message is String) {
              try {
                final json = jsonDecode(message);
                if (json['messageType'] == 'ping') {
                  websocket.add(
                    jsonEncode({
                      'pong': '${DateTime.now().millisecondsSinceEpoch}',
                    }),
                  );
                } else {
                  final mess = MessageRegister.fromJson(json);
                  if (mounted) {
                    final socs = clients[mess.sender.connectorID];
                    setState(() {
                      clients = Map.from(clients)..addAll({
                        mess.sender.connectorID: Map<String, WebSocket>.from(
                          socs ?? {},
                        )..addAll({mess.sender.deviceID: websocket}),
                      });
                    });
                  }
                }
              } catch (e, s) {
                log(e.toString(), stackTrace: s);
              }
            } else if (message is Uint8List) {
              try {
                final json = jsonDecode(utf8.decode(message));
                log(json.toString());
                if (json['messageType'] == 'ping') {
                  websocket.add(
                    jsonEncode({
                      'pong': '${DateTime.now().millisecondsSinceEpoch}',
                    }),
                  );
                } else {
                  final mess = MessageRegister.fromJson(
                    jsonDecode(utf8.decode(message)),
                  );
                  if (mounted) {
                    final socs = clients[mess.sender.connectorID];
                    setState(() {
                      clients = Map.from(clients)..addAll({
                        mess.sender.connectorID: Map<String, WebSocket>.from(
                          socs ?? {},
                        )..addAll({mess.sender.deviceID: websocket}),
                      });
                    });
                  }
                }
              } catch (e, s) {
                log(e.toString(), stackTrace: s);
              }
            } else {
              log(message);
            }
          },
          onDone: () {
            log('Client disconnected: ${websocket.hashCode}');
            String? clientId;
            String? deviceId;
            for (final item in clients.entries) {
              for (final sItem in item.value.entries) {
                if (sItem.value.hashCode == websocket.hashCode) {
                  clientId = item.key;
                  deviceId = sItem.key;
                  break;
                }
              }
            }
            if (clientId == null || deviceId == null) return;
            setState(() {
              final socs = Map<String, WebSocket?>.from(
                clients[clientId!] ?? <String, WebSocket?>{},
              )..removeWhere((k, v) => k == deviceId);
              clients =
                  Map.from(clients)
                    ..removeWhere((k, v) => k == clientId)
                    ..addAll({clientId: socs});
            });
          },
          onError: (error) {
            log('Error: $error');
          },
        );
      } else {
        request.response
          ..statusCode = HttpStatus.notFound
          ..write('404 Not Found')
          ..close();
      }
    }
  }

  void _initialTCP() async {
    final server = await ServerSocket.bind(InternetAddress.anyIPv4, 4041);
    log('TCP Server is running on ${server.address.address}:${server.port}');

    await for (var socket in server) {
      log(
        'Connection from: ${socket.remoteAddress.address}:${socket.remotePort}',
      );

      // Handle incoming data
      socket.listen(
        (data) {
          final message = String.fromCharCodes(data);
          log('Received: $message');

          log(message.runtimeType.toString());
          try {
            final json = jsonDecode(message);
            final mess = MessageRegister.fromJson(json);
            if (mounted) {
              final socs = clientsTCP[mess.sender.connectorID];
              setState(() {
                clientsTCP = Map.from(clientsTCP)..addAll({
                  mess.sender.connectorID: Map<String, Socket>.from(socs ?? {})
                    ..addAll({mess.sender.deviceID: socket}),
                });
              });
            }
          } catch (e, s) {
            log(e.toString(), stackTrace: s);
          }
        },
        onDone: () {
          log(
            'Client disconnected: ${socket.remoteAddress.address}:${socket.remotePort}',
          );
          String? clientId;
          String? deviceId;
          for (final item in clientsTCP.entries) {
            for (final sItem in item.value.entries) {
              if (sItem.value.hashCode == socket.hashCode) {
                clientId = item.key;
                deviceId = sItem.key;
                break;
              }
            }
          }
          if (clientId == null || deviceId == null) return;
          setState(() {
            final socs = Map<String, Socket?>.from(
              clientsTCP[clientId!] ?? <String, Socket?>{},
            )..removeWhere((k, v) => k == deviceId);
            clientsTCP =
                Map.from(clientsTCP)
                  ..removeWhere((k, v) => k == clientId)
                  ..addAll({clientId: socs});
          });
          socket.close();
        },
        onError: (e, s) {
          log(
            'Client disconnected: ${socket.remoteAddress.address}:${socket.remotePort}',
          );
          String? clientId;
          String? deviceId;
          for (final item in clientsTCP.entries) {
            for (final sItem in item.value.entries) {
              if (sItem.value.hashCode == socket.hashCode) {
                clientId = item.key;
                deviceId = sItem.key;
                break;
              }
            }
          }
          if (clientId == null || deviceId == null) return;
          setState(() {
            final socs = Map<String, Socket?>.from(
              clientsTCP[clientId!] ?? <String, Socket?>{},
            )..removeWhere((k, v) => k == deviceId);
            clientsTCP =
                Map.from(clientsTCP)
                  ..removeWhere((k, v) => k == clientId)
                  ..addAll({clientId: socs});
          });
          socket.close();
          log(e.toString(), stackTrace: s);
        },
        cancelOnError: false,
      );
    }
  }

  void _initialTCPSecure() async {
    // Load the .p12 file.
    final p12File = await rootBundle.load('assets/SimplePushServer.p12');
    final p12Bytes = p12File.buffer.asUint8List();

    // Decode the .p12 file using SecureSocket.
    final securityContext = SecurityContext(withTrustedRoots: true);
    securityContext.useCertificateChainBytes(p12Bytes, password: 'apple');
    securityContext.usePrivateKeyBytes(p12Bytes, password: 'apple');

    // Create a secure server socket.
    var server = await SecureServerSocket.bind(
      InternetAddress.anyIPv4,
      4042,
      securityContext,
    );

    log('Secure server listening on port 4042');

    server.listen(
      (SecureSocket client) {
        log(
          'Secure connection from ${client.remoteAddress.address}:${client.remotePort}',
        );

        // Handle client communication.
        client.listen(
          (data) {
            final String message = String.fromCharCodes(data);
            log('Received: $message');
            try {
              final json = jsonDecode(message);
              final mess = MessageRegister.fromJson(json);
              if (mounted) {
                final socs = clientsTCPSecure[mess.sender.connectorID];
                setState(() {
                  clientsTCPSecure = Map.from(clientsTCPSecure)..addAll({
                    mess.sender.connectorID: Map<String, SecureSocket>.from(
                      socs ?? {},
                    )..addAll({mess.sender.deviceID: client}),
                  });
                });
              }
            } catch (e, s) {
              log(e.toString(), stackTrace: s);
            }
          },
          onDone: () {
            log('Client disconnected.');
            String? clientId;
            String? deviceId;
            for (final item in clientsTCPSecure.entries) {
              for (final sItem in item.value.entries) {
                if (sItem.value.hashCode == client.hashCode) {
                  clientId = item.key;
                  deviceId = sItem.key;
                  break;
                }
              }
            }
            if (clientId == null || deviceId == null) return;
            setState(() {
              final socs = Map<String, SecureSocket?>.from(
                clientsTCPSecure[clientId!] ?? <String, SecureSocket?>{},
              )..removeWhere((k, v) => k == deviceId);
              clientsTCPSecure =
                  Map.from(clientsTCPSecure)
                    ..removeWhere((k, v) => k == clientId)
                    ..addAll({clientId: socs});
            });
            client.close();
          },
          onError: (e, s) {
            log('Client disconnected.');
            String? clientId;
            String? deviceId;
            for (final item in clientsTCPSecure.entries) {
              for (final sItem in item.value.entries) {
                if (sItem.value.hashCode == client.hashCode) {
                  clientId = item.key;
                  deviceId = sItem.key;
                  break;
                }
              }
            }
            if (clientId == null || deviceId == null) return;
            setState(() {
              final socs = Map<String, SecureSocket?>.from(
                clientsTCPSecure[clientId!] ?? <String, SecureSocket?>{},
              )..removeWhere((k, v) => k == deviceId);
              clientsTCPSecure =
                  Map.from(clientsTCPSecure)
                    ..removeWhere((k, v) => k == clientId)
                    ..addAll({clientId: socs});
            });
            client.close();
            log(e.toString(), stackTrace: s);
          },
        );
      },
      onError: (e, s) {
        log(e.toString(), stackTrace: s);
      },
    );
  }
}

class MessageRegister {
  final String messageType;
  final int systemType;
  final Sender sender;
  final Data? data;

  const MessageRegister({
    required this.messageType,
    required this.systemType,
    required this.sender,
    this.data,
  });
  factory MessageRegister.fromJson(Map<String, dynamic> json) =>
      MessageRegister(
        messageType: json['messageType'],
        systemType: json['systemType'],
        sender: Sender.fromJson(json['sender']),
        data: json['data'] != null ? Data.fromJson(json['data']) : null,
      );
}

class Sender {
  final String connectorID, connectorTag, deviceID;

  const Sender({
    required this.connectorID,
    required this.connectorTag,
    required this.deviceID,
  });
  factory Sender.fromJson(Map<String, dynamic> json) => Sender(
    connectorID: json['connectorID'],
    connectorTag: json['connectorTag'],
    deviceID: json['deviceID'],
  );
  Map<String, dynamic> toJson() => {
    'connectorID': connectorID,
    'connectorTag': connectorTag,
    'deviceID': deviceID,
  };
}

class Data {
  final String? id;
  const Data({this.id});
  factory Data.fromJson(Map<String, dynamic> json) => Data(id: json['id']);
  Map<String, dynamic> toJson() => {'id': id};
}
