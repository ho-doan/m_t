import 'package:flutter/material.dart';
import 'dart:async';
import 'flutter_timer_plugin.dart';

class TimerApp extends StatefulWidget {
  const TimerApp({super.key});
  @override
  State<TimerApp> createState() => _TimerAppState();
}

class _TimerAppState extends State<TimerApp> {
  final FlutterTimerPlugin _plugin = FlutterTimerPlugin();
  bool _isRunning = false;
  String _currentTime = '';
  Timer? _timer;

  @override
  void initState() {
    super.initState();
    _initializePlugin();
  }

  Future<void> _initializePlugin() async {
    final success = await _plugin.initialize();
    if (success) {
      print('Plugin initialized successfully');
    } else {
      print('Failed to initialize plugin');
    }
  }

  Future<void> _startTimer() async {
    final success = await _plugin.startTimer();
    if (success) {
      setState(() {
        _isRunning = true;
      });

      // Start polling for current time
      _timer = Timer.periodic(Duration(seconds: 1), (timer) {
        final time = _plugin.getCurrentTime();
        if (time.isNotEmpty) {
          setState(() {
            _currentTime = time;
          });
        }
      });
    }
  }

  Future<void> _stopTimer() async {
    await _plugin.stopTimer();
    _timer?.cancel();
    setState(() {
      _isRunning = false;
      _currentTime = '';
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Flutter Timer Plugin'),
        backgroundColor: Colors.blue,
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              Icons.timer,
              size: 100,
              color: _isRunning ? Colors.green : Colors.grey,
            ),
            SizedBox(height: 20),
            Text(
              'Timer Status:',
              style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
            ),
            SizedBox(height: 10),
            Text(
              _isRunning ? "Running" : "Stopped",
              style: TextStyle(
                fontSize: 28,
                color: _isRunning ? Colors.green : Colors.red,
                fontWeight: FontWeight.bold,
              ),
            ),
            SizedBox(height: 30),
            Text(
              'Current Time:',
              style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
            ),
            SizedBox(height: 10),
            Text(
              _currentTime.isEmpty ? 'No time data' : _currentTime,
              style: TextStyle(
                fontSize: 24,
                color: Colors.blue,
                fontFamily: 'monospace',
              ),
            ),
            SizedBox(height: 40),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              children: [
                ElevatedButton.icon(
                  onPressed: _isRunning ? null : _startTimer,
                  icon: Icon(Icons.play_arrow),
                  label: Text('Start Timer'),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.green,
                    foregroundColor: Colors.white,
                    padding: EdgeInsets.symmetric(horizontal: 20, vertical: 15),
                  ),
                ),
                ElevatedButton.icon(
                  onPressed: _isRunning ? _stopTimer : null,
                  icon: Icon(Icons.stop),
                  label: Text('Stop Timer'),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.red,
                    foregroundColor: Colors.white,
                    padding: EdgeInsets.symmetric(horizontal: 20, vertical: 15),
                  ),
                ),
              ],
            ),
            SizedBox(height: 20),
            if (_isRunning)
              Text(
                'Timer is running in background...',
                style: TextStyle(
                  fontSize: 16,
                  color: Colors.grey[600],
                  fontStyle: FontStyle.italic,
                ),
              ),
          ],
        ),
      ),
    );
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }
}
