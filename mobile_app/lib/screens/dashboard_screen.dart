
import 'dart:async';
import 'package:flutter/material.dart';
import '../models/sensor_data.dart';
import '../services/app_mode.dart';
import '../services/leak_detection_service.dart';
import '../services/sensor_service.dart';
import '../services/esp32_service.dart';
import '../widgets/three_scene_controller.dart';
import '../widgets/three_scene_view.dart';

class DashboardScreen extends StatefulWidget {
  const DashboardScreen({
    super.key,
  });

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  int selectedTab = 0;
  bool isDark = false;
  Timer? sensorTimer;

  // ESP32 connection state
  bool esp32Connected = false;
  bool _refreshInProgress = false;

  SensorData sensorData = const SensorData(
    yfFlowRate: 2.6,
    zjFlowRate: 2.5,
    vibration1: 1.0,
    vibration2: 1.1,
    leakDetected: false,
  );

  @override
  void initState() {
    super.initState();

    // Render the dashboard before starting sensor polling.
    WidgetsBinding.instance.addPostFrameCallback((_) {
      _refreshSensors();
    });

    // Poll every two seconds.
    sensorTimer = Timer.periodic(
      const Duration(seconds: 2),
      (_) => _refreshSensors(),
    );
  }

  @override
  void dispose() {
    sensorTimer?.cancel();
    super.dispose();
  }

  // SENSOR REFRESH

  Future<void> _refreshSensors() async {
    if (_refreshInProgress || !mounted) return;

    _refreshInProgress = true;
    final bool isEsp32Mode = AppModeController.isEsp32;

    try {
      final SensorData? data =
          await SensorService.getCurrentSensorData();

      if (!mounted) return;

      // Ignore data from a mode that is no longer selected.
      if (isEsp32Mode != AppModeController.isEsp32) return;

      if (isEsp32Mode && data == null) {
        // Esp32Service tracks consecutive failures.
        // Keep the last successful sensor readings.
        final bool connected = Esp32Service.isConnected;

        if (esp32Connected != connected) {
          setState(() {
            esp32Connected = connected;
          });
        }
        return;
      }

      if (data == null) return;

      final SensorData result = LeakDetectionService.analyze(data);

      setState(() {
        esp32Connected = isEsp32Mode
            ? Esp32Service.isConnected
            : false;
        sensorData = result;
      });

      ThreeSceneController.updateSensorState(
        yfFlowRate: result.yfFlowRate,
        zjFlowRate: result.zjFlowRate,
        vibration1: result.vibration1,
        vibration2: result.vibration2,
        leakDetected: result.leakDetected,
        leakZone: result.leakZone,
      );

      ThreeSceneController.setTheme(dark: isDark);
    } catch (error, stackTrace) {
      debugPrint('PIPE-SENSE dashboard refresh error: $error');
      debugPrintStack(stackTrace: stackTrace);

      // Never clear valid readings because of an unexpected exception.
      if (mounted && isEsp32Mode && AppModeController.isEsp32) {
        final bool connected = Esp32Service.isConnected;

        if (esp32Connected != connected) {
          setState(() {
            esp32Connected = connected;
          });
        }
      }
    } finally {
      _refreshInProgress = false;
    }
  }

  // THEME

  void _toggleTheme() {
    setState(() {
      isDark = !isDark;
    });

    ThreeSceneController.setTheme(dark: isDark);
  }

  Color get backgroundColor {
    return isDark
        ? const Color(0xFF07110D)
        : const Color(0xFFF2F8F5);
  }

  Color get cardColor {
    return isDark
        ? const Color(0xFF0D1B15)
        : Colors.white;
  }

  Color get borderColor {
    return isDark
        ? const Color(0xFF244438)
        : const Color(0xFFD2E4DD);
  }

  Color get primaryText {
    return isDark
        ? Colors.white
        : const Color(0xFF10231C);
  }

  Color get secondaryText {
    return isDark
        ? const Color(0xFF91A79E)
        : const Color(0xFF687B73);
  }

  Color get accentColor {
    return isDark
        ? const Color(0xFF63DFA8)
        : const Color(0xFF179B72);
  }

  Color get waterColor {
    return isDark
        ? const Color(0xFF27C7EE)
        : const Color(0xFF159FC9);
  }

