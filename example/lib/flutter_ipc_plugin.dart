import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

// Flutter IPC Plugin
// Communication between Parent App and Child Process

class FlutterIPCPlugin {
  static final FlutterIPCPlugin _instance = FlutterIPCPlugin._internal();
  factory FlutterIPCPlugin() => _instance;
  FlutterIPCPlugin._internal();

  late DynamicLibrary _dylib;
  bool _initialized = false;

  // Initialize the plugin
  Future<bool> initialize() async {
    try {
      if (Platform.isWindows) {
        // Try to load from the main executable
        _dylib = DynamicLibrary.open('local_push_connectivity_example.exe');
        _initialized = true;
        print('FlutterIPCPlugin: Initialized successfully');
        return true;
      } else {
        print('FlutterIPCPlugin: Only supported on Windows');
        return false;
      }
    } catch (e) {
      print('FlutterIPCPlugin: Failed to initialize: $e');
      return false;
    }
  }

  // Start IPC system
  Future<bool> startIPC() async {
    if (!_initialized) {
      print('FlutterIPCPlugin: Not initialized');
      return false;
    }

    try {
      final startIPC = _dylib.lookupFunction<Bool Function(), bool Function()>(
        'FlutterIPC_Start',
      );

      final result = startIPC();
      print('FlutterIPCPlugin: Start IPC result: $result');
      return result;
    } catch (e) {
      print('FlutterIPCPlugin: Failed to start IPC: $e');
      return false;
    }
  }

  // Stop IPC system
  Future<void> stopIPC() async {
    if (!_initialized) {
      print('FlutterIPCPlugin: Not initialized');
      return;
    }

    try {
      final stopIPC = _dylib.lookupFunction<Void Function(), void Function()>(
        'FlutterIPC_Stop',
      );

      stopIPC();
      print('FlutterIPCPlugin: IPC system stopped');
    } catch (e) {
      print('FlutterIPCPlugin: Failed to stop IPC: $e');
    }
  }

  // Get current number from child
  int getCurrentNumber() {
    if (!_initialized) {
      print('FlutterIPCPlugin: Not initialized');
      return 0;
    }

    try {
      final getCurrentNumber = _dylib
          .lookupFunction<Int32 Function(), int Function()>(
            'FlutterIPC_GetCurrentNumber',
          );

      return getCurrentNumber();
    } catch (e) {
      print('FlutterIPCPlugin: Failed to get current number: $e');
      return 0;
    }
  }

  // Check if number is ready
  bool isNumberReady() {
    if (!_initialized) {
      print('FlutterIPCPlugin: Not initialized');
      return false;
    }

    try {
      final isNumberReady = _dylib
          .lookupFunction<Bool Function(), bool Function()>(
            'FlutterIPC_IsNumberReady',
          );

      return isNumberReady();
    } catch (e) {
      print('FlutterIPCPlugin: Failed to check number ready: $e');
      return false;
    }
  }

  // Reset number ready flag
  void resetNumberReady() {
    if (!_initialized) {
      print('FlutterIPCPlugin: Not initialized');
      return;
    }

    try {
      final resetNumberReady = _dylib
          .lookupFunction<Void Function(), void Function()>(
            'FlutterIPC_ResetNumberReady',
          );

      resetNumberReady();
      print('FlutterIPCPlugin: Number ready flag reset');
    } catch (e) {
      print('FlutterIPCPlugin: Failed to reset number ready: $e');
    }
  }
}
