import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

// Flutter Timer Plugin
// Simple integration for Flutter app

class FlutterTimerPlugin {
  static final FlutterTimerPlugin _instance = FlutterTimerPlugin._internal();
  factory FlutterTimerPlugin() => _instance;
  FlutterTimerPlugin._internal();

  late DynamicLibrary _dylib;
  bool _initialized = false;

  // Initialize the plugin
  Future<bool> initialize() async {
    try {
      if (Platform.isWindows) {
        // Try to load from the main executable
        _dylib = DynamicLibrary.open('local_push_connectivity_example.exe');
        _initialized = true;
        print('FlutterTimerPlugin: Initialized successfully');
        return true;
      } else {
        print('FlutterTimerPlugin: Only supported on Windows');
        return false;
      }
    } catch (e) {
      print('FlutterTimerPlugin: Failed to initialize: $e');
      return false;
    }
  }

  // Start timer
  Future<bool> startTimer() async {
    if (!_initialized) {
      print('FlutterTimerPlugin: Not initialized');
      return false;
    }

    try {
      final startTimer = _dylib
          .lookupFunction<Bool Function(), bool Function()>(
            'FlutterTimer_Start',
          );

      final result = startTimer();
      print('FlutterTimerPlugin: Start timer result: $result');
      return result;
    } catch (e) {
      print('FlutterTimerPlugin: Failed to start timer: $e');
      return false;
    }
  }

  // Stop timer
  Future<void> stopTimer() async {
    if (!_initialized) {
      print('FlutterTimerPlugin: Not initialized');
      return;
    }

    try {
      final stopTimer = _dylib.lookupFunction<Void Function(), void Function()>(
        'FlutterTimer_Stop',
      );

      stopTimer();
      print('FlutterTimerPlugin: Timer stopped');
    } catch (e) {
      print('FlutterTimerPlugin: Failed to stop timer: $e');
    }
  }

  // Check if timer is running
  bool isTimerRunning() {
    if (!_initialized) {
      print('FlutterTimerPlugin: Not initialized');
      return false;
    }

    try {
      final isRunning = _dylib.lookupFunction<Bool Function(), bool Function()>(
        'FlutterTimer_IsRunning',
      );

      return isRunning();
    } catch (e) {
      print('FlutterTimerPlugin: Failed to check timer status: $e');
      return false;
    }
  }

  // Get current time
  String getCurrentTime() {
    if (!_initialized) {
      print('FlutterTimerPlugin: Not initialized');
      return '';
    }

    try {
      final getCurrentTime = _dylib
          .lookupFunction<Pointer<Utf8> Function(), Pointer<Utf8> Function()>(
            'FlutterTimer_GetCurrentTime',
          );

      final timePtr = getCurrentTime();
      return timePtr.toDartString();
    } catch (e) {
      print('FlutterTimerPlugin: Failed to get current time: $e');
      return '';
    }
  }
}