  Color get dangerColor {
    return isDark
        ? const Color(0xFFFF596A)
        : const Color(0xFFE9475B);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: backgroundColor,
      body: SafeArea(
        child: _buildCurrentPage(),
      ),
      bottomNavigationBar: _buildBottomNavigation(),
    );
  }

  Widget _buildCurrentPage() {
    if (selectedTab == 1) {
      return _buildMonitorPage();
    }

    if (selectedTab == 2) {
      return _buildZonesPage();
    }

    if (selectedTab == 3) {
      return _buildSettingsPage();
    }

    return _buildHomePage();
  }

  // HOME PAGE

  Widget _buildHomePage() {
    return LayoutBuilder(
      builder: (
        BuildContext context,
        BoxConstraints constraints,
      ) {
        final bool desktop = constraints.maxWidth >= 900;
        final double horizontalPadding = desktop ? 32 : 18;

        return SingleChildScrollView(
          padding: EdgeInsets.fromLTRB(
            horizontalPadding,
            18,
            horizontalPadding,
            28,
          ),
          child: Center(
            child: ConstrainedBox(
              constraints: const BoxConstraints(maxWidth: 1500),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  _buildHeader(),
                  const SizedBox(height: 18),
                  _buildStatusCard(),
                  const SizedBox(height: 18),
                  _build3DCard(desktop),
                  const SizedBox(height: 18),
                  _buildMetrics(desktop),
                  const SizedBox(height: 18),
                  _buildFlowBalance(),
                  const SizedBox(height: 18),
                  _buildZonesPreview(),
                ],
              ),
            ),
          ),
        );
      },
    );
  }

  Widget _buildHeader() {
    return LayoutBuilder(
      builder: (context, constraints) {
        final bool compact = constraints.maxWidth < 360;

        return Row(
          children: [
            Container(
              width: compact ? 46 : 54,
              height: compact ? 46 : 54,
              decoration: BoxDecoration(
                color: accentColor.withValues(alpha: 0.14),
                borderRadius: BorderRadius.circular(
                  compact ? 14 : 17,
                ),
              ),
              child: Icon(
                Icons.water_drop_rounded,
                color: accentColor,
                size: compact ? 24 : 29,
              ),
            ),
            SizedBox(width: compact ? 9 : 14),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    'PIPE-SENSE',
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(
                      color: primaryText,
                      fontSize: compact ? 22 : 27,
                      fontWeight: FontWeight.w800,
                      letterSpacing: -0.7,
                    ),
                  ),
                  const SizedBox(height: 3),
                  Text(
                    'SMART WATER MONITORING',
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(
                      color: secondaryText,
                      fontSize: 8,
                      fontWeight: FontWeight.w700,
                      letterSpacing: 1.0,
                    ),
                  ),
                ],
              ),
            ),
            if (!compact) ...[
              const SizedBox(width: 8),
              _buildConnectionBadge(),
            ],
            SizedBox(width: compact ? 6 : 8),
            _buildThemeButton(),
          ],
        );
      },
    );
  }

  // CONNECTION BADGE

  Widget _buildConnectionBadge() {
    final bool simulation = AppModeController.isSimulation;
    final bool connected =
        AppModeController.isEsp32 && esp32Connected;
    final bool connecting =
        AppModeController.isEsp32 &&
        !connected &&
        Esp32Service.isConnecting;

    final Color statusColor = simulation || connected
        ? accentColor
        : connecting
            ? Colors.orange
            : dangerColor;

    final String statusText = simulation
        ? 'SIMULATION'
        : connected
            ? 'ESP32 CONNECTED'
            : connecting
                ? 'CONNECTING…'
                : 'ESP32 OFFLINE';

    return Container(
      padding: const EdgeInsets.symmetric(
        horizontal: 10,
        vertical: 8,
      ),
      decoration: BoxDecoration(
        color: statusColor.withValues(alpha: 0.10),
        borderRadius: BorderRadius.circular(18),
        border: Border.all(
          color: statusColor.withValues(alpha: 0.35),
        ),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 7,
            height: 7,
            decoration: BoxDecoration(
              color: statusColor,
              shape: BoxShape.circle,
            ),
          ),
          const SizedBox(width: 6),
          Text(
            statusText,
            style: TextStyle(
              color: statusColor,
              fontSize: 8,
              fontWeight: FontWeight.w800,
              letterSpacing: 0.4,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildThemeButton() {
    return GestureDetector(
      onTap: _toggleTheme,
      child: Container(
        width: 52,
        height: 52,
        decoration: BoxDecoration(
          color: cardColor,
          borderRadius: BorderRadius.circular(17),
          border: Border.all(color: borderColor),
        ),
        child: Icon(
          isDark
              ? Icons.light_mode_rounded
              : Icons.dark_mode_rounded,
          color: accentColor,
          size: 22,
        ),
      ),
    );
  }

  // SYSTEM STATUS

  Widget _buildStatusCard() {
    final bool connecting =
        AppModeController.isEsp32 &&
        !esp32Connected &&
        Esp32Service.isConnecting;

    final bool offline =
        AppModeController.isEsp32 &&
        !esp32Connected &&
        !connecting;

    final bool leak =
        !offline && !connecting && sensorData.leakDetected;

    final Color statusColor = offline || leak
        ? dangerColor
        : connecting
            ? Colors.orange
            : accentColor;

    final String title = offline
        ? 'ESP32 disconnected'
        : connecting
            ? 'Connecting to ESP32…'
            : leak
                ? 'Leak detected'
                : 'System normal';

    final String subtitle = offline
        ? 'Unable to reach the ESP32. Check its power and Wi-Fi connection. Last readings are retained.'
        : connecting
            ? 'Waiting for live sensor data. Your dashboard remains available.'
            : leak
                ? (sensorData.leakZone != null
                    ? '${sensorData.leakZone} requires attention'
                    : 'Leak detected — location not determined yet')
                : AppModeController.isSimulation
                    ? 'Simulation readings are active'
                    : 'Pipeline flow and vibration are within normal range';

    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: (offline || leak)
            ? dangerColor.withValues(alpha: 0.07)
            : cardColor,
        borderRadius: BorderRadius.circular(25),
        border: Border.all(
          color: (offline || leak)
              ? dangerColor.withValues(alpha: 0.55)
              : borderColor,
        ),
      ),
      child: Row(
        children: [
          Container(
            width: 54,
            height: 54,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: statusColor.withValues(alpha: 0.12),
            ),
            child: Icon(
              offline
                  ? Icons.wifi_off_rounded
                  : connecting
                      ? Icons.sync_rounded
                      : leak
                          ? Icons.warning_amber_rounded
                          : Icons.verified_rounded,
              color: statusColor,
              size: 27,
            ),
          ),
          const SizedBox(width: 14),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  title,
                  style: TextStyle(
                    color: primaryText,
                    fontSize: 18,
                    fontWeight: FontWeight.w800,
                  ),
                ),
                const SizedBox(height: 5),
                Text(
                  subtitle,
                  maxLines: 3,
                  overflow: TextOverflow.ellipsis,
                  style: TextStyle(
                    color: secondaryText,
                    fontSize: 11,
                    height: 1.35,
                  ),
                ),
              ],
            ),
          ),
          if (leak) ...[
            const SizedBox(width: 8),
            Container(
              padding: const EdgeInsets.symmetric(
                horizontal: 9,
                vertical: 7,
              ),
              decoration: BoxDecoration(
                color: dangerColor.withValues(alpha: 0.12),
                borderRadius: BorderRadius.circular(18),
              ),
              child: Text(
                sensorData.leakZone ?? 'LOCATION PENDING',
                style: TextStyle(
                  color: dangerColor,
                  fontSize: 9,
                  fontWeight: FontWeight.w800,
                ),
              ),
            ),
          ],
        ],
      ),
    );
  }

  // THREE.JS MODEL

  Widget _build3DCard(bool desktop) {
    return Container(
      width: double.infinity,
      height: desktop ? 470 : 390,
      clipBehavior: Clip.antiAlias,
      decoration: BoxDecoration(
        color: cardColor,
        borderRadius: BorderRadius.circular(28),
        border: Border.all(color: borderColor),
      ),
      child: Stack(
        fit: StackFit.expand,
        children: [
          const ThreeSceneView(),
          Positioned(
            top: 15,
            left: 15,
            child: _badge(
              Icons.view_in_ar_rounded,
              'LIVE 3D MODEL',
            ),
          ),
          Positioned(
            top: 15,
            right: 15,
            child: _badge(
              Icons.touch_app_rounded,
              'Drag to rotate',
            ),
          ),
          if (sensorData.leakDetected)
            Positioned(
              left: 15,
              right: 15,
              bottom: 15,
              child: _leakBanner(),
            ),
        ],
      ),
    );
  }

  Widget _badge(IconData icon, String text) {
    return Container(
      padding: const EdgeInsets.symmetric(
        horizontal: 10,
        vertical: 8,
      ),
      decoration: BoxDecoration(
        color: cardColor.withValues(alpha: 0.90),
        borderRadius: BorderRadius.circular(18),
        border: Border.all(color: borderColor),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(
            icon,
            size: 13,
            color: accentColor,
          ),
          const SizedBox(width: 6),
          Text(
            text,
            style: TextStyle(
              color: primaryText,
              fontSize: 9,
              fontWeight: FontWeight.w700,
            ),
          ),
        ],
      ),
    );
  }

  Widget _leakBanner() {
    return Container(
      height: 48,
      padding: const EdgeInsets.symmetric(horizontal: 16),
      decoration: BoxDecoration(
        color: dangerColor,
        borderRadius: BorderRadius.circular(25),
      ),
      child: Row(
        children: [
          const Icon(
            Icons.water_drop_rounded,
            color: Colors.white,
            size: 17,
          ),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              sensorData.leakZone != null
                  ? '${sensorData.leakZone} active'
                  : 'Leak detected — location pending',
              style: const TextStyle(
                color: Colors.white,
                fontSize: 11,
                fontWeight: FontWeight.w800,
              ),
            ),
          ),
          const Text(
            'APPROX.',
            style: TextStyle(
              color: Colors.white,
              fontSize: 8,
              fontWeight: FontWeight.w700,
            ),
          ),
        ],
      ),
    );
  }

  // SENSOR METRICS

  Widget _buildMetrics(bool desktop) {
    if (desktop) {
      return Row(
        children: [
          Expanded(
            child: _metricCard(
              Icons.water_drop_rounded,
              'YF-S201',
              sensorData.yfFlowRate,
              'L/min',
              waterColor,
            ),
          ),
          const SizedBox(width: 12),
          Expanded(
            child: _metricCard(
              Icons.waterfall_chart_rounded,
              'ZJ-S201',
              sensorData.zjFlowRate,
              'L/min',
              accentColor,
            ),
          ),
          const SizedBox(width: 12),
          Expanded(
            child: _metricCard(
              Icons.vibration_rounded,
              'Vibration 1',
              sensorData.vibration1,
              'level',
              accentColor,
            ),
          ),
          const SizedBox(width: 12),
          Expanded(
            child: _metricCard(
              Icons.graphic_eq_rounded,
              'Vibration 2',
              sensorData.vibration2,
              'level',
              waterColor,
            ),
          ),
        ],
      );
    }

    return Column(
      children: [
        Row(
          children: [
            Expanded(
              child: _metricCard(
                Icons.water_drop_rounded,
                'YF-S201',
                sensorData.yfFlowRate,
                'L/min',
                waterColor,
              ),
            ),
            const SizedBox(width: 12),
            Expanded(
              child: _metricCard(
                Icons.waterfall_chart_rounded,
                'ZJ-S201',
                sensorData.zjFlowRate,
                'L/min',
                accentColor,
              ),
            ),
          ],
        ),
        const SizedBox(height: 12),
        Row(
          children: [
            Expanded(
              child: _metricCard(
                Icons.vibration_rounded,
                'Vibration 1',
                sensorData.vibration1,
                'level',
                accentColor,
              ),
            ),
            const SizedBox(width: 12),
            Expanded(
              child: _metricCard(
                Icons.graphic_eq_rounded,
                'Vibration 2',
                sensorData.vibration2,
                'level',
                waterColor,
              ),
            ),
          ],
        ),
      ],
    );
  }

  Widget _metricCard(
    IconData icon,
    String title,
    double value,
    String unit,
    Color color,
  ) {
    return Container(
      height: 145,
      padding: const EdgeInsets.all(15),
      decoration: BoxDecoration(
        color: cardColor,
        borderRadius: BorderRadius.circular(21),
        border: Border.all(color: borderColor),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Container(
                width: 34,
                height: 34,
                decoration: BoxDecoration(
                  color: color.withValues(alpha: 0.12),
                  borderRadius: BorderRadius.circular(10),
                ),
                child: Icon(
                  icon,
                  color: color,
                  size: 18,
                ),
              ),
              const Spacer(),
              Container(
                width: 6,
                height: 6,
                decoration: BoxDecoration(
                  color: color,
                  shape: BoxShape.circle,
                ),
              ),
            ],
          ),
          const Spacer(),
          Text(
            title,
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
            style: TextStyle(
              color: secondaryText,
              fontSize: 10,
              fontWeight: FontWeight.w600,
            ),
          ),
          const SizedBox(height: 4),
          SizedBox(
            width: double.infinity,
            child: FittedBox(
              fit: BoxFit.scaleDown,
              alignment: Alignment.centerLeft,
              child: Row(
                mainAxisSize: MainAxisSize.min,
                crossAxisAlignment: CrossAxisAlignment.end,
                children: [
                  Text(
                    value.toStringAsFixed(1),
                    style: TextStyle(
                      color: primaryText,
                      fontSize: 23,
                      fontWeight: FontWeight.w800,
                    ),
                  ),
                  const SizedBox(width: 4),
                  Text(
                    unit,
                    style: TextStyle(
                      color: secondaryText,
                      fontSize: 8,
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  // FLOW BALANCE

  Widget _buildFlowBalance() {
    final double difference =
        sensorData.yfFlowRate - sensorData.zjFlowRate;
    final bool anomaly = difference.abs() >= 0.5;

    double ratio = 0;

    if (sensorData.yfFlowRate > 0) {
      ratio = sensorData.zjFlowRate / sensorData.yfFlowRate;
    }

    ratio = ratio.clamp(0.0, 1.0);

    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: cardColor,
        borderRadius: BorderRadius.circular(23),
        border: Border.all(color: borderColor),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Expanded(
                child: Text(
                  'Flow balance',
                  style: TextStyle(
                    color: primaryText,
                    fontSize: 16,
                    fontWeight: FontWeight.w800,
                  ),
                ),
              ),
              Text(
                anomaly ? 'ANOMALY' : 'BALANCED',
                style: TextStyle(
                  color: anomaly ? dangerColor : accentColor,
                  fontSize: 9,
                  fontWeight: FontWeight.w800,
                ),
              ),
            ],
          ),
          const SizedBox(height: 15),
          ClipRRect(
            borderRadius: BorderRadius.circular(10),
            child: LinearProgressIndicator(
              minHeight: 8,
              value: ratio,
              backgroundColor: borderColor,
              valueColor: AlwaysStoppedAnimation<Color>(
                anomaly ? dangerColor : accentColor,
              ),
            ),
          ),
          const SizedBox(height: 9),
          Row(
            children: [
              Expanded(
                child: Text(
                  'IN  ${sensorData.yfFlowRate.toStringAsFixed(1)} L/min',
                  style: TextStyle(
                    color: secondaryText,
                    fontSize: 9,
                  ),
                ),
              ),
              Text(
                'OUT  ${sensorData.zjFlowRate.toStringAsFixed(1)} L/min',
                style: TextStyle(
                  color: secondaryText,
                  fontSize: 9,
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  // LEAK ZONES — DISPLAY ONLY

  Widget _buildZonesPreview() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            Expanded(
              child: Text(
                'Leak zones',
                style: TextStyle(
                  color: primaryText,
                  fontSize: 18,
                  fontWeight: FontWeight.w800,
                ),
              ),
            ),
            Text(
              '3 ZONES',
              style: TextStyle(
                color: secondaryText,
                fontSize: 9,
                fontWeight: FontWeight.w800,
              ),
            ),
          ],
        ),
        const SizedBox(height: 11),
        Row(
          children: [
            Expanded(child: _zoneButton('Zone 1')),
            const SizedBox(width: 9),
            Expanded(child: _zoneButton('Zone 2')),
            const SizedBox(width: 9),
            Expanded(child: _zoneButton('Zone 3')),
          ],
        ),
      ],
    );
  }

  Widget _zoneButton(String zone) {
    final bool active = sensorData.leakZone == zone;

    return Container(
      height: 75,
      decoration: BoxDecoration(
        color: active
            ? dangerColor.withValues(alpha: 0.10)
            : cardColor,
        borderRadius: BorderRadius.circular(18),
        border: Border.all(
          color: active ? dangerColor : borderColor,
        ),
      ),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(
            active
                ? Icons.warning_rounded
                : Icons.radio_button_unchecked,
            color: active ? dangerColor : accentColor,
            size: 19,
          ),
          const SizedBox(height: 5),
          Text(
            zone,
            style: TextStyle(
              color: primaryText,
              fontSize: 10,
              fontWeight: FontWeight.w700,
            ),
          ),
        ],
      ),
    );
  }

  // MONITOR PAGE

  Widget _buildMonitorPage() {
    return SingleChildScrollView(
      padding: const EdgeInsets.fromLTRB(20, 22, 20, 30),
      child: Center(
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 1000),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              _pageHeader(
                'Monitor',
                'Live pipeline measurements',
              ),
              const SizedBox(height: 22),
              _largeReading(
                'YF-S201',
                sensorData.yfFlowRate,
                'L/min',
                waterColor,
              ),
              const SizedBox(height: 12),
              _largeReading(
                'ZJ-S201',
                sensorData.zjFlowRate,
                'L/min',
                accentColor,
              ),
              const SizedBox(height: 12),
              _largeReading(
                'Vibration 1',
                sensorData.vibration1,
                'level',
                accentColor,
              ),
              const SizedBox(height: 12),
              _largeReading(
                'Vibration 2',
                sensorData.vibration2,
                'level',
                waterColor,
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _largeReading(
    String title,
    double value,
    String unit,
    Color color,
  ) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(19),
      decoration: BoxDecoration(
        color: cardColor,
        borderRadius: BorderRadius.circular(21),
        border: Border.all(color: borderColor),
      ),
      child: Row(
        children: [
          Container(
            width: 48,
            height: 48,
            decoration: BoxDecoration(
              color: color.withValues(alpha: 0.12),
              borderRadius: BorderRadius.circular(14),
            ),
            child: Icon(
              Icons.analytics_rounded,
              color: color,
            ),
          ),
          const SizedBox(width: 13),
          Expanded(
            child: Text(
              title,
              style: TextStyle(
                color: primaryText,
                fontWeight: FontWeight.w700,
              ),
            ),
          ),
          Text(
            value.toStringAsFixed(1),
            style: TextStyle(
              color: primaryText,
              fontSize: 24,
              fontWeight: FontWeight.w800,
            ),
          ),
          const SizedBox(width: 4),
          Text(
            unit,
            style: TextStyle(
              color: secondaryText,
              fontSize: 9,
            ),
          ),
        ],
      ),
    );
  }

  // ZONES PAGE — DISPLAY ONLY

  Widget _buildZonesPage() {
    return SingleChildScrollView(
      padding: const EdgeInsets.fromLTRB(20, 22, 20, 30),
      child: Center(
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 900),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              _pageHeader(
                'Leak Zones',
                'Approximate localization',
              ),
              const SizedBox(height: 22),
              _zoneDetail('Zone 1'),
              const SizedBox(height: 12),
              _zoneDetail('Zone 2'),
              const SizedBox(height: 12),
              _zoneDetail('Zone 3'),
              const SizedBox(height: 20),
              Center(
                child: Text(
                  'Localization is approximate and based on flow difference and vibration response.',
                  textAlign: TextAlign.center,
                  style: TextStyle(
                    color: secondaryText,
                    fontSize: 10,
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _zoneDetail(String zone) {
    final bool active = sensorData.leakZone == zone;

    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(19),
      decoration: BoxDecoration(
        color: active
            ? dangerColor.withValues(alpha: 0.08)
            : cardColor,
        borderRadius: BorderRadius.circular(21),
        border: Border.all(
          color: active ? dangerColor : borderColor,
        ),
      ),
      child: Row(
        children: [
          Container(
            width: 48,
            height: 48,
            decoration: BoxDecoration(
              color: (active ? dangerColor : accentColor)
                  .withValues(alpha: 0.12),
              shape: BoxShape.circle,
            ),
            child: Icon(
              active
                  ? Icons.warning_rounded
                  : Icons.location_on_rounded,
              color: active ? dangerColor : accentColor,
            ),
          ),
          const SizedBox(width: 13),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  zone,
                  style: TextStyle(
                    color: primaryText,
                    fontWeight: FontWeight.w800,
                  ),
                ),
                const SizedBox(height: 4),
                Text(
                  active
                      ? 'Possible leak detected'
                      : 'Monitoring active',
                  style: TextStyle(
                    color: secondaryText,
                    fontSize: 10,
                  ),
                ),
              ],
            ),
          ),
          Icon(
            active
                ? Icons.warning_rounded
                : Icons.circle_outlined,
            color: active ? dangerColor : secondaryText,
          ),
        ],
      ),
    );
  }

  // SETTINGS PAGE

  Widget _buildSettingsPage() {
    return SingleChildScrollView(
      padding: const EdgeInsets.fromLTRB(20, 22, 20, 30),
      child: Center(
        child: ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 900),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              _pageHeader(
                'Settings',
                'Configure PIPE-SENSE',
              ),
              const SizedBox(height: 24),
              _sectionLabel('OPERATING MODE'),
              const SizedBox(height: 10),
              Row(
                children: [
                  Expanded(
                    child: _modeCard(
                      'Simulation',
                      'Demo sensor data',
                      Icons.science_rounded,
                      true,
                    ),
                  ),
                  const SizedBox(width: 12),
                  Expanded(
                    child: _modeCard(
                      'ESP32',
                      'Live hardware',
                      Icons.memory_rounded,
                      false,
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 25),
              _sectionLabel('APPEARANCE'),
              const SizedBox(height: 10),
              _appearanceSection(),
              const SizedBox(height: 25),
              _sectionLabel('SYSTEM'),
              const SizedBox(height: 10),
              _systemSection(),
            ],
          ),
        ),
      ),
    );
  }

  Widget _pageHeader(String title, String subtitle) {
    return LayoutBuilder(
      builder: (context, constraints) {
        final bool compact = constraints.maxWidth < 360;

        return Row(
          children: [
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    title,
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(
                      color: primaryText,
                      fontSize: compact ? 24 : 28,
                      fontWeight: FontWeight.w800,
                    ),
                  ),
                  const SizedBox(height: 4),
                  Text(
                    subtitle,
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(
                      color: secondaryText,
                      fontSize: 11,
                    ),
                  ),
                ],
              ),
            ),
            if (!compact) ...[
              const SizedBox(width: 8),
              _buildConnectionBadge(),
            ],
            const SizedBox(width: 8),
            _buildThemeButton(),
          ],
        );
      },
    );
  }

  Widget _sectionLabel(String text) {
    return Text(
      text,
      style: TextStyle(
        color: secondaryText,
        fontSize: 9,
        fontWeight: FontWeight.w800,
        letterSpacing: 1.5,
      ),
    );
  }

  Widget _modeCard(
    String title,
    String subtitle,
    IconData icon,
    bool simulation,
  ) {
    final bool selected = simulation
        ? AppModeController.isSimulation
        : AppModeController.isEsp32;

    return GestureDetector(
      onTap: () {
        setState(() {
          AppModeController.setMode(
            simulation ? AppMode.simulation : AppMode.esp32,
          );
          esp32Connected = false;
        });

        _refreshSensors();
      },
      child: Container(
        padding: const EdgeInsets.all(16),
        decoration: BoxDecoration(
          color: selected
              ? accentColor.withValues(alpha: 0.10)
              : cardColor,
          borderRadius: BorderRadius.circular(20),
          border: Border.all(
            color: selected ? accentColor : borderColor,
          ),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Icon(
              icon,
              color: selected ? accentColor : secondaryText,
            ),
            const SizedBox(height: 13),
            Text(
              title,
              style: TextStyle(
                color: primaryText,
                fontWeight: FontWeight.w700,
              ),
            ),
            const SizedBox(height: 3),
            Text(
              subtitle,
              style: TextStyle(
                color: secondaryText,
                fontSize: 9,
              ),
            ),
            const SizedBox(height: 12),
            Align(
              alignment: Alignment.centerRight,
              child: Icon(
                selected
                    ? Icons.check_circle_rounded
                    : Icons.radio_button_unchecked,
                color: selected ? accentColor : secondaryText,
                size: 18,
              ),
            ),
          ],
        ),
      ),
    );
  }

  // APPEARANCE

  Widget _appearanceSection() {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(8),
      decoration: BoxDecoration(
        color: cardColor,
        borderRadius: BorderRadius.circular(22),
        border: Border.all(color: borderColor),
      ),
      child: Column(
        children: [
          _themeOption('Dark', true),
          _themeOption('Light', false),
        ],
      ),
    );
  }

  Widget _themeOption(String title, bool dark) {
    final bool selected = isDark == dark;

    return GestureDetector(
      onTap: () {
        if (isDark != dark) {
          _toggleTheme();
        }
      },
      child: Container(
        height: 54,
        padding: const EdgeInsets.symmetric(horizontal: 12),
        decoration: BoxDecoration(
          color: selected
              ? accentColor.withValues(alpha: 0.08)
              : Colors.transparent,
          borderRadius: BorderRadius.circular(15),
        ),
        child: Row(
          children: [
            Container(
              width: 36,
              height: 24,
              decoration: BoxDecoration(
                color: dark
                    ? const Color(0xFF07110D)
                    : const Color(0xFFF2F8F5),
                borderRadius: BorderRadius.circular(7),
                border: Border.all(
                  color: dark
                      ? const Color(0xFF36564A)
                      : const Color(0xFFD0E2DB),
                ),
              ),
              child: Center(
                child: Container(
                  width: 14,
                  height: 5,
                  decoration: BoxDecoration(
                    color: waterColor,
                    borderRadius: BorderRadius.circular(5),
                  ),
                ),
              ),
            ),
            const SizedBox(width: 12),
            Expanded(
              child: Text(
                title,
                style: TextStyle(
                  color: primaryText,
                  fontSize: 11,
                  fontWeight: FontWeight.w600,
                ),
              ),
            ),
            Icon(
              selected
                  ? Icons.radio_button_checked
                  : Icons.radio_button_unchecked,
              color: selected ? accentColor : secondaryText,
              size: 18,
            ),
          ],
        ),
      ),
    );
  }

  // SYSTEM INFORMATION

  Widget _systemSection() {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(18),
      decoration: BoxDecoration(
        color: cardColor,
        borderRadius: BorderRadius.circular(22),
        border: Border.all(color: borderColor),
      ),
      child: Column(
        children: [
          _systemRow('Controller', 'ESP32'),
          _systemRow('Flow sensing', 'YF-S201 + ZJ-S201'),
          _systemRow('Vibration', '2 × MPU6050'),
          _systemRow('Leak zones', '3 zones'),
          _systemRow('Localization', 'Approximate'),
          _systemRow('Visualization', 'Three.js 3D'),
        ],
      ),
    );
  }

  Widget _systemRow(String label, String value) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 7),
      child: Row(
        children: [
          Expanded(
            child: Text(
              label,
              style: TextStyle(
                color: secondaryText,
                fontSize: 10,
              ),
            ),
          ),
          Flexible(
            child: Text(
              value,
              textAlign: TextAlign.right,
              style: TextStyle(
                color: primaryText,
                fontSize: 10,
                fontWeight: FontWeight.w700,
              ),
            ),
          ),
        ],
      ),
    );
  }

  // BOTTOM NAVIGATION

  Widget _buildBottomNavigation() {
    return SafeArea(
      top: false,
      child: Container(
        margin: const EdgeInsets.fromLTRB(18, 0, 18, 12),
        height: 70,
        padding: const EdgeInsets.all(6),
        decoration: BoxDecoration(
          color: isDark
              ? const Color(0xFF0B2118)
              : const Color(0xFF10291F),
          borderRadius: BorderRadius.circular(28),
        ),
        child: Row(
          children: [
            _navItem(0, Icons.home_rounded, 'Home'),
            _navItem(1, Icons.monitor_heart_rounded, 'Monitor'),
            _navItem(2, Icons.account_tree_rounded, 'Zones'),
            _navItem(3, Icons.settings_rounded, 'Settings'),
          ],
        ),
      ),
    );
  }

  Widget _navItem(int index, IconData icon, String label) {
    final bool selected = selectedTab == index;

    return Expanded(
      child: GestureDetector(
        onTap: () {
          setState(() {
            selectedTab = index;
          });
        },
        child: Container(
          margin: const EdgeInsets.symmetric(horizontal: 2),
          decoration: BoxDecoration(
            color: selected ? accentColor : Colors.transparent,
            borderRadius: BorderRadius.circular(21),
          ),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Icon(
                icon,
                color: selected
                    ? const Color(0xFF092016)
                    : const Color(0xFF9AAFA7),
                size: 20,
              ),
              const SizedBox(height: 3),
              Text(
                label,
                style: TextStyle(
                  color: selected
                      ? const Color(0xFF092016)
                      : const Color(0xFF9AAFA7),
                  fontSize: 9,
                  fontWeight: FontWeight.w700,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
