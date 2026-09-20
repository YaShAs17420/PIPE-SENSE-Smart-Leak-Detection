#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <math.h>

// ============================================================
// WIFI SETTINGS
// ============================================================
// Keep the same Wi-Fi network you are currently using.
// Replace these two values with your existing local credentials.

const char* WIFI_SSID = "WIN-EGIE4UL1P0Q 2013";
const char* WIFI_PASSWORD = "2532A|x3";

// ============================================================
// PIN DEFINITIONS
// ============================================================

#define YF_FLOW_SENSOR 27
#define ZJ_FLOW_SENSOR 32

#define MPU1_SDA 4
#define MPU1_SCL 22
#define MPU1_ADDR 0x68

#define MPU2_SDA 25
#define MPU2_SCL 26
#define MPU2_ADDR 0x69

#define LCD_RS 21
#define LCD_EN 19
#define LCD_D4 18
#define LCD_D5 17
#define LCD_D6 16
#define LCD_D7 5

#define BUZZER 23

// ============================================================
// OBJECTS
// ============================================================

WebServer server(80);

LiquidCrystal lcd(
  LCD_RS,
  LCD_EN,
  LCD_D4,
  LCD_D5,
  LCD_D6,
  LCD_D7
);

TwoWire I2C_0 = TwoWire(0);
TwoWire I2C_1 = TwoWire(1);

// ============================================================
// FLOW SENSOR DATA
// ============================================================

volatile uint32_t yfPulseCount = 0;
volatile uint32_t zjPulseCount = 0;

float yfFlowRate = 0.0;
float zjFlowRate = 0.0;

unsigned long lastFlowUpdate = 0;

// ============================================================
// MPU6050 DATA
// ============================================================

float vibration1 = 0.0;
float vibration2 = 0.0;

float ax1 = 0.0;
float ay1 = 0.0;
float az1 = 0.0;

float ax2 = 0.0;
float ay2 = 0.0;
float az2 = 0.0;

// ============================================================
// GRAVITY BASELINE
// ============================================================

float gravityX1 = 0.0;
float gravityY1 = 0.0;
float gravityZ1 = 0.0;

float gravityX2 = 0.0;
float gravityY2 = 0.0;
float gravityZ2 = 0.0;

bool gravityInitialized1 = false;
bool gravityInitialized2 = false;

// Slowly follows the gravity component while preserving
// short-duration vibration.
const float GRAVITY_FILTER_ALPHA = 0.05;

// ============================================================
// VIBRATION PEAK WINDOW
// ============================================================
//
// The MPU is sampled continuously.
// During each 1-second window, the highest dynamic
// acceleration is stored.
//
// This prevents a short vibration event from being missed
// simply because Flutter happened to request /data at another
// instant.
//

float vibrationPeak1 = 0.0;
float vibrationPeak2 = 0.0;

unsigned long lastVibrationWindow = 0;

// ============================================================
// INTERRUPT FUNCTIONS
// ============================================================

void IRAM_ATTR countYFPulse() {
  yfPulseCount++;
}

void IRAM_ATTR countZJPulse() {
  zjPulseCount++;
}

// ============================================================
// MPU REGISTER WRITE
// ============================================================

bool writeMPURegister(
  TwoWire &bus,
  uint8_t address,
  uint8_t reg,
  uint8_t value
) {
  bus.beginTransmission(address);
  bus.write(reg);
  bus.write(value);

  return bus.endTransmission() == 0;
}

// ============================================================
// INITIALIZE MPU6050
// ============================================================

bool initializeMPU(
  TwoWire &bus,
  uint8_t address
) {
  // Wake the MPU6050.
  if (!writeMPURegister(bus, address, 0x6B, 0x00)) {
    return false;
  }

  delay(100);

  // Accelerometer range = +/-2g.
  if (!writeMPURegister(bus, address, 0x1C, 0x00)) {
    return false;
  }

  delay(20);

  return true;
}

// ============================================================
// READ MPU6050 ACCELERATION
// ============================================================

