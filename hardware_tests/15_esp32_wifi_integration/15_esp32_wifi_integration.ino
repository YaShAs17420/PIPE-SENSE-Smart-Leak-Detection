
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <math.h>

// ============================================================
// PIPE-SENSE - ESP32 SENSOR SERVER
// Readable Serial Monitor + Flutter HTTP Integration
// ============================================================

// ENTER YOUR CURRENTLY WORKING WI-FI CREDENTIALS
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

float yfFlowRate = 0.0f;
float zjFlowRate = 0.0f;

uint32_t yfPulsesLastSecond = 0;
uint32_t zjPulsesLastSecond = 0;

unsigned long lastFlowUpdate = 0;

// Provisional conversion. Calibrate using measured water volume.
const float YF_PULSES_PER_LPM = 7.5f;
const float ZJ_PULSES_PER_LPM = 7.5f;

// ============================================================
// MPU6050 DATA
// ============================================================

float vibration1 = 0.0f;
float vibration2 = 0.0f;

float ax1 = 0.0f;
float ay1 = 0.0f;
float az1 = 0.0f;

float ax2 = 0.0f;
float ay2 = 0.0f;
float az2 = 0.0f;

// Gravity estimates
float gravityX1 = 0.0f;
float gravityY1 = 0.0f;
float gravityZ1 = 0.0f;

float gravityX2 = 0.0f;
float gravityY2 = 0.0f;
float gravityZ2 = 0.0f;

bool gravityInitialized1 = false;
bool gravityInitialized2 = false;

const float GRAVITY_FILTER_ALPHA = 0.05f;

// Peak vibration measured during each one-second window
float vibrationPeak1 = 0.0f;
float vibrationPeak2 = 0.0f;

unsigned long lastVibrationWindow = 0;

// ============================================================
// SERIAL MONITOR SETTINGS
// ============================================================

// One readable report every five seconds.
// Sensor sampling and HTTP responses remain active in between.
const unsigned long SERIAL_REPORT_INTERVAL = 5000;

unsigned long lastSerialReport = 0;

// ============================================================
// INTERRUPTS
// ============================================================

void IRAM_ATTR countYFPulse() {
  yfPulseCount++;
}

void IRAM_ATTR countZJPulse() {
  zjPulseCount++;
}

// ============================================================
// MPU6050 REGISTER WRITE
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
  if (!writeMPURegister(bus, address, 0x6B, 0x00)) {
    return false;
  }

  delay(100);

  // Accelerometer range: +/- 2g
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

  // +/- 2g = 16384 LSB/g
  ax = rawAx / 16384.0f;
  ay = rawAy / 16384.0f;
  az = rawAz / 16384.0f;

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
  if (!initialized) {
    gravityX = ax;
    gravityY = ay;
    gravityZ = az;

    initialized = true;

    return 0.0f;
  }

  // Slowly estimate the gravity component.
  gravityX += GRAVITY_FILTER_ALPHA * (ax - gravityX);
  gravityY += GRAVITY_FILTER_ALPHA * (ay - gravityY);
  gravityZ += GRAVITY_FILTER_ALPHA * (az - gravityZ);

  // Remove estimated gravity.
  float dynamicX = ax - gravityX;
  float dynamicY = ay - gravityY;
  float dynamicZ = az - gravityZ;

  return sqrtf(
    dynamicX * dynamicX +
    dynamicY * dynamicY +
    dynamicZ * dynamicZ
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

  yfPulsesLastSecond = yfPulses;
  zjPulsesLastSecond = zjPulses;

  // Provisional conversion; physical calibration is still required.
  yfFlowRate = yfPulses / YF_PULSES_PER_LPM;
  zjFlowRate = zjPulses / ZJ_PULSES_PER_LPM;

  // Intentionally no Serial printing here.
  // The consolidated report is printed every five seconds.
}

// ============================================================
// UPDATE MPU DATA
// ============================================================

