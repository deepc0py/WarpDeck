import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  testWidgets('WarpDeck app smoke test', (WidgetTester tester) async {
    // Build a simple app widget to test Flutter framework works
    await tester.pumpWidget(
      const MaterialApp(
        home: Scaffold(
          body: Center(
            child: Text('WarpDeck'),
          ),
        ),
      ),
    );

    // Verify that WarpDeck text appears
    expect(find.text('WarpDeck'), findsOneWidget);
  });
}
