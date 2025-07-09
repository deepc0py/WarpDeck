import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:material_design_icons_flutter/material_design_icons_flutter.dart';

import '../services/warpdeck_service.dart';
import '../models/peer.dart';
import 'send_files_button.dart';

class PeerListWidget extends ConsumerWidget {
  const PeerListWidget({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final warpDeckState = ref.watch(warpDeckServiceProvider);
    final peers = warpDeckState.discoveredPeers.values.toList();

    if (peers.isEmpty) {
      return Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              MdiIcons.accountSearch,
              size: 64,
              color: Colors.grey,
            ),
            const SizedBox(height: 16),
            Text(
              'No peers discovered',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                color: Colors.grey,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              warpDeckState.status == WarpDeckStatus.running
                  ? 'Scanning for nearby devices...'
                  : 'Start WarpDeck to discover peers',
              style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                color: Colors.grey,
              ),
            ),
            if (warpDeckState.status != WarpDeckStatus.running) ...[
              const SizedBox(height: 16),
              ElevatedButton.icon(
                onPressed: () => ref.read(warpDeckServiceProvider.notifier).start(),
                icon: Icon(MdiIcons.play),
                label: const Text('Start Discovery'),
              ),
            ],
          ],
        ),
      );
    }

    return ListView.builder(
      itemCount: peers.length,
      itemBuilder: (context, index) {
        final peer = peers[index];
        return PeerCard(peer: peer);
      },
    );
  }
}

class PeerCard extends ConsumerWidget {
  final Peer peer;

  const PeerCard({
    super.key,
    required this.peer,
  });

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          children: [
            // Platform Icon
            Container(
              width: 48,
              height: 48,
              decoration: BoxDecoration(
                color: Theme.of(context).colorScheme.primary.withOpacity(0.1),
                borderRadius: BorderRadius.circular(8),
              ),
              child: Center(
                child: Text(
                  peer.platformIcon,
                  style: const TextStyle(fontSize: 24),
                ),
              ),
            ),
            
            const SizedBox(width: 16),
            
            // Peer Info
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      Expanded(
                        child: Text(
                          peer.name,
                          style: Theme.of(context).textTheme.titleMedium,
                          overflow: TextOverflow.ellipsis,
                        ),
                      ),
                      if (peer.isOnline) ...[
                        Container(
                          width: 8,
                          height: 8,
                          decoration: const BoxDecoration(
                            color: Colors.green,
                            shape: BoxShape.circle,
                          ),
                        ),
                        const SizedBox(width: 8),
                        Text(
                          'Online',
                          style: Theme.of(context).textTheme.bodySmall?.copyWith(
                            color: Colors.green,
                            fontWeight: FontWeight.w500,
                          ),
                        ),
                      ],
                    ],
                  ),
                  const SizedBox(height: 4),
                  Text(
                    'ID: ${peer.shortId}',
                    style: Theme.of(context).textTheme.bodySmall?.copyWith(
                      color: Colors.grey,
                    ),
                  ),
                  Text(
                    '${peer.platform.toUpperCase()} • ${peer.hostAddress}:${peer.port}',
                    style: Theme.of(context).textTheme.bodySmall?.copyWith(
                      color: Colors.grey,
                    ),
                  ),
                  if (peer.lastSeen != null)
                    Text(
                      'Last seen: ${_formatLastSeen(peer.lastSeen!)}',
                      style: Theme.of(context).textTheme.bodySmall?.copyWith(
                        color: Colors.grey,
                      ),
                    ),
                ],
              ),
            ),
            
            const SizedBox(width: 16),
            
            // Actions
            Column(
              children: [
                ElevatedButton.icon(
                  onPressed: peer.isOnline
                      ? () => _showSendFilesDialog(context, ref, peer)
                      : null,
                  icon: Icon(MdiIcons.send, size: 16),
                  label: const Text('Send'),
                  style: ElevatedButton.styleFrom(
                    padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
                    minimumSize: const Size(80, 32),
                  ),
                ),
                const SizedBox(height: 8),
                OutlinedButton.icon(
                  onPressed: () => _showPeerDetails(context, peer),
                  icon: Icon(MdiIcons.informationOutline, size: 16),
                  label: const Text('Info'),
                  style: OutlinedButton.styleFrom(
                    padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
                    minimumSize: const Size(80, 32),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  void _showSendFilesDialog(BuildContext context, WidgetRef ref, Peer peer) {
    showDialog(
      context: context,
      builder: (context) => SendFilesDialog(targetPeer: peer),
    );
  }

  void _showPeerDetails(BuildContext context, Peer peer) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Row(
          children: [
            Text(peer.platformIcon, style: const TextStyle(fontSize: 24)),
            const SizedBox(width: 12),
            Expanded(child: Text(peer.name)),
          ],
        ),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            _DetailRow('Device ID', peer.id),
            _DetailRow('Platform', peer.platform.toUpperCase()),
            _DetailRow('Address', '${peer.hostAddress}:${peer.port}'),
            _DetailRow('Fingerprint', peer.fingerprint.substring(0, 16) + '...'),
            if (peer.lastSeen != null)
              _DetailRow('Last Seen', _formatLastSeen(peer.lastSeen!)),
            _DetailRow('Status', peer.isOnline ? 'Online' : 'Offline'),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Close'),
          ),
        ],
      ),
    );
  }

  String _formatLastSeen(DateTime lastSeen) {
    final now = DateTime.now();
    final difference = now.difference(lastSeen);
    
    if (difference.inMinutes < 1) {
      return 'Just now';
    } else if (difference.inHours < 1) {
      return '${difference.inMinutes}m ago';
    } else if (difference.inDays < 1) {
      return '${difference.inHours}h ago';
    } else {
      return '${difference.inDays}d ago';
    }
  }
}

class _DetailRow extends StatelessWidget {
  final String label;
  final String value;

  const _DetailRow(this.label, this.value);

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 80,
            child: Text(
              '$label:',
              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                fontWeight: FontWeight.w500,
                color: Colors.grey,
              ),
            ),
          ),
          Expanded(
            child: Text(
              value,
              style: Theme.of(context).textTheme.bodySmall,
            ),
          ),
        ],
      ),
    );
  }
}