import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:material_design_icons_flutter/material_design_icons_flutter.dart';
import 'package:file_picker/file_picker.dart';

import '../services/warpdeck_service.dart';
import '../models/peer.dart';

class SendFilesDialog extends ConsumerStatefulWidget {
  final Peer? targetPeer;

  const SendFilesDialog({
    super.key,
    this.targetPeer,
  });

  @override
  ConsumerState<SendFilesDialog> createState() => _SendFilesDialogState();
}

class _SendFilesDialogState extends ConsumerState<SendFilesDialog> {
  List<PlatformFile> selectedFiles = [];
  Peer? selectedPeer;
  bool isLoading = false;

  @override
  void initState() {
    super.initState();
    selectedPeer = widget.targetPeer;
  }

  @override
  Widget build(BuildContext context) {
    final warpDeckState = ref.watch(warpDeckServiceProvider);
    final availablePeers = warpDeckState.discoveredPeers.values
        .where((peer) => peer.isOnline)
        .toList();

    return AlertDialog(
      title: Row(
        children: [
          Icon(MdiIcons.send),
          const SizedBox(width: 12),
          const Text('Send Files'),
        ],
      ),
      content: SizedBox(
        width: 500,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Peer Selection
            if (widget.targetPeer == null) ...[
              Text(
                'Select Target Device',
                style: Theme.of(context).textTheme.titleSmall,
              ),
              const SizedBox(height: 8),
              Container(
                width: double.infinity,
                padding: const EdgeInsets.symmetric(horizontal: 12),
                decoration: BoxDecoration(
                  border: Border.all(color: Colors.grey),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: DropdownButtonHideUnderline(
                  child: DropdownButton<Peer>(
                    value: selectedPeer,
                    hint: const Text('Choose a peer'),
                    isExpanded: true,
                    items: availablePeers.map((peer) {
                      return DropdownMenuItem<Peer>(
                        value: peer,
                        child: Row(
                          children: [
                            Text(peer.platformIcon, style: const TextStyle(fontSize: 20)),
                            const SizedBox(width: 12),
                            Expanded(
                              child: Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                mainAxisSize: MainAxisSize.min,
                                children: [
                                  Text(peer.name),
                                  Text(
                                    peer.platform.toUpperCase(),
                                    style: Theme.of(context).textTheme.bodySmall?.copyWith(
                                      color: Colors.grey,
                                    ),
                                  ),
                                ],
                              ),
                            ),
                          ],
                        ),
                      );
                    }).toList(),
                    onChanged: (peer) {
                      setState(() {
                        selectedPeer = peer;
                      });
                    },
                  ),
                ),
              ),
              const SizedBox(height: 24),
            ] else ...[
              // Show selected peer
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Theme.of(context).colorScheme.primary.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Row(
                  children: [
                    Text(selectedPeer!.platformIcon, style: const TextStyle(fontSize: 24)),
                    const SizedBox(width: 12),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            'Sending to ${selectedPeer!.name}',
                            style: Theme.of(context).textTheme.titleSmall,
                          ),
                          Text(
                            selectedPeer!.platform.toUpperCase(),
                            style: Theme.of(context).textTheme.bodySmall?.copyWith(
                              color: Colors.grey,
                            ),
                          ),
                        ],
                      ),
                    ),
                  ],
                ),
              ),
              const SizedBox(height: 24),
            ],

            // File Selection
            Text(
              'Select Files',
              style: Theme.of(context).textTheme.titleSmall,
            ),
            const SizedBox(height: 8),
            
            // File picker button
            OutlinedButton.icon(
              onPressed: _pickFiles,
              icon: Icon(MdiIcons.fileMultiple),
              label: const Text('Choose Files'),
              style: OutlinedButton.styleFrom(
                minimumSize: const Size(double.infinity, 48),
              ),
            ),
            
            const SizedBox(height: 16),
            
            // Selected files list
            if (selectedFiles.isNotEmpty) ...[
              Text(
                'Selected Files (${selectedFiles.length})',
                style: Theme.of(context).textTheme.titleSmall,
              ),
              const SizedBox(height: 8),
              Container(
                constraints: const BoxConstraints(maxHeight: 200),
                decoration: BoxDecoration(
                  border: Border.all(color: Colors.grey.withOpacity(0.3)),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: ListView.builder(
                  shrinkWrap: true,
                  itemCount: selectedFiles.length,
                  itemBuilder: (context, index) {
                    final file = selectedFiles[index];
                    return ListTile(
                      dense: true,
                      leading: Icon(
                        _getFileIcon(file.name),
                        size: 20,
                      ),
                      title: Text(
                        file.name,
                        style: Theme.of(context).textTheme.bodySmall,
                      ),
                      subtitle: Text(
                        _formatFileSize(file.size),
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          color: Colors.grey,
                        ),
                      ),
                      trailing: IconButton(
                        icon: Icon(MdiIcons.close, size: 16),
                        onPressed: () {
                          setState(() {
                            selectedFiles.removeAt(index);
                          });
                        },
                      ),
                    );
                  },
                ),
              ),
              const SizedBox(height: 16),
              
              // Total size
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.grey.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(8),
                ),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    const Text('Total Size:'),
                    Text(
                      _formatFileSize(_getTotalSize()),
                      style: const TextStyle(fontWeight: FontWeight.bold),
                    ),
                  ],
                ),
              ),
            ],
          ],
        ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.of(context).pop(),
          child: const Text('Cancel'),
        ),
        ElevatedButton(
          onPressed: _canSend() ? _sendFiles : null,
          child: isLoading
              ? const SizedBox(
                  width: 16,
                  height: 16,
                  child: CircularProgressIndicator(strokeWidth: 2),
                )
              : const Text('Send'),
        ),
      ],
    );
  }

  Future<void> _pickFiles() async {
    try {
      final result = await FilePicker.platform.pickFiles(
        allowMultiple: true,
        type: FileType.any,
      );

      if (result != null && result.files.isNotEmpty) {
        setState(() {
          selectedFiles.addAll(result.files);
        });
      }
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Error picking files: $e'),
          backgroundColor: Colors.red,
        ),
      );
    }
  }

  bool _canSend() {
    return selectedPeer != null && 
           selectedFiles.isNotEmpty && 
           !isLoading &&
           selectedPeer!.isOnline;
  }

  Future<void> _sendFiles() async {
    if (!_canSend()) return;

    setState(() {
      isLoading = true;
    });

    try {
      final filePaths = selectedFiles
          .where((file) => file.path != null)
          .map((file) => file.path!)
          .toList();

      await ref.read(warpDeckServiceProvider.notifier).sendFiles(
        selectedPeer!.id,
        filePaths,
      );

      if (mounted) {
        Navigator.of(context).pop();
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Sending ${selectedFiles.length} files to ${selectedPeer!.name}'),
            backgroundColor: Colors.green,
          ),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text('Error sending files: $e'),
            backgroundColor: Colors.red,
          ),
        );
      }
    } finally {
      if (mounted) {
        setState(() {
          isLoading = false;
        });
      }
    }
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

  String _formatFileSize(int? bytes) {
    if (bytes == null) return 'Unknown';
    if (bytes < 1024) return '${bytes}B';
    if (bytes < 1024 * 1024) return '${(bytes / 1024).toStringAsFixed(1)}KB';
    if (bytes < 1024 * 1024 * 1024) return '${(bytes / (1024 * 1024)).toStringAsFixed(1)}MB';
    return '${(bytes / (1024 * 1024 * 1024)).toStringAsFixed(1)}GB';
  }

  int _getTotalSize() {
    return selectedFiles.fold(0, (sum, file) => sum + file.size);
  }
}