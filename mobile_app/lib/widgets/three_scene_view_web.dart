import 'dart:ui_web' as ui_web;

import 'package:flutter/material.dart';
import 'package:web/web.dart' as web;

class ThreeSceneView extends StatelessWidget {
  const ThreeSceneView({
    super.key,
  });

  static const String _viewType = 'pipe-sense-three-scene';

  static bool _registered = false;

  static void _registerViewFactory() {
    if (_registered) {
      return;
    }

    ui_web.platformViewRegistry.registerViewFactory(
      _viewType,
      (int viewId) {
        final web.HTMLDivElement container =
            web.HTMLDivElement();

        container.id = 'pipe-sense-three-container';

        container.style.width = '100%';
        container.style.height = '100%';
        container.style.overflow = 'hidden';
        container.style.backgroundColor = 'transparent';
        container.style.borderRadius = '32px';

        return container;
      },
    );

    _registered = true;
  }

  @override
  Widget build(BuildContext context) {
    _registerViewFactory();

    return const HtmlElementView(
      viewType: _viewType,
    );
  }
}