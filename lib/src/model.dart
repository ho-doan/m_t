import 'dart:convert';

import 'package:json_annotation/json_annotation.dart';

part 'model.g.dart';

@JsonSerializable()
class Notification {
  Notification({required this.title, required this.body});

  @JsonKey(name: 'Title')
  final String title;
  @JsonKey(name: 'Body')
  final String body;

  factory Notification.fromJson(Map<String, dynamic> json) =>
      _$NotificationFromJson(json);
  Map<String, dynamic> toJson() => _$NotificationToJson(this);
}

@JsonSerializable()
class MessageResponse {
  const MessageResponse({required this.notification, required this.mPayload});

  @JsonKey(name: 'Notification')
  final Notification notification;
  @JsonKey(name: 'mPayload')
  final String? mPayload;

  factory MessageResponse.fromJson(Map<String, dynamic> json) =>
      _$MessageResponseFromJson(json);

  factory MessageResponse.fromString(String str) =>
      MessageResponse.fromJson(jsonDecode(str));
  Map<String, dynamic> toJson() => _$MessageResponseToJson(this);
}

@JsonSerializable(explicitToJson: true)
class RegisterMessage {
  RegisterMessage({
    required this.messageType,
    required this.sender,
    required this.data,
    required this.systemType,
  });

  final String messageType;
  final int systemType;
  final Sender sender;
  final Data data;

  factory RegisterMessage.fromJson(Map<String, dynamic> json) =>
      _$RegisterMessageFromJson(json);
  Map<String, dynamic> toJson() => _$RegisterMessageToJson(this);
}

@JsonSerializable(explicitToJson: true)
class Sender {
  Sender({
    required this.connectorID,
    required this.connectorTag,
    required this.deviceID,
  });

  final String connectorID;
  final String connectorTag;
  final String deviceID;

  factory Sender.fromJson(Map<String, dynamic> json) => _$SenderFromJson(json);
  Map<String, dynamic> toJson() => _$SenderToJson(this);
}

@JsonSerializable(explicitToJson: true)
class Data {
  const Data({this.apnsToken, this.applicationID, this.apnsServerType});
  final String? apnsToken;
  final String? applicationID;
  final int? apnsServerType;

  factory Data.fromJson(Map<String, dynamic> json) => _$DataFromJson(json);
  Map<String, dynamic> toJson() => _$DataToJson(this);
}

@JsonSerializable(explicitToJson: true)
class PingMessage {
  const PingMessage({this.messageType = 'ping'});
  final String messageType;

  factory PingMessage.fromJson(Map<String, dynamic> json) =>
      _$PingMessageFromJson(json);
  Map<String, dynamic> toJson() => _$PingMessageToJson(this);
}

@JsonSerializable(explicitToJson: true)
class PongMessage {
  const PongMessage({this.pong});
  final String? pong;

  factory PongMessage.fromJson(Map<String, dynamic> json) =>
      _$PongMessageFromJson(json);
  Map<String, dynamic> toJson() => _$PongMessageToJson(this);
}
