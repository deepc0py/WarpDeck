import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'package:warpdeck_gui/screens/dashboard_screen.dart';

void main() {
  testWidgets('WarpDeck app smoke test', (WidgetTester tester) async {
    // Build our app and trigger a frame.
    await tester.pumpWidget(
      const ProviderScope(
        child: MaterialApp(
          home: DashboardScreen(),
        ),
      ),
    );

    // Verify that WarpDeck text appears
    expect(find.text('WarpDeck'), findsOneWidget);
  });
}