void updateMPUData() {
  // MPU 1
  bool mpu1OK = readAccel(
    I2C_0,
    MPU1_ADDR,
    ax1,
    ay1,
    az1
  );

  if (mpu1OK) {
    float currentVibration1 = calculateDynamicVibration(
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

  // MPU 2
  bool mpu2OK = readAccel(
    I2C_1,
    MPU2_ADDR,
    ax2,
    ay2,
    az2
  );

  if (mpu2OK) {
    float currentVibration2 = calculateDynamicVibration(
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

  // Update the vibration readings every second.
  if (millis() - lastVibrationWindow >= 1000) {
    lastVibrationWindow = millis();

    vibration1 = vibrationPeak1;
    vibration2 = vibrationPeak2;

    vibrationPeak1 = 0.0f;
    vibrationPeak2 = 0.0f;
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
  lcd.setCursor(0, 0);
  lcd.print("                ");

  lcd.setCursor(0, 0);
  lcd.print("YF:");
  lcd.print(yfFlowRate, 1);
  lcd.print(" ZJ:");
  lcd.print(zjFlowRate, 1);

  lcd.setCursor(0, 1);
  lcd.print("                ");

  lcd.setCursor(0, 1);
  lcd.print("V1:");
  lcd.print(vibration1, 2);
  lcd.print(" V2:");
  lcd.print(vibration2, 2);
}

// ============================================================
// CREATE JSON FOR FLUTTER
// ============================================================

String createJSON() {
  String json = "{";

  json += "\"yfFlowRate\":";
  json += String(yfFlowRate, 2);

  json += ",\"zjFlowRate\":";
  json += String(zjFlowRate, 2);

  json += ",\"yfPulses\":";
  json += String(yfPulsesLastSecond);

  json += ",\"zjPulses\":";
  json += String(zjPulsesLastSecond);

  json += ",\"vibration1\":";
  json += String(vibration1, 2);

  json += ",\"vibration2\":";
  json += String(vibration2, 2);

  json += "}";

  return json;
}

// ============================================================
// ROOT WEB PAGE
// ============================================================

void handleRoot() {
  String page = "<!DOCTYPE html><html><head>";
  page += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  page += "<title>PIPE-SENSE ESP32</title></head><body>";
  page += "<h1>PIPE-SENSE</h1>";
  page += "<h2>ESP32 Sensor Server</h2>";
  page += "<p>ESP32 is connected successfully.</p>";
  page += "<p>Open <b>/data</b> to view sensor data.</p>";
  page += "</body></html>";

  server.send(200, "text/html", page);
}

// ============================================================
// SENSOR DATA ENDPOINT
// ============================================================

void handleData() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.sendHeader("Pragma", "no-cache");

  server.send(
    200,
    "application/json",
    createJSON()
  );
}

// ============================================================
// NOT FOUND
// ============================================================

void handleNotFound() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  server.send(
    404,
    "text/plain",
    "PIPE-SENSE: Endpoint not found"
  );
}

// ============================================================
// BUZZER STARTUP BEEP
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
// WI-FI CONNECTION
// ============================================================

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println();
  Serial.println("Connecting to Wi-Fi...");

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 60) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected!");
    Serial.print("ESP32 IP address: ");
    Serial.println(WiFi.localIP());

    Serial.print("Wi-Fi signal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("Wi-Fi connection failed.");
  }
}

// ============================================================
// START HTTP SERVER
// ============================================================

void startHttpServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.onNotFound(handleNotFound);

  server.begin();

  Serial.println("HTTP server started.");
  Serial.print("Dashboard data URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/data");
}

// ============================================================
// PRINT ONE READABLE SENSOR REPORT
// ============================================================

void printSensorReport() {
  Serial.println();
  Serial.println("========================================");
  Serial.println("          PIPE-SENSE LIVE DATA");
  Serial.println("========================================");

  Serial.print("Wi-Fi status : ");
  Serial.println(
    WiFi.status() == WL_CONNECTED
      ? "CONNECTED"
      : "DISCONNECTED"
  );

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("ESP32 IP     : ");
    Serial.println(WiFi.localIP());

    Serial.print("Wi-Fi signal : ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }

  Serial.println("----------------------------------------");

  Serial.print("YF-S201 flow : ");
  Serial.print(yfFlowRate, 2);
  Serial.println(" L/min");

  Serial.print("ZJ-S201 flow : ");
  Serial.print(zjFlowRate, 2);
  Serial.println(" L/min");

  Serial.print("YF pulses    : ");
  Serial.println(yfPulsesLastSecond);

  Serial.print("ZJ pulses    : ");
  Serial.println(zjPulsesLastSecond);

  Serial.println("----------------------------------------");

  Serial.print("MPU6050 #1   : ");
  Serial.println(vibration1, 3);

  Serial.print("MPU6050 #2   : ");
  Serial.println(vibration2, 3);

  Serial.println("========================================");
}

// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("       PIPE-SENSE ESP32 SERVER");
  Serial.println("========================================");

  // Buzzer
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  startupBeep();

  // Flow sensors
  pinMode(YF_FLOW_SENSOR, INPUT);
  pinMode(ZJ_FLOW_SENSOR, INPUT_PULLUP);

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

  // LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PIPE-SENSE");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  // Initialize independent I2C buses.
  I2C_0.begin(MPU1_SDA, MPU1_SCL);
  I2C_1.begin(MPU2_SDA, MPU2_SCL);

  I2C_0.setClock(400000);
  I2C_1.setClock(400000);

  bool mpu1Initialized = initializeMPU(I2C_0, MPU1_ADDR);
  bool mpu2Initialized = initializeMPU(I2C_1, MPU2_ADDR);

  Serial.print("MPU6050 #1: ");
  Serial.println(mpu1Initialized ? "OK" : "FAILED");

  Serial.print("MPU6050 #2: ");
  Serial.println(mpu2Initialized ? "OK" : "FAILED");

  if (!mpu1Initialized || !mpu2Initialized) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MPU ERROR");
    lcd.setCursor(0, 1);
    lcd.print("Check sensor");
    delay(1500);
  }

  // Keep both sensors still during initial gravity calibration.
  Serial.println();
  Serial.println("Initializing vibration baseline...");
  Serial.println("Keep both MPU6050 sensors still.");

  // Wi-Fi
  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    startHttpServer();
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Wi-Fi failed");
    lcd.setCursor(0, 1);
    lcd.print("Check settings");
  }

  lastFlowUpdate = millis();
  lastVibrationWindow = millis();
  lastSerialReport = millis();

  lcd.clear();

  Serial.println();
  Serial.println("PIPE-SENSE INITIALIZATION COMPLETE.");
  Serial.println("Sensor report interval: 5 seconds.");
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {
  // Continue sampling sensors frequently.
  updateSensorData();

  // Refresh LCD once per second.
  static unsigned long lastLCDUpdate = 0;

  if (millis() - lastLCDUpdate >= 1000) {
    lastLCDUpdate = millis();
    updateLCD();
  }

  // Keep servicing Flutter/browser requests.
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }

  // Print only one consolidated report every five seconds.
  if (millis() - lastSerialReport >= SERIAL_REPORT_INTERVAL) {
    lastSerialReport = millis();
    printSensorReport();
  }

  // Small yield; do not add a long delay here.
  delay(2);
}
