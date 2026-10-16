import 'package:flutter/material.dart';

import 'screens/dashboard_screen.dart';

void main() {
  runApp(
    const PipeSenseApp(),
  );
}

class PipeSenseApp
    extends StatelessWidget {
  const PipeSenseApp({
    super.key,
  });

  @override
  Widget build(
    BuildContext context,
  ) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'PIPE-SENSE',

      theme: ThemeData(
        useMaterial3: true,

        fontFamily: 'Arial',

        colorScheme:
            ColorScheme.fromSeed(
          seedColor:
              const Color(
            0xFF55D99A,
          ),
          brightness:
              Brightness.light,
        ),

        scaffoldBackgroundColor:
            const Color(
          0xFFF2F8F4,
        ),
      ),

      home:
          const DashboardScreen(),
    );
  }
}