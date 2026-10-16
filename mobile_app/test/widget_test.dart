import 'package:flutter_test/flutter_test.dart';

import 'package:mobile_app/screens/dashboard_screen.dart';

void main() {
  testWidgets('PIPE-SENSE dashboard loads correctly',
      (WidgetTester tester) async {
    await tester.pumpWidget(
      const DashboardScreen(),
    );

    expect(find.text('PIPE-SENSE'), findsOneWidget);
    expect(
      find.text('Hidden Water Pipe Leak Detection'),
      findsOneWidget,
    );
    expect(find.text('System Status'), findsOneWidget);
    expect(find.text('Leak Status'), findsOneWidget);
    expect(find.text('Sensor Readings'), findsOneWidget);
    expect(find.text('Approximate Leak Location'), findsOneWidget);
  });
}
