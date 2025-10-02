import 'package:flutter/material.dart';
import 'dart:async';
import 'flutter_ipc_plugin.dart';

class SimpleIPCTest extends StatefulWidget {
  const SimpleIPCTest({super.key});
  @override
  State<SimpleIPCTest> createState() => _SimpleIPCTestState();
}

class _SimpleIPCTestState extends State<SimpleIPCTest> {
  final FlutterIPCPlugin _plugin = FlutterIPCPlugin();
  bool _isRunning = false;
  String _status = 'Not started';

  @override
  void initState() {
    super.initState();
    _initializePlugin();
  }

  Future<void> _initializePlugin() async {
    print('Initializing IPC Plugin...');
    final success = await _plugin.initialize();
    print('IPC Plugin initialization result: $success');
    if (success) {
      setState(() {
        _status = 'Initialized';
      });
    } else {
      setState(() {
        _status = 'Failed to initialize';
      });
    }
  }

  Future<void> _startIPC() async {
    print('Starting IPC system...');
    setState(() {
      _status = 'Starting...';
    });

    final success = await _plugin.startIPC();
    print('IPC start result: $success');

    if (success) {
      setState(() {
        _isRunning = true;
        _status = 'Running';
      });
      print('IPC system started successfully');
    } else {
      setState(() {
        _status = 'Failed to start';
      });
      print('Failed to start IPC system');
    }
  }

  Future<void> _stopIPC() async {
    print('Stopping IPC system...');
    await _plugin.stopIPC();
    setState(() {
      _isRunning = false;
      _status = 'Stopped';
    });
    print('IPC system stopped');
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Simple IPC Test'),
        backgroundColor: Colors.purple,
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Text(
              'IPC Status: $_status',
              style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
            ),
            SizedBox(height: 20),
            Text('Running: $_isRunning', style: TextStyle(fontSize: 18)),
            SizedBox(height: 40),
            ElevatedButton(
              onPressed: _isRunning ? null : _startIPC,
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.purple,
                foregroundColor: Colors.white,
                padding: EdgeInsets.symmetric(horizontal: 30, vertical: 15),
              ),
              child: Text('Start IPC'),
            ),
            SizedBox(height: 20),
            ElevatedButton(
              onPressed: _isRunning ? _stopIPC : null,
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.red,
                foregroundColor: Colors.white,
                padding: EdgeInsets.symmetric(horizontal: 30, vertical: 15),
              ),
              child: Text('Stop IPC'),
            ),
            SizedBox(height: 20),
            ElevatedButton(
              onPressed: () {
                print('Debug Status:');
                print('  _isRunning: $_isRunning');
                print('  _status: $_status');
              },
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.orange,
                foregroundColor: Colors.white,
                padding: EdgeInsets.symmetric(horizontal: 30, vertical: 15),
              ),
              child: Text('Debug'),
            ),
          ],
        ),
      ),
    );
  }
}