bool readAccel(
  TwoWire &bus,
  uint8_t address,
  float &ax,
  float &ay,
  float &az
) {
  bus.beginTransmission(address);
  bus.write(0x3B);

  if (bus.endTransmission(false) != 0) {
    return false;
  }

  uint8_t bytesReceived = bus.requestFrom(
    (int)address,
    6,
    true
  );

  if (bytesReceived != 6) {
    return false;
  }

  int16_t rawAx =
      (int16_t)((bus.read() << 8) | bus.read());

  int16_t rawAy =
      (int16_t)((bus.read() << 8) | bus.read());

  int16_t rawAz =
      (int16_t)((bus.read() << 8) | bus.read());

  // +/-2g = 16384 LSB/g.
  ax = rawAx / 16384.0;
  ay = rawAy / 16384.0;
  az = rawAz / 16384.0;

  return true;
}

// ============================================================
// CALCULATE DYNAMIC VIBRATION
// ============================================================

float calculateDynamicVibration(
  float ax,
  float ay,
  float az,
  float &gravityX,
  float &gravityY,
  float &gravityZ,
  bool &initialized
) {
  // First valid reading establishes the initial gravity baseline.
  if (!initialized) {
    gravityX = ax;
    gravityY = ay;
    gravityZ = az;
    initialized = true;

    return 0.0;
  }

  // Slowly track the gravity component.
  gravityX =
      gravityX +
      GRAVITY_FILTER_ALPHA * (ax - gravityX);

  gravityY =
      gravityY +
      GRAVITY_FILTER_ALPHA * (ay - gravityY);

  gravityZ =
      gravityZ +
      GRAVITY_FILTER_ALPHA * (az - gravityZ);

  // Remove estimated gravity.
  float dynamicX = ax - gravityX;
  float dynamicY = ay - gravityY;
  float dynamicZ = az - gravityZ;

  // Dynamic acceleration magnitude.
  return sqrt(
    (dynamicX * dynamicX) +
    (dynamicY * dynamicY) +
    (dynamicZ * dynamicZ)
  );
}

// ============================================================
// UPDATE FLOW DATA
// ============================================================

void updateFlowData() {

  if (millis() - lastFlowUpdate < 1000) {
    return;
  }

  lastFlowUpdate = millis();

  noInterrupts();

  uint32_t yfPulses = yfPulseCount;
  uint32_t zjPulses = zjPulseCount;

  yfPulseCount = 0;
  zjPulseCount = 0;

  interrupts();

  // Provisional conversion.
  // This will be calibrated using the physical prototype.
  yfFlowRate = yfPulses / 7.5;
  zjFlowRate = zjPulses / 7.5;

  Serial.println();
  Serial.println("========== FLOW DATA ==========");

  Serial.print("YF Pulses: ");
  Serial.println(yfPulses);

  Serial.print("YF Flow Rate: ");
  Serial.print(yfFlowRate, 2);
  Serial.println(" L/min");

  Serial.print("ZJ Pulses: ");
  Serial.println(zjPulses);

  Serial.print("ZJ Flow Rate: ");
  Serial.print(zjFlowRate, 2);
  Serial.println(" L/min");
}

// ============================================================
// UPDATE MPU DATA
// ============================================================

void updateMPUData() {

  // ---------------- MPU1 ----------------

  bool mpu1OK = readAccel(
    I2C_0,
    MPU1_ADDR,
    ax1,
    ay1,
    az1
  );

  if (mpu1OK) {

    float currentVibration1 =
        calculateDynamicVibration(
          ax1,
          ay1,
          az1,
          gravityX1,
          gravityY1,
          gravityZ1,
          gravityInitialized1
        );

    if (currentVibration1 > vibrationPeak1) {
      vibrationPeak1 = currentVibration1;
    }
  }

  // ---------------- MPU2 ----------------

  bool mpu2OK = readAccel(
    I2C_1,
    MPU2_ADDR,
    ax2,
    ay2,
    az2
  );

  if (mpu2OK) {

    float currentVibration2 =
        calculateDynamicVibration(
          ax2,
          ay2,
          az2,
          gravityX2,
          gravityY2,
          gravityZ2,
          gravityInitialized2
        );

    if (currentVibration2 > vibrationPeak2) {
      vibrationPeak2 = currentVibration2;
    }
  }

  // ---------------- 1-SECOND PEAK WINDOW ----------------

  if (millis() - lastVibrationWindow >= 1000) {

    lastVibrationWindow = millis();

    vibration1 = vibrationPeak1;
    vibration2 = vibrationPeak2;

    vibrationPeak1 = 0.0;
    vibrationPeak2 = 0.0;
  }
}

