import '../models/sensor_data.dart';
import 'app_mode.dart';
import 'esp32_service.dart';

class SensorService {
  // Each simulation state remains visible for 10 seconds.
  // The dashboard can still refresh every 5 seconds.
  static const int _stateDurationSeconds = 10;

  static SensorData getSensorData() {
    if (AppModeController.isEsp32) {
      return const SensorData(
        yfFlowRate: 0,
        zjFlowRate: 0,
        vibration1: 0,
        vibration2: 0,
        leakDetected: false,
      );
    }

    return _simulationData();
  }

  static Future<SensorData?> getCurrentSensorData() async {
    if (AppModeController.isEsp32) {
      return Esp32Service.getSensorData();
    }

    return _simulationData();
  }

  static SensorData _simulationData() {
    final int state =
        (DateTime.now().millisecondsSinceEpoch ~/ 1000) ~/
            _stateDurationSeconds %
        4;

    switch (state) {
      // NORMAL
      case 0:
        return const SensorData(
          yfFlowRate: 2.6,
          zjFlowRate: 2.5,
          vibration1: 1.0,
          vibration2: 1.1,
          leakDetected: false,
          leakZone: null,
        );

      // ZONE 1
      case 1:
        return const SensorData(
          yfFlowRate: 2.9,
          zjFlowRate: 1.6,
          vibration1: 4.5,
          vibration2: 1.0,
          leakDetected: true,
          leakZone: 'Zone 1',
        );

      // ZONE 2
      case 2:
        return const SensorData(
          yfFlowRate: 3.0,
          zjFlowRate: 1.4,
          vibration1: 4.3,
          vibration2: 4.2,
          leakDetected: true,
          leakZone: 'Zone 2',
        );

      // ZONE 3
      case 3:
        return const SensorData(
          yfFlowRate: 2.9,
          zjFlowRate: 1.0,
          vibration1: 1.0,
          vibration2: 4.6,
          leakDetected: true,
          leakZone: 'Zone 3',
        );

      default:
        return const SensorData(
          yfFlowRate: 2.6,
          zjFlowRate: 2.5,
          vibration1: 1.0,
          vibration2: 1.1,
          leakDetected: false,
          leakZone: null,
        );
    }
  }
}