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
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [Colors.blue[50]!, Colors.white],
          ),
        ),
        child: Center(
          child: Padding(
            padding: EdgeInsets.all(20),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                // Timer Icon
                Container(
                  width: 120,
                  height: 120,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    color: _isRunning ? Colors.green[100] : Colors.grey[100],
                    border: Border.all(
                      color: _isRunning ? Colors.green : Colors.grey,
                      width: 3,
                    ),
                  ),
                  child: Icon(
                    Icons.timer,
                    size: 60,
                    color: _isRunning ? Colors.green : Colors.grey,
                  ),
                ),
                SizedBox(height: 30),

                // Timer Status
                Text(
                  'Timer Status',
                  style: TextStyle(
                    fontSize: 24,
                    fontWeight: FontWeight.bold,
                    color: Colors.grey[800],
                  ),
                ),
                SizedBox(height: 10),
                Container(
                  padding: EdgeInsets.symmetric(horizontal: 20, vertical: 10),
                  decoration: BoxDecoration(
                    color: _isRunning ? Colors.green[100] : Colors.red[100],
                    borderRadius: BorderRadius.circular(20),
                    border: Border.all(
                      color: _isRunning ? Colors.green : Colors.red,
                      width: 2,
                    ),
                  ),
                  child: Text(
                    _isRunning ? "RUNNING" : "STOPPED",
                    style: TextStyle(
                      fontSize: 20,
                      color: _isRunning ? Colors.green[800] : Colors.red[800],
                      fontWeight: FontWeight.bold,
                      letterSpacing: 1.2,
                    ),
                  ),
                ),
                SizedBox(height: 30),

                // Current Time
                Text(
                  'Current Time',
                  style: TextStyle(
                    fontSize: 20,
                    fontWeight: FontWeight.bold,
                    color: Colors.grey[800],
                  ),
                ),
                SizedBox(height: 10),
                Container(
                  padding: EdgeInsets.symmetric(horizontal: 20, vertical: 15),
                  decoration: BoxDecoration(
                    color: Colors.blue[50],
                    borderRadius: BorderRadius.circular(15),
                    border: Border.all(color: Colors.blue[200]!, width: 1),
                  ),
                  child: Text(
                    _currentTime.isEmpty ? 'No time data' : _currentTime,
                    style: TextStyle(
                      fontSize: 24,
                      color: Colors.blue[800],
                      fontFamily: 'monospace',
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
                SizedBox(height: 40),

                // Control Buttons
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    // Start Button
                    ElevatedButton.icon(
                      onPressed: _isRunning ? null : _startTimer,
                      icon: Icon(Icons.play_arrow, size: 24),
                      label: Text(
                        'Start Timer',
                        style: TextStyle(fontSize: 16),
                      ),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Colors.green,
                        foregroundColor: Colors.white,
                        padding: EdgeInsets.symmetric(
                          horizontal: 25,
                          vertical: 15,
                        ),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(25),
                        ),
                        elevation: 3,
                      ),
                    ),

                    // Stop Button
                    ElevatedButton.icon(
                      onPressed: _isRunning ? _stopTimer : null,
                      icon: Icon(Icons.stop, size: 24),
                      label: Text('Stop Timer', style: TextStyle(fontSize: 16)),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Colors.red,
                        foregroundColor: Colors.white,
                        padding: EdgeInsets.symmetric(
                          horizontal: 25,
                          vertical: 15,
                        ),
                        shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(25),
                        ),
                        elevation: 3,
                      ),
                    ),
                  ],
                ),
                SizedBox(height: 30),

                // Status Message
                if (_isRunning)
                  Container(
                    padding: EdgeInsets.symmetric(horizontal: 20, vertical: 10),
                    decoration: BoxDecoration(
                      color: Colors.blue[50],
                      borderRadius: BorderRadius.circular(15),
                      border: Border.all(color: Colors.blue[200]!, width: 1),
                    ),
                    child: Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Icon(
                          Icons.info_outline,
                          color: Colors.blue[600],
                          size: 20,
                        ),
                        SizedBox(width: 10),
                        Text(
                          'Timer is running in background...',
                          style: TextStyle(
                            fontSize: 14,
                            color: Colors.blue[600],
                            fontStyle: FontStyle.italic,
                          ),
                        ),
                      ],
                    ),
                  ),
              ],
            ),
          ),
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
