import '../models/sensor_data.dart';

class LeakDetectionService {
  // Flow difference is currently used for leak detection.
  // This value will be calibrated after testing the physical prototype.
  static const double flowDifferenceThreshold = 0.5;

  // IMPORTANT:
  // The ESP32 now sends dynamic vibration (gravity removed).
  // Do not lower this threshold blindly. We will calibrate it
  // after observing the real pump + pipe vibration.
  static const double vibrationThreshold = 2.0;

  static SensorData analyze(SensorData raw) {
    final double difference =
        raw.yfFlowRate - raw.zjFlowRate;

    final bool flowAnomaly =
        difference.abs() >= flowDifferenceThreshold;

    final bool v1High =
        raw.vibration1 >= vibrationThreshold;

    final bool v2High =
        raw.vibration2 >= vibrationThreshold;

    // ----------------------------------------------------------
    // NORMAL
    // ----------------------------------------------------------

    if (!flowAnomaly && !v1High && !v2High) {
      return raw.copyWith(
        leakDetected: false,
        clearLeakZone: true,
      );
    }

    // ----------------------------------------------------------
    // VIBRATION-BASED APPROXIMATE LOCATION
    // ----------------------------------------------------------
    //
    // Zone mapping:
    //   V1 only      -> Zone 1
    //   V1 + V2      -> Zone 2
    //   V2 only      -> Zone 3
    //
    // If only a flow imbalance is detected, we DO NOT invent
    // a zone. The UI will show "Location pending".
    //

    String? zone;

    if (v1High && !v2High) {
      zone = 'Zone 1';
    } else if (v1High && v2High) {
      zone = 'Zone 2';
    } else if (!v1High && v2High) {
      zone = 'Zone 3';
    }

    return raw.copyWith(
      leakDetected: true,
      leakZone: zone,
      clearLeakZone: zone == null,
    );
  }
}
