#include <LiquidCrystal.h>

// LCD connections
LiquidCrystal lcd(21, 19, 18, 17, 16, 14);

// Sensor pins
#define FLOW_SENSOR 27
#define VIBRATION_1 26
#define VIBRATION_2 25

// Buzzer
#define BUZZER 23

volatile int pulseCount = 0;

int vibration1Count = 0;
int vibration2Count = 0;

void IRAM_ATTR countPulse() {
  pulseCount++;
}

void setup() {

  Serial.begin(115200);

  // Sensor setup
  pinMode(FLOW_SENSOR, INPUT_PULLUP);
  pinMode(VIBRATION_1, INPUT);
  pinMode(VIBRATION_2, INPUT);

  // Buzzer
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  // Flow sensor interrupt
  attachInterrupt(
    digitalPinToInterrupt(FLOW_SENSOR),
    countPulse,
    FALLING
  );

  // LCD
  lcd.begin(16, 2);

  lcd.setCursor(0, 0);
  lcd.print("PIPE-SENSE");

  lcd.setCursor(0, 1);
  lcd.print("SYSTEM READY");

  Serial.println("PIPE-SENSE INTEGRATED TEST");

  delay(2000);

  lcd.clear();
}

void loop() {

  // Measure flow for 1 second
  pulseCount = 0;

  delay(1000);

  float flowRate = pulseCount / 7.5;

  // Read vibration sensors
  int vibration1 = digitalRead(VIBRATION_1);
  int vibration2 = digitalRead(VIBRATION_2);

  // Count repeated vibration detections
  if (vibration1 == 0) {
    vibration1Count++;
  } else {
    vibration1Count = 0;
  }

  if (vibration2 == 0) {
    vibration2Count++;
  } else {
    vibration2Count = 0;
  }

  // Determine leak status
  bool leakZone1 = (flowRate > 0 && vibration1Count >= 2);
  bool leakZone2 = (flowRate > 0 && vibration2Count >= 2);

  // LCD line 1
  lcd.setCursor(0, 0);
  lcd.print("Flow:");
  lcd.print(flowRate, 1);
  lcd.print(" L/min   ");

  // LCD line 2 + buzzer
  lcd.setCursor(0, 1);

  if (leakZone1) {

    lcd.print("LEAK ZONE 1   ");
    digitalWrite(BUZZER, HIGH);

    Serial.println("STATUS: POSSIBLE LEAK | LOCATION: ZONE 1");
  }

  else if (leakZone2) {

    lcd.print("LEAK ZONE 2   ");
    digitalWrite(BUZZER, HIGH);

    Serial.println("STATUS: POSSIBLE LEAK | LOCATION: ZONE 2");
  }

  else {

    lcd.print("NORMAL         ");
    digitalWrite(BUZZER, LOW);

    Serial.println("STATUS: NORMAL");
  }

  // Serial Monitor
  Serial.print("Flow: ");
  Serial.print(flowRate, 2);

  Serial.print(" L/min | V1: ");
  Serial.print(vibration1);

  Serial.print(" | V2: ");
  Serial.println(vibration2);
}