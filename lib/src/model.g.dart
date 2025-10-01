// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'model.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

Notification _$NotificationFromJson(Map<String, dynamic> json) =>
    Notification(title: json['Title'] as String, body: json['Body'] as String);

Map<String, dynamic> _$NotificationToJson(Notification instance) =>
    <String, dynamic>{'Title': instance.title, 'Body': instance.body};

MessageResponse _$MessageResponseFromJson(Map<String, dynamic> json) =>
    MessageResponse(
      notification: Notification.fromJson(
        json['Notification'] as Map<String, dynamic>,
      ),
      mPayload: json['mPayload'] as String?,
    );

Map<String, dynamic> _$MessageResponseToJson(MessageResponse instance) =>
    <String, dynamic>{
      'Notification': instance.notification,
      'mPayload': instance.mPayload,
    };

RegisterMessage _$RegisterMessageFromJson(Map<String, dynamic> json) =>
    RegisterMessage(
      messageType: json['messageType'] as String,
      sender: Sender.fromJson(json['sender'] as Map<String, dynamic>),
      data: Data.fromJson(json['data'] as Map<String, dynamic>),
      systemType: (json['systemType'] as num).toInt(),
    );

Map<String, dynamic> _$RegisterMessageToJson(RegisterMessage instance) =>
    <String, dynamic>{
      'messageType': instance.messageType,
      'systemType': instance.systemType,
      'sender': instance.sender.toJson(),
      'data': instance.data.toJson(),
    };

Sender _$SenderFromJson(Map<String, dynamic> json) => Sender(
  connectorID: json['connectorID'] as String,
  connectorTag: json['connectorTag'] as String,
  deviceID: json['deviceID'] as String,
);

Map<String, dynamic> _$SenderToJson(Sender instance) => <String, dynamic>{
  'connectorID': instance.connectorID,
  'connectorTag': instance.connectorTag,
  'deviceID': instance.deviceID,
};

Data _$DataFromJson(Map<String, dynamic> json) => Data(
  apnsToken: json['apnsToken'] as String?,
  applicationID: json['applicationID'] as String?,
  apnsServerType: (json['apnsServerType'] as num?)?.toInt(),
);

Map<String, dynamic> _$DataToJson(Data instance) => <String, dynamic>{
  'apnsToken': instance.apnsToken,
  'applicationID': instance.applicationID,
  'apnsServerType': instance.apnsServerType,
};

PingMessage _$PingMessageFromJson(Map<String, dynamic> json) =>
    PingMessage(messageType: json['messageType'] as String? ?? 'ping');

Map<String, dynamic> _$PingMessageToJson(PingMessage instance) =>
    <String, dynamic>{'messageType': instance.messageType};

PongMessage _$PongMessageFromJson(Map<String, dynamic> json) =>
    PongMessage(pong: json['pong'] as String?);

Map<String, dynamic> _$PongMessageToJson(PongMessage instance) =>
    <String, dynamic>{'pong': instance.pong};
