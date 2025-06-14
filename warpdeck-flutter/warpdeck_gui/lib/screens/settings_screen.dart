import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:material_design_icons_flutter/material_design_icons_flutter.dart';

import '../services/warpdeck_service.dart';

class SettingsScreen extends ConsumerStatefulWidget {
  const SettingsScreen({super.key});

  @override
  ConsumerState<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends ConsumerState<SettingsScreen> {
  final _deviceNameController = TextEditingController();
  bool _isEditing = false;

  @override
  void initState() {
    super.initState();
    final state = ref.read(warpDeckServiceProvider);
    _deviceNameController.text = state.deviceName ?? '';
  }

  @override
  void dispose() {
    _deviceNameController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final warpDeckState = ref.watch(warpDeckServiceProvider);

    return SingleChildScrollView(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'Settings',
            style: Theme.of(context).textTheme.headlineMedium,
          ),
          const SizedBox(height: 32),

          // Device Settings Section
          _SettingsSection(
            title: 'Device Settings',
            icon: MdiIcons.laptop,
            children: [
              _SettingsTile(
                title: 'Device Name',
                subtitle: 'How this device appears to others',
                trailing: _isEditing
                    ? SizedBox(
                        width: 280,
                        child: Row(
                          mainAxisSize: MainAxisSize.min,
                          children: [
                            Expanded(
                              child: TextField(
                                controller: _deviceNameController,
                                decoration: const InputDecoration(
                                  isDense: true,
                                  contentPadding: EdgeInsets.symmetric(
                                    horizontal: 12,
                                    vertical: 8,
                                  ),
                                ),
                                onSubmitted: _saveDeviceName,
                              ),
                            ),
                            const SizedBox(width: 8),
                            IconButton(
                              icon: Icon(MdiIcons.check, color: Colors.green),
                              onPressed: () => _saveDeviceName(),
                              padding: EdgeInsets.zero,
                              constraints: const BoxConstraints(),
                            ),
                            IconButton(
                              icon: Icon(MdiIcons.close, color: Colors.red),
                              onPressed: _cancelEdit,
                              padding: EdgeInsets.zero,
                              constraints: const BoxConstraints(),
                            ),
                          ],
                        ),
                      )
                    : Row(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          Text(
                            warpDeckState.deviceName ?? 'Not set',
                            style: Theme.of(context).textTheme.bodyMedium,
                          ),
                          const SizedBox(width: 8),
                          IconButton(
                            icon: Icon(MdiIcons.pencil, size: 16),
                            onPressed: _startEdit,
                            padding: EdgeInsets.zero,
                            constraints: const BoxConstraints(),
                          ),
                        ],
                      ),
              ),
              
              _SettingsTile(
                title: 'Status',
                subtitle: 'Current service status',
                trailing: _StatusDisplay(status: warpDeckState.status),
              ),
              
              if (warpDeckState.currentPort != null)
                _SettingsTile(
                  title: 'Network Port',
                  subtitle: 'Port used for file sharing',
                  trailing: Text(
                    '${warpDeckState.currentPort}',
                    style: Theme.of(context).textTheme.bodyMedium,
                  ),
                ),
            ],
          ),

          const SizedBox(height: 32),

          // Service Control Section
          _SettingsSection(
            title: 'Service Control',
            icon: MdiIcons.cogs,
            children: [
              _SettingsTile(
                title: warpDeckState.status == WarpDeckStatus.running
                    ? 'Stop WarpDeck'
                    : 'Start WarpDeck',
                subtitle: warpDeckState.status == WarpDeckStatus.running
                    ? 'Stop file sharing service'
                    : 'Start file sharing service',
                trailing: ElevatedButton.icon(
                  onPressed: _toggleService,
                  icon: Icon(
                    warpDeckState.status == WarpDeckStatus.running
                        ? MdiIcons.stop
                        : MdiIcons.play,
                  ),
                  label: Text(
                    warpDeckState.status == WarpDeckStatus.running
                        ? 'Stop'
                        : 'Start',
                  ),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: warpDeckState.status == WarpDeckStatus.running
                        ? Colors.red
                        : Colors.green,
                    foregroundColor: Colors.white,
                  ),
                ),
              ),
            ],
          ),

          const SizedBox(height: 32),

