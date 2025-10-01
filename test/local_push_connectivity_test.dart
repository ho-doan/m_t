// import 'package:flutter_test/flutter_test.dart';
// import 'package:local_push_connectivity/local_push_connectivity.dart';
// import 'package:local_push_connectivity/local_push_connectivity_platform_interface.dart';
// import 'package:local_push_connectivity/local_push_connectivity_method_channel.dart';
// import 'package:plugin_platform_interface/plugin_platform_interface.dart';

// class MockLocalPushConnectivityPlatform
//     with MockPlatformInterfaceMixin
//     implements LocalPushConnectivityPlatform {

//   @override
//   Future<String?> getPlatformVersion() => Future.value('42');
// }

// void main() {
//   final LocalPushConnectivityPlatform initialPlatform = LocalPushConnectivityPlatform.instance;

//   test('$MethodChannelLocalPushConnectivity is the default instance', () {
//     expect(initialPlatform, isInstanceOf<MethodChannelLocalPushConnectivity>());
//   });

//   test('getPlatformVersion', () async {
//     LocalPushConnectivity localPushConnectivityPlugin = LocalPushConnectivity();
//     MockLocalPushConnectivityPlatform fakePlatform = MockLocalPushConnectivityPlatform();
//     LocalPushConnectivityPlatform.instance = fakePlatform;

//     expect(await localPushConnectivityPlugin.getPlatformVersion(), '42');
//   });
// }
