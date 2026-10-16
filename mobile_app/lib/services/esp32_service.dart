
import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:http/http.dart' as http;

import '../models/sensor_data.dart';
import 'esp32_config.dart';
import 'leak_detection_service.dart';

enum Esp32ConnectionStatus {
  connecting,
  connected,
  disconnected,
}

class Esp32Service {
  static Esp32ConnectionStatus _connectionStatus =
      Esp32ConnectionStatus.disconnected;

  static Esp32ConnectionStatus get connectionStatus =>
      _connectionStatus;

  static bool get isConnected =>
      _connectionStatus == Esp32ConnectionStatus.connected;

  static bool get isConnecting =>
      _connectionStatus == Esp32ConnectionStatus.connecting;

  static const int _failuresBeforeDisconnect = 3;
  static const Duration _requestTimeout = Duration(seconds: 3);

  static int _consecutiveFailures = 0;
  static Future<SensorData?>? _inFlightRequest;

  static Future<SensorData?> getSensorData() {
    final existingRequest = _inFlightRequest;

    if (existingRequest != null) {
      return existingRequest;
    }

    final request = _fetchSensorData();
    _inFlightRequest = request;

    return request.whenComplete(() {
      if (identical(_inFlightRequest, request)) {
        _inFlightRequest = null;
      }
    });
  }

  static Future<SensorData?> _fetchSensorData() async {
    if (_connectionStatus != Esp32ConnectionStatus.connected) {
      _connectionStatus = Esp32ConnectionStatus.connecting;
    }

    try {
      final uri = Uri.parse(Esp32Config.dataUrl).replace(
        queryParameters: {
          't': DateTime.now().millisecondsSinceEpoch.toString(),
        },
      );

      debugPrint('PIPE-SENSE: Requesting ESP32 sensor data: $uri');

      // Do not add custom request headers here.
      // Custom headers can trigger a browser CORS preflight.
      final response = await http
          .get(uri)
          .timeout(_requestTimeout);

      if (response.statusCode != 200) {
        return _recordFailure(
          'HTTP status ${response.statusCode}',
        );
      }

      final dynamic decoded = jsonDecode(response.body);

      if (decoded is! Map<String, dynamic>) {
        return _recordFailure('Invalid JSON response');
      }

      final double? yfFlowRate =
          _tryToDouble(decoded['yfFlowRate']);
      final double? zjFlowRate =
          _tryToDouble(decoded['zjFlowRate']);
      final double? vibration1 =
          _tryToDouble(decoded['vibration1']);
      final double? vibration2 =
          _tryToDouble(decoded['vibration2']);

      if (yfFlowRate == null ||
          zjFlowRate == null ||
          vibration1 == null ||
          vibration2 == null) {
        return _recordFailure(
          'Missing or invalid sensor values',
        );
      }

      final rawData = SensorData(
        yfFlowRate: yfFlowRate,
        zjFlowRate: zjFlowRate,
        vibration1: vibration1,
        vibration2: vibration2,
        leakDetected: false,
        leakZone: null,
      );

      final processedData = LeakDetectionService.analyze(rawData);

      _consecutiveFailures = 0;
      _connectionStatus = Esp32ConnectionStatus.connected;

      debugPrint(
        'PIPE-SENSE: Connected | '
        'YF=${processedData.yfFlowRate.toStringAsFixed(2)} L/min | '
        'ZJ=${processedData.zjFlowRate.toStringAsFixed(2)} L/min | '
        'V1=${processedData.vibration1.toStringAsFixed(2)} | '
        'V2=${processedData.vibration2.toStringAsFixed(2)}',
      );

      return processedData;
    } catch (error) {
      return _recordFailure('Request failed: $error');
    }
  }

  static SensorData? _recordFailure(String reason) {
    _consecutiveFailures++;

    debugPrint(
      'PIPE-SENSE: $reason '
      '($_consecutiveFailures/$_failuresBeforeDisconnect failures)',
    );

    if (_consecutiveFailures >= _failuresBeforeDisconnect) {
      _connectionStatus = Esp32ConnectionStatus.disconnected;
    } else if (_connectionStatus !=
        Esp32ConnectionStatus.connected) {
      _connectionStatus = Esp32ConnectionStatus.connecting;
    }

    return null;
  }

  static double? _tryToDouble(dynamic value) {
    if (value is num) {
      final result = value.toDouble();
      return result.isFinite ? result : null;
    }

    if (value is String) {
      final result = double.tryParse(value);

      if (result != null && result.isFinite) {
        return result;
      }
    }

    return null;
  }
}
