import 'package:flutter/material.dart';
import 'package:material_design_icons_flutter/material_design_icons_flutter.dart';

import '../services/warpdeck_service.dart';

class StatusIndicator extends StatelessWidget {
  final WarpDeckStatus status;
  final String? errorMessage;

  const StatusIndicator({
    super.key,
    required this.status,
    this.errorMessage,
  });

  @override
  Widget build(BuildContext context) {
    final statusWidget = Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
      decoration: BoxDecoration(
        color: _getStatusColor().withOpacity(0.1),
        borderRadius: BorderRadius.circular(16),
        border: Border.all(
          color: _getStatusColor().withOpacity(0.3),
          width: 1,
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
          if (status == WarpDeckStatus.error && errorMessage != null) ...[
            const SizedBox(width: 4),
            Icon(
              MdiIcons.informationOutline,
              size: 16,
              color: _getStatusColor(),
            ),
          ],
        ],
      ),
    );

    // If there's an error with a message, make it clickable
    if (status == WarpDeckStatus.error && errorMessage != null) {
      return InkWell(
        onTap: () => _showErrorDialog(context),
        borderRadius: BorderRadius.circular(16),
        child: statusWidget,
      );
    }

    return statusWidget;
  }

  void _showErrorDialog(BuildContext context) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Row(
          children: [
            Icon(MdiIcons.alertCircle, color: Colors.red),
            const SizedBox(width: 8),
            const Text('Error'),
          ],
        ),
        content: Text(errorMessage ?? 'An unknown error occurred'),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('OK'),
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