// ============================================================
// UPDATE ALL SENSOR DATA
// ============================================================

void updateSensorData() {
  updateFlowData();
  updateMPUData();
}

// ============================================================
// LCD UPDATE
// ============================================================

void updateLCD() {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("YF:");
  lcd.print(yfFlowRate, 1);

  lcd.print(" ZJ:");
  lcd.print(zjFlowRate, 1);

  lcd.setCursor(0, 1);
  lcd.print("V1:");
  lcd.print(vibration1, 2);

  lcd.print(" V2:");
  lcd.print(vibration2, 2);
}

// ============================================================
// CREATE JSON
// ============================================================

String createJSON() {

  String json = "{";

  json += "\"yfFlowRate\":";
  json += String(yfFlowRate, 2);

  json += ",";

  json += "\"zjFlowRate\":";
  json += String(zjFlowRate, 2);

  json += ",";

  json += "\"yfPulses\":";
  json += String(yfPulseCount);

  json += ",";

  json += "\"zjPulses\":";
  json += String(zjPulseCount);

  json += ",";

  json += "\"vibration1\":";
  json += String(vibration1, 2);

  json += ",";

  json += "\"vibration2\":";
  json += String(vibration2, 2);

  json += "}";

  return json;
}

// ============================================================
// ROOT PAGE
// ============================================================

void handleRoot() {

  String page = "<!DOCTYPE html>";
  page += "<html>";
  page += "<head>";
  page += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  page += "<title>PIPE-SENSE ESP32</title>";
  page += "</head>";

  page += "<body>";

  page += "<h1>PIPE-SENSE</h1>";
  page += "<h2>ESP32 Sensor Server</h2>";

  page += "<p>ESP32 is connected successfully.</p>";
  page += "<p>Open <b>/data</b> to view sensor data.</p>";

  page += "</body>";
  page += "</html>";

  server.send(
    200,
    "text/html",
    page
  );
}

// ============================================================
// DATA ENDPOINT
// ============================================================

void handleData() {

  server.sendHeader(
    "Access-Control-Allow-Origin",
    "*"
  );

  server.sendHeader(
    "Cache-Control",
    "no-cache"
  );

  String json = createJSON();

  server.send(
    200,
    "application/json",
    json
  );
}

// ============================================================
// NOT FOUND
// ============================================================

void handleNotFound() {

  server.sendHeader(
    "Access-Control-Allow-Origin",
    "*"
  );

  server.send(
    404,
    "text/plain",
    "PIPE-SENSE: Endpoint not found"
  );
}

// ============================================================
// STARTUP BEEP
// ============================================================

