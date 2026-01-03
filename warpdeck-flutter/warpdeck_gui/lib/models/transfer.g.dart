// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'transfer.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

FileInfo _$FileInfoFromJson(Map<String, dynamic> json) => FileInfo(
      name: json['name'] as String,
      size: (json['size'] as num).toInt(),
      path: json['path'] as String?,
      relativePath: json['relativePath'] as String?,
      hash: json['hash'] as String?,
    );

Map<String, dynamic> _$FileInfoToJson(FileInfo instance) => <String, dynamic>{
      'name': instance.name,
      'size': instance.size,
      'path': instance.path,
      'relativePath': instance.relativePath,
      'hash': instance.hash,
    };

QueuedTransfer _$QueuedTransferFromJson(Map<String, dynamic> json) =>
    QueuedTransfer(
      queueId: json['queueId'] as String,
      peerDeviceId: json['peerDeviceId'] as String,
      peerName: json['peerName'] as String,
      status: $enumDecode(_$QueuedTransferStatusEnumMap, json['status']),
      transferId: json['transferId'] as String?,
      fileCount: (json['fileCount'] as num).toInt(),
      totalBytes: (json['totalBytes'] as num).toInt(),
      errorMessage: json['errorMessage'] as String?,
    );

Map<String, dynamic> _$QueuedTransferToJson(QueuedTransfer instance) =>
    <String, dynamic>{
      'queueId': instance.queueId,
      'peerDeviceId': instance.peerDeviceId,
      'peerName': instance.peerName,
      'status': _$QueuedTransferStatusEnumMap[instance.status]!,
      'transferId': instance.transferId,
      'fileCount': instance.fileCount,
      'totalBytes': instance.totalBytes,
      'errorMessage': instance.errorMessage,
    };

const _$QueuedTransferStatusEnumMap = {
  QueuedTransferStatus.queued: 'queued',
  QueuedTransferStatus.active: 'active',
  QueuedTransferStatus.completed: 'completed',
  QueuedTransferStatus.failed: 'failed',
  QueuedTransferStatus.cancelled: 'cancelled',
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
