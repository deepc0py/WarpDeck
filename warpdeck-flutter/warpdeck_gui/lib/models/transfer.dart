import 'package:json_annotation/json_annotation.dart';

part 'transfer.g.dart';

enum TransferStatus {
  pending,
  inProgress,
  completed,
  failed,
  cancelled,
}

enum TransferDirection {
  incoming,
  outgoing,
}

@JsonSerializable()
class FileInfo {
  final String name;
  final int size;
  final String? path;
  final String? hash;

  const FileInfo({
    required this.name,
    required this.size,
    this.path,
    this.hash,
  });

  factory FileInfo.fromJson(Map<String, dynamic> json) => _$FileInfoFromJson(json);
  Map<String, dynamic> toJson() => _$FileInfoToJson(this);

  String get formattedSize {
    if (size < 1024) return '${size}B';
    if (size < 1024 * 1024) return '${(size / 1024).toStringAsFixed(1)}KB';
    if (size < 1024 * 1024 * 1024) return '${(size / (1024 * 1024)).toStringAsFixed(1)}MB';
    return '${(size / (1024 * 1024 * 1024)).toStringAsFixed(1)}GB';
  }
}

@JsonSerializable()
class Transfer {
  final String id;
  final String peerId;
  final String peerName;
  final List<FileInfo> files;
  final TransferDirection direction;
  final TransferStatus status;
  final double progress;
  final int bytesTransferred;
  final int totalBytes;
  final DateTime createdAt;
  final DateTime? completedAt;
  final String? errorMessage;

  const Transfer({
    required this.id,
    required this.peerId,
    required this.peerName,
    required this.files,
    required this.direction,
    required this.status,
    this.progress = 0.0,
    this.bytesTransferred = 0,
    required this.totalBytes,
    required this.createdAt,
    this.completedAt,
    this.errorMessage,
  });

  factory Transfer.fromJson(Map<String, dynamic> json) => _$TransferFromJson(json);
  Map<String, dynamic> toJson() => _$TransferToJson(this);

  Transfer copyWith({
    String? id,
    String? peerId,
    String? peerName,
    List<FileInfo>? files,
    TransferDirection? direction,
    TransferStatus? status,
    double? progress,
    int? bytesTransferred,
    int? totalBytes,
    DateTime? createdAt,
    DateTime? completedAt,
    String? errorMessage,
  }) {
    return Transfer(
      id: id ?? this.id,
      peerId: peerId ?? this.peerId,
      peerName: peerName ?? this.peerName,
      files: files ?? this.files,
      direction: direction ?? this.direction,
      status: status ?? this.status,
      progress: progress ?? this.progress,
      bytesTransferred: bytesTransferred ?? this.bytesTransferred,
      totalBytes: totalBytes ?? this.totalBytes,
      createdAt: createdAt ?? this.createdAt,
      completedAt: completedAt ?? this.completedAt,
      errorMessage: errorMessage ?? this.errorMessage,
    );
  }

  String get formattedSpeed {
    if (status != TransferStatus.inProgress) return '';
    
    final duration = DateTime.now().difference(createdAt);
    if (duration.inSeconds == 0) return '';
    
    final speed = bytesTransferred / duration.inSeconds;
    if (speed < 1024) return '${speed.toStringAsFixed(0)}B/s';
    if (speed < 1024 * 1024) return '${(speed / 1024).toStringAsFixed(1)}KB/s';
    return '${(speed / (1024 * 1024)).toStringAsFixed(1)}MB/s';
  }

  String get formattedProgress {
    return '${(progress * 100).toStringAsFixed(1)}%';
  }

  Duration? get estimatedTimeRemaining {
    if (status != TransferStatus.inProgress || progress == 0) return null;
    
    final elapsed = DateTime.now().difference(createdAt);
    final totalEstimated = elapsed.inSeconds / progress;
    final remaining = totalEstimated - elapsed.inSeconds;
    
    return Duration(seconds: remaining.round());
  }
}