import 'package:flutter/material.dart';
import 'dart:async';
import 'flutter_ipc_plugin.dart';

class IPCApp extends StatefulWidget {
  const IPCApp({super.key});
  @override
  State<IPCApp> createState() => _IPCAppState();
}

class _IPCAppState extends State<IPCApp> {
  final FlutterIPCPlugin _plugin = FlutterIPCPlugin();
  bool _isRunning = false;
  int _currentNumber = 0;
  int _sentNumber = 0;
  Timer? _timer;

  @override
  void initState() {
    super.initState();
    _initializePlugin();
  }

  Future<void> _initializePlugin() async {
    final success = await _plugin.initialize();
    if (success) {
      print('IPC Plugin initialized successfully');
    } else {
      print('Failed to initialize IPC Plugin');
    }
  }

  Future<void> _startIPC() async {
    print('Starting IPC system...');
    final success = await _plugin.startIPC();
    print('IPC start result: $success');

    if (success) {
      setState(() {
        _isRunning = true;
      });
      print('IPC system started successfully');

      // Start polling for numbers from child
      _timer = Timer.periodic(Duration(seconds: 1), (timer) {
        if (_plugin.isNumberReady()) {
          final number = _plugin.getCurrentNumber();
          print('Received number from child: $number');
          setState(() {
            _currentNumber = number;
          });
          _plugin.resetNumberReady();
        }
      });
    } else {
      print('Failed to start IPC system');
    }
  }

  Future<void> _stopIPC() async {
    await _plugin.stopIPC();
    _timer?.cancel();
    setState(() {
      _isRunning = false;
      _currentNumber = 0;
    });
  }

  void _sendNumber(int number) {
    setState(() {
      _sentNumber = number;
    });
    // In real implementation, this would send to child process
    print('Sending number to child: $number');
  }

  @override
  Widget build(BuildContext context) {
    print('Building IPC App - _isRunning: $_isRunning');
    return Scaffold(
      body: Container(
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [Colors.purple[50]!, Colors.white],
          ),
        ),
        child: Center(
          child: Padding(
            padding: EdgeInsets.all(20),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                // IPC Icon
                Container(
                  width: 120,
                  height: 120,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    color: _isRunning ? Colors.purple[100] : Colors.grey[100],
                    border: Border.all(
                      color: _isRunning ? Colors.purple : Colors.grey,
                      width: 3,
                    ),
                  ),
                  child: Icon(
                    Icons.sync,
                    size: 60,
                    color: _isRunning ? Colors.purple : Colors.grey,
                  ),
                ),
                SizedBox(height: 30),

                // IPC Status
                Text(
                  'IPC Status',
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
                    color: _isRunning ? Colors.purple[100] : Colors.red[100],
                    borderRadius: BorderRadius.circular(20),
                    border: Border.all(
                      color: _isRunning ? Colors.purple : Colors.red,
                      width: 2,
                    ),
                  ),
                  child: Text(
                    _isRunning ? "RUNNING" : "STOPPED",
                    style: TextStyle(
                      fontSize: 20,
                      color: _isRunning ? Colors.purple[800] : Colors.red[800],
                      fontWeight: FontWeight.bold,
                      letterSpacing: 1.2,
                    ),
                  ),
                ),
                SizedBox(height: 30),

                // Sent Number
                Text(
                  'Sent Number',
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
                    _sentNumber.toString(),
                    style: TextStyle(
                      fontSize: 24,
                      color: Colors.blue[800],
                      fontFamily: 'monospace',
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
                SizedBox(height: 20),

                // Received Number
                Text(
                  'Received Number (Child + 1)',
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
                    color: Colors.green[50],
                    borderRadius: BorderRadius.circular(15),
                    border: Border.all(color: Colors.green[200]!, width: 1),
                  ),
                  child: Text(
                    _currentNumber.toString(),
                    style: TextStyle(
                      fontSize: 24,
                      color: Colors.green[800],
                      fontFamily: 'monospace',
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
                SizedBox(height: 40),

                // Number Input
                Container(
                  padding: EdgeInsets.symmetric(horizontal: 20, vertical: 15),
                  decoration: BoxDecoration(
                    color: Colors.grey[50],
                    borderRadius: BorderRadius.circular(15),
                    border: Border.all(color: Colors.grey[300]!, width: 1),
                  ),
                  child: Row(
                    children: [
                      Expanded(
                        child: TextField(
                          keyboardType: TextInputType.number,
                          decoration: InputDecoration(
                            hintText: 'Enter number to send',
                            border: InputBorder.none,
                            contentPadding: EdgeInsets.symmetric(
                              horizontal: 10,
                            ),
                          ),
                          onSubmitted: (value) {
                            final number = int.tryParse(value);
                            if (number != null) {
                              _sendNumber(number);
                            }
                          },
                        ),
                      ),
                      SizedBox(width: 10),
                      ElevatedButton(
                        onPressed: () {
                          final number = _sentNumber + 1;
                          _sendNumber(number);
                        },
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.blue,
                          foregroundColor: Colors.white,
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(10),
                          ),
                        ),
                        child: Text('Send'),
                      ),
                    ],
                  ),
                ),
                SizedBox(height: 40),

                // Control Buttons
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    // Start Button
                    ElevatedButton.icon(
                      onPressed: _isRunning ? null : _startIPC,
                      icon: Icon(Icons.play_arrow, size: 24),
                      label: Text('Start IPC', style: TextStyle(fontSize: 16)),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Colors.purple,
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
                      onPressed: _isRunning ? _stopIPC : null,
                      icon: Icon(Icons.stop, size: 24),
                      label: Text('Stop IPC', style: TextStyle(fontSize: 16)),
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
                SizedBox(height: 20),

                // Debug Button
                ElevatedButton(
                  onPressed: () {
                    print('Debug - _isRunning: $_isRunning');
                    print('Debug - _currentNumber: $_currentNumber');
                    print('Debug - _sentNumber: $_sentNumber');
                  },
                  style: ElevatedButton.styleFrom(
                    backgroundColor: Colors.orange,
                    foregroundColor: Colors.white,
                  ),
                  child: Text('Debug Status'),
                ),
                SizedBox(height: 30),

                // Status Message
                if (_isRunning)
                  Container(
                    padding: EdgeInsets.symmetric(horizontal: 20, vertical: 10),
                    decoration: BoxDecoration(
                      color: Colors.purple[50],
                      borderRadius: BorderRadius.circular(15),
                      border: Border.all(color: Colors.purple[200]!, width: 1),
                    ),
                    child: Row(
                      mainAxisSize: MainAxisSize.min,
                      children: [
                        Icon(
                          Icons.info_outline,
                          color: Colors.purple[600],
                          size: 20,
                        ),
                        SizedBox(width: 10),
                        Text(
                          'IPC system running - Child process listening...',
                          style: TextStyle(
                            fontSize: 14,
                            color: Colors.purple[600],
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