void startupBeep() {

  digitalWrite(BUZZER, HIGH);
  delay(150);

  digitalWrite(BUZZER, LOW);
  delay(100);

  digitalWrite(BUZZER, HIGH);
  delay(150);

  digitalWrite(BUZZER, LOW);
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("       PIPE-SENSE ESP32 INTEGRATION");
  Serial.println("========================================");

  // ----------------------------------------------------------
  // BUZZER
  // ----------------------------------------------------------

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  startupBeep();

  // ----------------------------------------------------------
  // FLOW SENSORS
  // ----------------------------------------------------------

  pinMode(
    YF_FLOW_SENSOR,
    INPUT
  );

  pinMode(
    ZJ_FLOW_SENSOR,
    INPUT_PULLUP
  );

  attachInterrupt(
    digitalPinToInterrupt(YF_FLOW_SENSOR),
    countYFPulse,
    FALLING
  );

  attachInterrupt(
    digitalPinToInterrupt(ZJ_FLOW_SENSOR),
    countZJPulse,
    FALLING
  );

  // ----------------------------------------------------------
  // LCD
  // ----------------------------------------------------------

  lcd.begin(16, 2);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PIPE-SENSE");

  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  delay(1000);

  // ----------------------------------------------------------
  // MPU1
  // ----------------------------------------------------------

  I2C_0.begin(
    MPU1_SDA,
    MPU1_SCL
  );

  I2C_0.setClock(400000);

  // ----------------------------------------------------------
  // MPU2
  // ----------------------------------------------------------

  I2C_1.begin(
    MPU2_SDA,
    MPU2_SCL
  );

  I2C_1.setClock(400000);

  bool mpu1Initialized =
      initializeMPU(
        I2C_0,
        MPU1_ADDR
      );

  bool mpu2Initialized =
      initializeMPU(
        I2C_1,
        MPU2_ADDR
      );

  Serial.println();

  Serial.print("MPU1 initialization: ");

  if (mpu1Initialized) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
  }

  Serial.print("MPU2 initialization: ");

  if (mpu2Initialized) {
    Serial.println("OK");
  } else {
    Serial.println("FAILED");
  }

  // ----------------------------------------------------------
  // WIFI
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("Connecting to WiFi...");

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  int wifiAttempts = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    wifiAttempts < 30
  ) {

    delay(500);

    Serial.print(".");

    wifiAttempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("WiFi connected!");

    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    // --------------------------------------------------------
    // HTTP SERVER
    // --------------------------------------------------------

    server.on(
      "/",
      HTTP_GET,
      handleRoot
    );

    server.on(
      "/data",
      HTTP_GET,
      handleData
    );

    server.onNotFound(
      handleNotFound
    );

    server.begin();

    Serial.println("HTTP server started.");

    Serial.println();
    Serial.println("========================================");
    Serial.println("PIPE-SENSE READY");
    Serial.println("========================================");

    Serial.print("Open in browser: http://");
    Serial.println(WiFi.localIP());

    Serial.print("Sensor data: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/data");

  } else {

    Serial.println("WiFi connection FAILED.");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed");

    lcd.setCursor(0, 1);
    lcd.print("Check SSID");
  }

  // Start the first vibration window.
  lastVibrationWindow = millis();
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  updateSensorData();

  // ----------------------------------------------------------
  // LCD
  // ----------------------------------------------------------

  static unsigned long lastLCDUpdate = 0;

  if (millis() - lastLCDUpdate >= 1000) {

    lastLCDUpdate = millis();

    updateLCD();
  }

  // ----------------------------------------------------------
  // HTTP SERVER
  // ----------------------------------------------------------

  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }

  // ----------------------------------------------------------
  // SERIAL MONITOR
  // ----------------------------------------------------------

  static unsigned long lastSerialDisplay = 0;

  if (millis() - lastSerialDisplay >= 1000) {

    lastSerialDisplay = millis();

    Serial.println();
    Serial.println("========== VIBRATION DATA ==========");

    Serial.print("MPU1 Peak Vibration: ");
    Serial.println(vibration1, 3);

    Serial.print("MPU2 Peak Vibration: ");
    Serial.println(vibration2, 3);

    Serial.print("Gravity Baseline 1: ");
    Serial.print(gravityX1, 2);
    Serial.print(", ");
    Serial.print(gravityY1, 2);
    Serial.print(", ");
    Serial.println(gravityZ1, 2);

    Serial.print("Gravity Baseline 2: ");
    Serial.print(gravityX2, 2);
    Serial.print(", ");
    Serial.print(gravityY2, 2);
    Serial.print(", ");
    Serial.println(gravityZ2, 2);
  }

  delay(5);
}
