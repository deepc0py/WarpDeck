// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'transfer.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

FileInfo _$FileInfoFromJson(Map<String, dynamic> json) => FileInfo(
      name: json['name'] as String,
      size: (json['size'] as num).toInt(),
      path: json['path'] as String?,
      hash: json['hash'] as String?,
    );

Map<String, dynamic> _$FileInfoToJson(FileInfo instance) => <String, dynamic>{
      'name': instance.name,
      'size': instance.size,
      'path': instance.path,
      'hash': instance.hash,
    };

Transfer _$TransferFromJson(Map<String, dynamic> json) => Transfer(
      id: json['id'] as String,
      peerId: json['peerId'] as String,
      peerName: json['peerName'] as String,
      files: (json['files'] as List<dynamic>)
          .map((e) => FileInfo.fromJson(e as Map<String, dynamic>))
          .toList(),
      direction: $enumDecode(_$TransferDirectionEnumMap, json['direction']),
      status: $enumDecode(_$TransferStatusEnumMap, json['status']),
      progress: (json['progress'] as num?)?.toDouble() ?? 0.0,
      bytesTransferred: (json['bytesTransferred'] as num?)?.toInt() ?? 0,
      totalBytes: (json['totalBytes'] as num).toInt(),
      createdAt: DateTime.parse(json['createdAt'] as String),
      completedAt: json['completedAt'] == null
          ? null
          : DateTime.parse(json['completedAt'] as String),
      errorMessage: json['errorMessage'] as String?,
    );

Map<String, dynamic> _$TransferToJson(Transfer instance) => <String, dynamic>{
      'id': instance.id,
      'peerId': instance.peerId,
      'peerName': instance.peerName,
      'files': instance.files,
      'direction': _$TransferDirectionEnumMap[instance.direction]!,
      'status': _$TransferStatusEnumMap[instance.status]!,
      'progress': instance.progress,
      'bytesTransferred': instance.bytesTransferred,
      'totalBytes': instance.totalBytes,
      'createdAt': instance.createdAt.toIso8601String(),
      'completedAt': instance.completedAt?.toIso8601String(),
      'errorMessage': instance.errorMessage,
    };

const _$TransferDirectionEnumMap = {
  TransferDirection.incoming: 'incoming',
  TransferDirection.outgoing: 'outgoing',
};

const _$TransferStatusEnumMap = {
  TransferStatus.pending: 'pending',
  TransferStatus.inProgress: 'inProgress',
  TransferStatus.completed: 'completed',
  TransferStatus.failed: 'failed',
  TransferStatus.cancelled: 'cancelled',
};
