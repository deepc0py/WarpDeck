// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'peer.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

Peer _$PeerFromJson(Map<String, dynamic> json) => Peer(
      id: json['id'] as String,
      name: json['name'] as String,
      platform: json['platform'] as String,
      port: (json['port'] as num).toInt(),
      fingerprint: json['fingerprint'] as String,
      hostAddress: json['hostAddress'] as String,
      lastSeen: json['lastSeen'] == null
          ? null
          : DateTime.parse(json['lastSeen'] as String),
      isOnline: json['isOnline'] as bool? ?? true,
    );

Map<String, dynamic> _$PeerToJson(Peer instance) => <String, dynamic>{
      'id': instance.id,
      'name': instance.name,
      'platform': instance.platform,
      'port': instance.port,
      'fingerprint': instance.fingerprint,
      'hostAddress': instance.hostAddress,
      'lastSeen': instance.lastSeen?.toIso8601String(),
      'isOnline': instance.isOnline,
    };
