#include <LiquidCrystal.h>

// =====================================================
// PIPE-SENSE COMPLETE HARDWARE TEST
// =====================================================

// -------------------------
// FLOW SENSORS
// -------------------------

#define YF_FLOW_SENSOR 27
#define ZJ_FLOW_SENSOR 32

// -------------------------
// VIBRATION SENSORS
// -------------------------

#define VIBRATION_1 26
#define VIBRATION_2 25

// -------------------------
// BUZZER
// -------------------------

#define BUZZER 23

// -------------------------
// LCD
// RS, EN, D4, D5, D6, D7
// -------------------------

LiquidCrystal lcd(21, 19, 18, 17, 16, 14);

// =====================================================
// VARIABLES
// =====================================================

volatile unsigned long yfPulseCount = 0;
volatile unsigned long zjPulseCount = 0;

int vibration1Count = 0;
int vibration2Count = 0;

// =====================================================
// INTERRUPT FUNCTIONS
// =====================================================

void IRAM_ATTR countYFPulse()
{
  yfPulseCount++;
}

void IRAM_ATTR countZJPulse()
{
  zjPulseCount++;
}

// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  // Flow sensors
  pinMode(YF_FLOW_SENSOR, INPUT_PULLUP);
  pinMode(ZJ_FLOW_SENSOR, INPUT_PULLUP);

  // Vibration sensors
  pinMode(VIBRATION_1, INPUT);
  pinMode(VIBRATION_2, INPUT);

  // Buzzer
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  // Flow sensor interrupts
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
  lcd.print("SYSTEM TEST");

  delay(1500);

  lcd.clear();

  Serial.println();
  Serial.println("=================================");
  Serial.println("PIPE-SENSE COMPLETE HARDWARE TEST");
  Serial.println("=================================");
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop()
{
  // ---------------------------------------------------
  // Reset pulse counters
  // ---------------------------------------------------

  noInterrupts();

  yfPulseCount = 0;
  zjPulseCount = 0;

  interrupts();

  // ---------------------------------------------------
  // Measure flow for 1 second
  // ---------------------------------------------------

  delay(1000);

  // ---------------------------------------------------
  // Copy pulse counts safely
  // ---------------------------------------------------

  noInterrupts();

  unsigned long yfPulses = yfPulseCount;
  unsigned long zjPulses = zjPulseCount;

  interrupts();

  // ---------------------------------------------------
  // Calculate flow
  // Temporary testing factor
  // Calibration will be done later
  // ---------------------------------------------------

  float yfFlow = yfPulses / 7.5;
  float zjFlow = zjPulses / 7.5;

  // ---------------------------------------------------
  // Read vibration sensors
  //
  // SW-420:
  // 1 = Normal
  // 0 = Vibration
  // ---------------------------------------------------

  int vibration1 = digitalRead(VIBRATION_1);
  int vibration2 = digitalRead(VIBRATION_2);

  // ---------------------------------------------------
  // VIBRATION STABILITY
  // Require 2 consecutive detections
  // ---------------------------------------------------

  if (vibration1 == LOW)
  {
    vibration1Count++;
  }
  else
  {
    vibration1Count = 0;
  }

  if (vibration2 == LOW)
  {
    vibration2Count++;
  }
  else
  {
    vibration2Count = 0;
  }

  // ---------------------------------------------------
  // FLOW DETECTION
  // Either flow sensor can indicate water movement
  // ---------------------------------------------------

  bool flowDetected =
    (yfFlow > 0.0 || zjFlow > 0.0);

  // ---------------------------------------------------
  // LEAK ZONE DETECTION
  // ---------------------------------------------------

  bool leakZone1 = false;
  bool leakZone2 = false;

  if (flowDetected && vibration1Count >= 2)
  {
    leakZone1 = true;
  }

  if (flowDetected && vibration2Count >= 2)
  {
    leakZone2 = true;
  }

  // ---------------------------------------------------
  // BUZZER
  //
  // ONLY:
  // FLOW + SUSTAINED VIBRATION
  // ---------------------------------------------------

  if (leakZone1 || leakZone2)
  {
    digitalWrite(BUZZER, HIGH);
  }
  else
  {
    digitalWrite(BUZZER, LOW);
  }

  // ===================================================
  // SERIAL MONITOR
  // ===================================================

  Serial.print("YF-S201: ");
  Serial.print(yfFlow, 2);
  Serial.print(" L/min");

  Serial.print(" | ZJ-S201: ");
  Serial.print(zjFlow, 2);
  Serial.print(" L/min");

  Serial.print(" | V1: ");
  Serial.print(vibration1);

  Serial.print(" | V2: ");
  Serial.print(vibration2);

  Serial.print(" | STATUS: ");

  if (leakZone1)
  {
    Serial.println("POSSIBLE LEAK - ZONE 1");
  }
  else if (leakZone2)
  {
    Serial.println("POSSIBLE LEAK - ZONE 2");
  }
  else
  {
    Serial.println("NORMAL");
  }

  // ===================================================
  // LCD
  // SINGLE FIXED SCREEN
  // ===================================================

  // -------------------------
  // LINE 1
  // -------------------------
  //
  // Y:00.0 Z:00.0 L/m
  //

  lcd.setCursor(0, 0);

  lcd.print("Y:");
  lcd.print(yfFlow, 1);

  lcd.print(" Z:");
  lcd.print(zjFlow, 1);

  lcd.print(" L/m");

  // Clear remaining character
  lcd.print(" ");

  // -------------------------
  // LINE 2
  // -------------------------
  //
  // NORMAL:
  // V1:1 V2:1 NORMAL
  //
  // ZONE 1:
  // V1:0 V2:1 ZONE1
  //
  // ZONE 2:
  // V1:1 V2:0 ZONE2
  // -------------------------

  lcd.setCursor(0, 1);

  lcd.print("V1:");
  lcd.print(vibration1);

  lcd.print(" V2:");
  lcd.print(vibration2);

  lcd.print(" ");

  if (leakZone1)
  {
    lcd.print("ZONE1");
  }
  else if (leakZone2)
  {
    lcd.print("ZONE2");
  }
  else
  {
    lcd.print("NORMAL");
  }

  // Clear any leftover characters
  lcd.print(" ");
}