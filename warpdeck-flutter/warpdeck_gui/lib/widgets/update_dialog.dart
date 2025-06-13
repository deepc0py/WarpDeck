import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:material_design_icons_flutter/material_design_icons_flutter.dart';
import 'package:url_launcher/url_launcher.dart';

import '../services/update_service.dart';

class UpdateDialog extends ConsumerWidget {
  final UpdateInfo updateInfo;

  const UpdateDialog({
    super.key,
    required this.updateInfo,
  });

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final updateState = ref.watch(updateServiceProvider);
    
    return AlertDialog(
      title: Row(
        children: [
          Icon(
            updateInfo.isCritical ? MdiIcons.alertCircle : MdiIcons.download,
            color: updateInfo.isCritical ? Colors.orange : Colors.blue,
          ),
          const SizedBox(width: 12),
          Text(updateInfo.isCritical ? 'Critical Update Available' : 'Update Available'),
        ],
      ),
      content: SizedBox(
        width: 500,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Version info
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Theme.of(context).colorScheme.primary.withOpacity(0.1),
                borderRadius: BorderRadius.circular(8),
              ),
              child: Row(
                children: [
                  Icon(MdiIcons.tagOutline, size: 20),
                  const SizedBox(width: 8),
                  Text(
                    'Version ${updateInfo.version}',
                    style: Theme.of(context).textTheme.titleMedium?.copyWith(
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  const Spacer(),
                  Text(
                    _formatDate(updateInfo.releaseDate),
                    style: Theme.of(context).textTheme.bodySmall?.copyWith(
                      color: Colors.grey,
                    ),
                  ),
                ],
              ),
            ),
            
            const SizedBox(height: 16),
            
            // Release notes
            Text(
              'What\'s New:',
              style: Theme.of(context).textTheme.titleSmall,
            ),
            const SizedBox(height: 8),
            Container(
              constraints: const BoxConstraints(maxHeight: 200),
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                border: Border.all(color: Colors.grey.withOpacity(0.3)),
                borderRadius: BorderRadius.circular(8),
              ),
              child: SingleChildScrollView(
                child: Text(
                  updateInfo.releaseNotes.isNotEmpty 
                      ? updateInfo.releaseNotes 
                      : 'No release notes available.',
                  style: Theme.of(context).textTheme.bodyMedium,
                ),
              ),
            ),
            
            const SizedBox(height: 16),
            
            // Critical update warning
            if (updateInfo.isCritical) ...[
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.orange.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(color: Colors.orange.withOpacity(0.3)),
                ),
                child: Row(
                  children: [
                    Icon(MdiIcons.alert, color: Colors.orange, size: 20),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Text(
                        'This is a critical security update. We strongly recommend updating immediately.',
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          color: Colors.orange[800],
                          fontWeight: FontWeight.w500,
                        ),
                      ),
                    ),
                  ],
                ),
              ),
              const SizedBox(height: 16),
            ],
            
            // Download progress
            if (updateState.status == UpdateStatus.downloading) ...[
              Text(
                'Downloading Update...',
                style: Theme.of(context).textTheme.titleSmall,
              ),
              const SizedBox(height: 8),
              LinearProgressIndicator(
                value: updateState.downloadProgress,
                backgroundColor: Colors.grey.withOpacity(0.2),
              ),
              const SizedBox(height: 8),
              Text(
                '${(updateState.downloadProgress * 100).toInt()}%',
                style: Theme.of(context).textTheme.bodySmall?.copyWith(
                  color: Colors.grey,
                ),
              ),
              const SizedBox(height: 16),
            ],
            
            // Ready to install
            if (updateState.status == UpdateStatus.readyToInstall) ...[
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.green.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(color: Colors.green.withOpacity(0.3)),
                ),
                child: Row(
                  children: [
                    Icon(MdiIcons.checkCircle, color: Colors.green, size: 20),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Text(
                        'Update downloaded successfully. Click Install to complete the update.',
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          color: Colors.green[800],
                          fontWeight: FontWeight.w500,
                        ),
                      ),
                    ),
                  ],
                ),
              ),
              const SizedBox(height: 16),
            ],
          ],
        ),
      ),
      actions: [
        // Later button (for non-critical updates)
        if (!updateInfo.isCritical && updateState.status == UpdateStatus.available)
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Later'),
          ),
        
        // Skip version button
        if (updateState.status == UpdateStatus.available)
          TextButton(
            onPressed: () {
              // In a real app, you'd save this preference
              Navigator.of(context).pop();
            },
            child: const Text('Skip This Version'),
          ),
        
        // Download/Install button
        ElevatedButton(
          onPressed: _getActionButtonCallback(context, ref, updateState),
          child: Row(
            mainAxisSize: MainAxisSize.min,
            children: [
              if (updateState.status == UpdateStatus.downloading)
                const SizedBox(
                  width: 16,
                  height: 16,
                  child: CircularProgressIndicator(strokeWidth: 2),
                )
              else
                Icon(_getActionButtonIcon(updateState.status), size: 16),
              const SizedBox(width: 8),
              Text(_getActionButtonText(updateState.status)),
            ],
          ),
        ),
      ],
    );
  }

  VoidCallback? _getActionButtonCallback(
    BuildContext context, 
    WidgetRef ref, 
    UpdateState updateState
  ) {
    switch (updateState.status) {
      case UpdateStatus.available:
        return () => ref.read(updateServiceProvider.notifier).downloadUpdate();
      case UpdateStatus.downloading:
        return null; // Disable during download
      case UpdateStatus.readyToInstall:
        return () {
          ref.read(updateServiceProvider.notifier).installUpdate();
          Navigator.of(context).pop();
        };
      case UpdateStatus.error:
        return () => _openDownloadPage();
      default:
        return null;
    }
  }

  IconData _getActionButtonIcon(UpdateStatus status) {
    switch (status) {
      case UpdateStatus.available:
        return MdiIcons.download;
      case UpdateStatus.readyToInstall:
        return MdiIcons.installMobile;
      case UpdateStatus.error:
        return MdiIcons.openInNew;
      default:
        return MdiIcons.download;
    }
  }

  String _getActionButtonText(UpdateStatus status) {
    switch (status) {
      case UpdateStatus.available:
        return 'Download';
      case UpdateStatus.downloading:
        return 'Downloading...';
      case UpdateStatus.readyToInstall:
        return 'Install Now';
      case UpdateStatus.error:
        return 'Download Manually';
      default:
        return 'Download';
    }
  }

  void _openDownloadPage() async {
    const url = 'https://github.com/deepc0py/WarpDeck/releases/latest';
    if (await canLaunchUrl(Uri.parse(url))) {
      await launchUrl(Uri.parse(url));
    }
  }

  String _formatDate(DateTime date) {
    final now = DateTime.now();
    final difference = now.difference(date);
    
    if (difference.inDays == 0) {
      return 'Today';
    } else if (difference.inDays == 1) {
      return 'Yesterday';
    } else if (difference.inDays < 7) {
      return '${difference.inDays} days ago';
    } else {
      return '${date.day}/${date.month}/${date.year}';
    }
  }
}

/// Widget to show in the app bar or settings for update notifications
class UpdateBadge extends ConsumerWidget {
  const UpdateBadge({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final updateState = ref.watch(updateServiceProvider);
    
    if (updateState.status != UpdateStatus.available) {
      return const SizedBox.shrink();
    }
    
    return IconButton(
      onPressed: () => _showUpdateDialog(context, updateState.updateInfo!),
      icon: Stack(
        children: [
          Icon(MdiIcons.downloadOutline),
          Positioned(
            right: 0,
            top: 0,
            child: Container(
              width: 8,
              height: 8,
              decoration: BoxDecoration(
                color: updateState.updateInfo?.isCritical == true 
                    ? Colors.orange 
                    : Colors.blue,
                shape: BoxShape.circle,
              ),
            ),
          ),
        ],
      ),
      tooltip: 'Update Available',
    );
  }

  void _showUpdateDialog(BuildContext context, UpdateInfo updateInfo) {
    showDialog(
      context: context,
      barrierDismissible: !updateInfo.isCritical,
      builder: (context) => UpdateDialog(updateInfo: updateInfo),
    );
  }
}