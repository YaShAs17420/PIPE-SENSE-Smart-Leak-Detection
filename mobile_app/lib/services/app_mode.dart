enum AppMode {
  simulation,
  esp32,
}

class AppModeController {
  static AppMode currentMode = AppMode.simulation;

  static bool get isSimulation =>
      currentMode == AppMode.simulation;

  static bool get isEsp32 =>
      currentMode == AppMode.esp32;

  static void setMode(AppMode mode) {
    currentMode = mode;
  }
}