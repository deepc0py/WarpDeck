import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:material_design_icons_flutter/material_design_icons_flutter.dart';

import '../services/warpdeck_service.dart';
import '../models/transfer.dart';

class TransferListWidget extends ConsumerWidget {
  const TransferListWidget({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final warpDeckState = ref.watch(warpDeckServiceProvider);
    final transfers = warpDeckState.activeTransfers.values.toList();

    if (transfers.isEmpty) {
      return Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              MdiIcons.transferRight,
              size: 64,
              color: Colors.grey,
            ),
            const SizedBox(height: 16),
            Text(
              'No active transfers',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                color: Colors.grey,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              'File transfers will appear here',
              style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                color: Colors.grey,
              ),
            ),
          ],
        ),
      );
    }

    return ListView.builder(
      itemCount: transfers.length,
      itemBuilder: (context, index) {
        final transfer = transfers[index];
        return TransferCard(transfer: transfer);
      },
    );
  }
}

class TransferCard extends ConsumerWidget {
  final Transfer transfer;

  const TransferCard({
    super.key,
    required this.transfer,
  });

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Header
            Row(
              children: [
                Icon(
                  transfer.direction == TransferDirection.incoming
                      ? MdiIcons.download
                      : MdiIcons.upload,
                  color: transfer.direction == TransferDirection.incoming
                      ? Colors.blue
                      : Colors.green,
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        transfer.direction == TransferDirection.incoming
                            ? 'Receiving from ${transfer.peerName}'
                            : 'Sending to ${transfer.peerName}',
                        style: Theme.of(context).textTheme.titleMedium,
                      ),
                      Text(
                        '${transfer.files.length} file${transfer.files.length != 1 ? 's' : ''}',
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          color: Colors.grey,
                        ),
                      ),
                    ],
                  ),
                ),
                _StatusChip(status: transfer.status),
              ],
            ),
            
            const SizedBox(height: 16),
            
            // Progress (if in progress)
            if (transfer.status == TransferStatus.inProgress) ...[
              Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      Text(
                        transfer.formattedProgress,
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          fontWeight: FontWeight.w500,
                        ),
                      ),
                      Text(
                        transfer.formattedSpeed,
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          color: Colors.grey,
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 8),
                  LinearProgressIndicator(
                    value: transfer.progress,
                    backgroundColor: Colors.grey.withOpacity(0.2),
                  ),
                  const SizedBox(height: 8),
                  Row(
                    mainAxisAlignment: MainAxisAlignment.spaceBetween,
                    children: [
                      Text(
                        '${transfer.bytesTransferred} / ${transfer.totalBytes} bytes',
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          color: Colors.grey,
                        ),
                      ),
                      if (transfer.estimatedTimeRemaining != null)
                        Text(
                          _formatDuration(transfer.estimatedTimeRemaining!),
                          style: Theme.of(context).textTheme.bodySmall?.copyWith(
                            color: Colors.grey,
                          ),
                        ),
                    ],
                  ),
                ],
              ),
              const SizedBox(height: 16),
            ],
            
            // Files list
            ...transfer.files.map((file) => Padding(
              padding: const EdgeInsets.only(bottom: 8),
              child: Row(
                children: [
                  Icon(
                    _getFileIcon(file.name),
                    size: 16,
                    color: Colors.grey,
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: Text(
                      file.name,
                      style: Theme.of(context).textTheme.bodySmall,
                      overflow: TextOverflow.ellipsis,
                    ),
                  ),
                  Text(
                    file.formattedSize,
                    style: Theme.of(context).textTheme.bodySmall?.copyWith(
                      color: Colors.grey,
                    ),
                  ),
                ],
              ),
            )),
            
            // Actions
            if (transfer.status == TransferStatus.pending &&
                transfer.direction == TransferDirection.incoming) ...[
              const SizedBox(height: 16),
              Row(
                mainAxisAlignment: MainAxisAlignment.end,
                children: [
                  OutlinedButton(
                    onPressed: () => ref
                        .read(warpDeckServiceProvider.notifier)
                        .respondToTransfer(transfer.id, false),
                    child: const Text('Decline'),
                  ),
                  const SizedBox(width: 12),
                  ElevatedButton(
                    onPressed: () => ref
                        .read(warpDeckServiceProvider.notifier)
                        .respondToTransfer(transfer.id, true),
                    child: const Text('Accept'),
                  ),
                ],
              ),
            ],
            
            // Error message
            if (transfer.status == TransferStatus.failed &&
                transfer.errorMessage != null) ...[
              const SizedBox(height: 12),
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.red.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(
                    color: Colors.red.withOpacity(0.3),
                  ),
                ),
                child: Row(
                  children: [
                    Icon(
                      MdiIcons.alertCircle,
                      color: Colors.red,
                      size: 16,
                    ),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Text(
                        transfer.errorMessage!,
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          color: Colors.red,
                        ),
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ],
        ),
      ),
    );
  }

  IconData _getFileIcon(String fileName) {
    final extension = fileName.split('.').last.toLowerCase();
    switch (extension) {
      case 'jpg':
      case 'jpeg':
      case 'png':
      case 'gif':
      case 'bmp':
        return MdiIcons.fileImage;
      case 'mp4':
      case 'avi':
      case 'mov':
      case 'mkv':
        return MdiIcons.fileVideo;
      case 'mp3':
      case 'wav':
      case 'flac':
      case 'aac':
        return MdiIcons.fileMusic;
      case 'pdf':
        return MdiIcons.filePdfBox;
      case 'doc':
      case 'docx':
        return MdiIcons.fileWord;
      case 'xls':
      case 'xlsx':
        return MdiIcons.fileExcel;
      case 'zip':
      case 'rar':
      case '7z':
        return MdiIcons.zipBox;
      default:
        return MdiIcons.fileOutline;
    }
  }

  String _formatDuration(Duration duration) {
    if (duration.inHours > 0) {
      return '${duration.inHours}h ${duration.inMinutes % 60}m';
    } else if (duration.inMinutes > 0) {
      return '${duration.inMinutes}m ${duration.inSeconds % 60}s';
    } else {
      return '${duration.inSeconds}s';
    }
  }
}

class _StatusChip extends StatelessWidget {
  final TransferStatus status;

  const _StatusChip({required this.status});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: _getStatusColor().withOpacity(0.1),
        borderRadius: BorderRadius.circular(12),
        border: Border.all(
          color: _getStatusColor().withOpacity(0.3),
        ),
      ),
      child: Text(
        _getStatusText(),
        style: Theme.of(context).textTheme.bodySmall?.copyWith(
          color: _getStatusColor(),
          fontWeight: FontWeight.w500,
        ),
      ),
    );
  }

  Color _getStatusColor() {
    switch (status) {
      case TransferStatus.pending:
        return Colors.orange;
      case TransferStatus.inProgress:
        return Colors.blue;
      case TransferStatus.completed:
        return Colors.green;
      case TransferStatus.failed:
        return Colors.red;
      case TransferStatus.cancelled:
        return Colors.grey;
    }
  }

  String _getStatusText() {
    switch (status) {
      case TransferStatus.pending:
        return 'Pending';
      case TransferStatus.inProgress:
        return 'In Progress';
      case TransferStatus.completed:
        return 'Completed';
      case TransferStatus.failed:
        return 'Failed';
      case TransferStatus.cancelled:
        return 'Cancelled';
    }
  }
}