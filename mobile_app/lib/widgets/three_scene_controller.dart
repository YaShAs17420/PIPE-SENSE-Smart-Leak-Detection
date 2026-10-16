import 'dart:js_interop';

@JS('pipeSense3D.updateSensorState')
external void _updateSensorState(
  double yfFlowRate,
  double zjFlowRate,
  double vibration1,
  double vibration2,
  bool leakDetected,
  String? leakZone,
);

@JS('pipeSense3D.selectZone')
external void _selectZone(
  String zone,
);

@JS('pipeSense3D.setTheme')
external void _setTheme(
  String theme,
);

class ThreeSceneController {
  static void updateSensorState({
    required double yfFlowRate,
    required double zjFlowRate,
    required double vibration1,
    required double vibration2,
    required bool leakDetected,
    String? leakZone,
  }) {
    try {
      _updateSensorState(
        yfFlowRate,
        zjFlowRate,
        vibration1,
        vibration2,
        leakDetected,
        leakZone,
      );
    } catch (_) {}
  }

  static void selectZone(
    String zone,
  ) {
    try {
      _selectZone(zone);
    } catch (_) {}
  }

  static void setTheme({
    required bool dark,
  }) {
    try {
      _setTheme(
        dark ? 'dark' : 'light',
      );
    } catch (_) {}
  }
}