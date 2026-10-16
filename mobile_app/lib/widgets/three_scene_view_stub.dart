import 'package:flutter/material.dart';

class ThreeSceneView extends StatelessWidget {
  const ThreeSceneView({
    super.key,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: BoxDecoration(
        color: const Color(0xFFEFF7F1),
        borderRadius: BorderRadius.circular(32),
      ),
      child: const Center(
        child: Text(
          '3D visualization available on web',
          style: TextStyle(
            color: Color(0xFF18201B),
            fontSize: 16,
            fontWeight: FontWeight.w600,
          ),
        ),
      ),
    );
  }
}