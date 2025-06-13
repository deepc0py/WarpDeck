import 'package:json_annotation/json_annotation.dart';

part 'peer.g.dart';

@JsonSerializable()
class Peer {
  final String id;
  final String name;
  final String platform;
  final int port;
  final String fingerprint;
  final String hostAddress;
  final DateTime? lastSeen;
  final bool isOnline;

  const Peer({
    required this.id,
    required this.name,
    required this.platform,
    required this.port,
    required this.fingerprint,
    required this.hostAddress,
    this.lastSeen,
    this.isOnline = true,
  });

  factory Peer.fromJson(Map<String, dynamic> json) => _$PeerFromJson(json);
  Map<String, dynamic> toJson() => _$PeerToJson(this);

  Peer copyWith({
    String? id,
    String? name,
    String? platform,
    int? port,
    String? fingerprint,
    String? hostAddress,
    DateTime? lastSeen,
    bool? isOnline,
  }) {
    return Peer(
      id: id ?? this.id,
      name: name ?? this.name,
      platform: platform ?? this.platform,
      port: port ?? this.port,
      fingerprint: fingerprint ?? this.fingerprint,
      hostAddress: hostAddress ?? this.hostAddress,
      lastSeen: lastSeen ?? this.lastSeen,
      isOnline: isOnline ?? this.isOnline,
    );
  }

  String get shortId => id.length > 8 ? id.substring(0, 8) : id;
  
  String get platformIcon {
    switch (platform.toLowerCase()) {
      case 'macos':
      case 'mac':
        return '💻';
      case 'linux':
      case 'steamdeck':
        return '🐧';
      case 'windows':
        return '🪟';
      default:
        return '📱';
    }
  }
}