          // Statistics Section
          _SettingsSection(
            title: 'Statistics',
            icon: MdiIcons.chartLine,
            children: [
              _SettingsTile(
                title: 'Discovered Peers',
                subtitle: 'Number of devices found',
                trailing: Text(
                  '${warpDeckState.discoveredPeers.length}',
                  style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                    color: Theme.of(context).colorScheme.primary,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ),
              
              _SettingsTile(
                title: 'Active Transfers',
                subtitle: 'Current file transfers',
                trailing: Text(
                  '${warpDeckState.activeTransfers.length}',
                  style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                    color: Theme.of(context).colorScheme.secondary,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ),
            ],
          ),

          const SizedBox(height: 32),

          // About Section
          _SettingsSection(
            title: 'About',
            icon: MdiIcons.information,
            children: [
              _SettingsTile(
                title: 'WarpDeck',
                subtitle: 'Cross-platform peer-to-peer file sharing',
                trailing: const Text('v1.0.0'),
              ),
              
              _SettingsTile(
                title: 'GitHub Repository',
                subtitle: 'View source code and report issues',
                trailing: Icon(MdiIcons.openInNew, size: 16),
                onTap: () {
                  // TODO: Open GitHub repository
                },
              ),
            ],
          ),

          // Error display
          if (warpDeckState.errorMessage != null) ...[
            const SizedBox(height: 32),
            Container(
              padding: const EdgeInsets.all(16),
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
                  ),
                  const SizedBox(width: 12),
                  Expanded(
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          'Error',
                          style: Theme.of(context).textTheme.titleSmall?.copyWith(
                            color: Colors.red,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                        Text(
                          warpDeckState.errorMessage!,
                          style: Theme.of(context).textTheme.bodySmall?.copyWith(
                            color: Colors.red,
                          ),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),
          ],
        ],
      ),
    );
  }

  void _startEdit() {
    setState(() {
      _isEditing = true;
      final state = ref.read(warpDeckServiceProvider);
      _deviceNameController.text = state.deviceName ?? '';
    });
  }

  void _cancelEdit() {
    setState(() {
      _isEditing = false;
      final state = ref.read(warpDeckServiceProvider);
      _deviceNameController.text = state.deviceName ?? '';
    });
  }

  void _saveDeviceName([String? value]) async {
    final newName = _deviceNameController.text.trim();
    if (newName.isNotEmpty) {
      await ref.read(warpDeckServiceProvider.notifier).setDeviceName(newName);
      setState(() {
        _isEditing = false;
      });
      
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Device name updated to "$newName"'),
          backgroundColor: Colors.green,
        ),
      );
    }
  }

  void _toggleService() async {
    final state = ref.read(warpDeckServiceProvider);
    
    if (state.status == WarpDeckStatus.running) {
      await ref.read(warpDeckServiceProvider.notifier).stop();
    } else {
      await ref.read(warpDeckServiceProvider.notifier).start();
    }
  }
}

class _SettingsSection extends StatelessWidget {
  final String title;
  final IconData icon;
  final List<Widget> children;

  const _SettingsSection({
    required this.title,
    required this.icon,
    required this.children,
  });

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            Icon(icon, size: 20),
            const SizedBox(width: 8),
            Text(
              title,
              style: Theme.of(context).textTheme.titleLarge,
            ),
          ],
        ),
        const SizedBox(height: 16),
        Card(
          child: Column(
            children: children,
          ),
        ),
      ],
    );
  }
}

class _SettingsTile extends StatelessWidget {
  final String title;
  final String subtitle;
  final Widget? trailing;
  final VoidCallback? onTap;

  const _SettingsTile({
    required this.title,
    required this.subtitle,
    this.trailing,
    this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return InkWell(
      onTap: onTap,
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
        child: Row(
          children: [
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                mainAxisSize: MainAxisSize.min,
                children: [
                  Text(
                    title,
                    style: Theme.of(context).textTheme.titleMedium,
                  ),
                  const SizedBox(height: 4),
                  Text(
                    subtitle,
                    style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                      color: Theme.of(context).colorScheme.onSurfaceVariant,
                    ),
                  ),
                ],
              ),
            ),
            if (trailing != null) ...[
              const SizedBox(width: 16),
              trailing!,
            ],
          ],
        ),
      ),
    );
  }
}

class _StatusDisplay extends StatelessWidget {
  final WarpDeckStatus status;

  const _StatusDisplay({required this.status});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
      decoration: BoxDecoration(
        color: _getStatusColor().withOpacity(0.1),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: _getStatusColor().withOpacity(0.3),
        ),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 8,
            height: 8,
            decoration: BoxDecoration(
              color: _getStatusColor(),
              shape: BoxShape.circle,
            ),
          ),
          const SizedBox(width: 8),
          Text(
            _getStatusText(),
            style: Theme.of(context).textTheme.bodySmall?.copyWith(
              color: _getStatusColor(),
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ),
    );
  }

  Color _getStatusColor() {
    switch (status) {
      case WarpDeckStatus.uninitialized:
        return Colors.grey;
      case WarpDeckStatus.initialized:
        return Colors.orange;
      case WarpDeckStatus.running:
        return Colors.green;
      case WarpDeckStatus.error:
        return Colors.red;
    }
  }

  String _getStatusText() {
    switch (status) {
      case WarpDeckStatus.uninitialized:
        return 'Not Started';
      case WarpDeckStatus.initialized:
        return 'Ready';
      case WarpDeckStatus.running:
        return 'Running';
      case WarpDeckStatus.error:
        return 'Error';
    }
  }
}