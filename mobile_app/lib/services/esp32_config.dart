class Esp32Config {
  static const String esp32Ip = '192.168.137.7';
  static const String baseUrl = 'http://$esp32Ip';
  static const String dataEndpoint = '/data';

  static String get dataUrl => '$baseUrl$dataEndpoint';
}