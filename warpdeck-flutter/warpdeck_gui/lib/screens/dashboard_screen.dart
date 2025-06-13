import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:material_design_icons_flutter/material_design_icons_flutter.dart';

import '../services/warpdeck_service.dart';
import '../widgets/status_indicator.dart';
import '../widgets/peer_list_widget.dart';
import '../widgets/transfer_list_widget.dart';
import '../widgets/send_files_button.dart';
import '../screens/settings_screen.dart';

class DashboardScreen extends ConsumerStatefulWidget {
  const DashboardScreen({super.key});

  @override
  ConsumerState<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends ConsumerState<DashboardScreen> {
  int _selectedIndex = 0;

  @override
  void initState() {
    super.initState();
    // Initialize WarpDeck service
    WidgetsBinding.instance.addPostFrameCallback((_) {
      ref.read(warpDeckServiceProvider.notifier).initialize();
    });
  }

  @override
  Widget build(BuildContext context) {
    final warpDeckState = ref.watch(warpDeckServiceProvider);
    
    return Scaffold(
      body: Row(
        children: [
          // Sidebar Navigation
          NavigationRail(
            selectedIndex: _selectedIndex,
            onDestinationSelected: (index) {
              setState(() {
                _selectedIndex = index;
              });
            },
            labelType: NavigationRailLabelType.all,
            destinations: [
              NavigationRailDestination(
                icon: Icon(MdiIcons.viewDashboard),
                selectedIcon: Icon(MdiIcons.viewDashboard),
                label: const Text('Dashboard'),
              ),
              NavigationRailDestination(
                icon: Icon(MdiIcons.accountMultiple),
                selectedIcon: Icon(MdiIcons.accountMultiple),
                label: const Text('Peers'),
              ),
              NavigationRailDestination(
                icon: Icon(MdiIcons.transferRight),
                selectedIcon: Icon(MdiIcons.transferRight),
                label: const Text('Transfers'),
              ),
              NavigationRailDestination(
                icon: Icon(MdiIcons.cog),
                selectedIcon: Icon(MdiIcons.cog),
                label: const Text('Settings'),
              ),
            ],
          ),
          
          const VerticalDivider(thickness: 1, width: 1),
          
          // Main Content Area
          Expanded(
            child: Column(
              children: [
                // Header
                Container(
                  padding: const EdgeInsets.all(24),
                  child: Row(
                    children: [
                      Text(
                        'WarpDeck',
                        style: Theme.of(context).textTheme.headlineLarge,
                      ),
                      const SizedBox(width: 16),
                      StatusIndicator(status: warpDeckState.status),
                      const Spacer(),
                      if (warpDeckState.deviceName != null) ...[
                        Icon(MdiIcons.laptop, size: 20),
                        const SizedBox(width: 8),
                        Text(
                          warpDeckState.deviceName!,
                          style: Theme.of(context).textTheme.titleMedium,
                        ),
                      ],
                      if (warpDeckState.currentPort != null) ...[
                        const SizedBox(width: 16),
                        Icon(MdiIcons.networkPos, size: 20),
                        const SizedBox(width: 8),
                        Text(
                          'Port ${warpDeckState.currentPort}',
                          style: Theme.of(context).textTheme.bodyMedium,
                        ),
                      ],
                    ],
                  ),
                ),
                
                const Divider(height: 1),
                
                // Content
                Expanded(
                  child: _buildContent(),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildContent() {
    switch (_selectedIndex) {
      case 0:
        return _buildDashboard();
      case 1:
        return _buildPeersView();
      case 2:
        return _buildTransfersView();
      case 3:
        return const SettingsScreen();
      default:
        return _buildDashboard();
    }
  }

  Widget _buildDashboard() {
    final warpDeckState = ref.watch(warpDeckServiceProvider);
    
    return Padding(
      padding: const EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Quick Actions
          Row(
            children: [
              Expanded(
                child: _QuickActionCard(
                  title: 'Start Sharing',
                  subtitle: 'Begin discovering peers',
                  icon: MdiIcons.play,
                  color: Colors.green,
                  onTap: warpDeckState.status == WarpDeckStatus.initialized
                      ? () => ref.read(warpDeckServiceProvider.notifier).start()
                      : null,
                ),
              ),
              const SizedBox(width: 16),
              Expanded(
                child: _QuickActionCard(
                  title: 'Send Files',
                  subtitle: 'Share files with peers',
                  icon: MdiIcons.send,
                  color: Colors.blue,
                  onTap: warpDeckState.discoveredPeers.isNotEmpty
                      ? () => _showSendFilesDialog()
                      : null,
                ),
              ),
            ],
          ),
          
          const SizedBox(height: 32),
          
          // Stats Overview
          Row(
            children: [
              Expanded(
                child: _StatsCard(
                  title: 'Discovered Peers',
                  value: '${warpDeckState.discoveredPeers.length}',
                  icon: MdiIcons.accountMultiple,
                  color: Colors.purple,
                ),
              ),
              const SizedBox(width: 16),
              Expanded(
                child: _StatsCard(
                  title: 'Active Transfers',
                  value: '${warpDeckState.activeTransfers.length}',
                  icon: MdiIcons.transferRight,
                  color: Colors.orange,
                ),
              ),
            ],
          ),
          
          const SizedBox(height: 32),
          
          // Recent Activity
          Text(
            'Recent Activity',
            style: Theme.of(context).textTheme.titleLarge,
          ),
          const SizedBox(height: 16),
          Expanded(
            child: Card(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: warpDeckState.activeTransfers.isEmpty
                    ? Center(
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
                              'No recent transfers',
                              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                                color: Colors.grey,
                              ),
                            ),
                          ],
                        ),
                      )
                    : const TransferListWidget(),
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildPeersView() {
    return const Padding(
      padding: EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'Discovered Peers',
            style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
          ),
          SizedBox(height: 16),
          Expanded(child: PeerListWidget()),
        ],
      ),
    );
  }

  Widget _buildTransfersView() {
    return const Padding(
      padding: EdgeInsets.all(24),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(
            'File Transfers',
            style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
          ),
          SizedBox(height: 16),
          Expanded(child: TransferListWidget()),
        ],
      ),
    );
  }

  void _showSendFilesDialog() {
    showDialog(
      context: context,
      builder: (context) => const SendFilesDialog(),
    );
  }
}

class _QuickActionCard extends StatelessWidget {
  final String title;
  final String subtitle;
  final IconData icon;
  final Color color;
  final VoidCallback? onTap;

  const _QuickActionCard({
    required this.title,
    required this.subtitle,
    required this.icon,
    required this.color,
    this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: InkWell(
        onTap: onTap,
        borderRadius: BorderRadius.circular(12),
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Row(
                children: [
                  Container(
                    padding: const EdgeInsets.all(12),
                    decoration: BoxDecoration(
                      color: color.withOpacity(0.1),
                      borderRadius: BorderRadius.circular(8),
                    ),
                    child: Icon(icon, color: color, size: 24),
                  ),
                  const Spacer(),
                  if (onTap != null)
                    Icon(
                      MdiIcons.chevronRight,
                      color: Colors.grey,
                    ),
                ],
              ),
              const SizedBox(height: 16),
              Text(
                title,
                style: Theme.of(context).textTheme.titleLarge,
              ),
              const SizedBox(height: 4),
              Text(
                subtitle,
                style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                  color: Colors.grey,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

class _StatsCard extends StatelessWidget {
  final String title;
  final String value;
  final IconData icon;
  final Color color;

  const _StatsCard({
    required this.title,
    required this.value,
    required this.icon,
    required this.color,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(icon, color: color, size: 24),
                const Spacer(),
                Text(
                  value,
                  style: Theme.of(context).textTheme.headlineMedium?.copyWith(
                    color: color,
                    fontWeight: FontWeight.bold,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 8),
            Text(
              title,
              style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                color: Colors.grey,
              ),
            ),
          ],
        ),
      ),
    );
  }
}