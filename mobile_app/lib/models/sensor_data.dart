class SensorData {
  final double yfFlowRate;
  final double zjFlowRate;
  final double vibration1;
  final double vibration2;
  final bool leakDetected;
  final String? leakZone;

  const SensorData({
    required this.yfFlowRate,
    required this.zjFlowRate,
    required this.vibration1,
    required this.vibration2,
    required this.leakDetected,
    this.leakZone,
  });

  double get flowDifference =>
      yfFlowRate - zjFlowRate;

  SensorData copyWith({
    double? yfFlowRate,
    double? zjFlowRate,
    double? vibration1,
    double? vibration2,
    bool? leakDetected,
    String? leakZone,
    bool clearLeakZone = false,
  }) {
    return SensorData(
      yfFlowRate:
          yfFlowRate ?? this.yfFlowRate,
      zjFlowRate:
          zjFlowRate ?? this.zjFlowRate,
      vibration1:
          vibration1 ?? this.vibration1,
      vibration2:
          vibration2 ?? this.vibration2,
      leakDetected:
          leakDetected ?? this.leakDetected,
      leakZone: clearLeakZone
          ? null
          : (leakZone ?? this.leakZone),
    );
  }
}