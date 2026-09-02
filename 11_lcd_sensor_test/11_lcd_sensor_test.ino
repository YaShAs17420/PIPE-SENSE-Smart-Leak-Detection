#include <LiquidCrystal.h>

#define FLOW_SENSOR 27
#define VIBRATION_1 26
#define VIBRATION_2 25

LiquidCrystal lcd(21, 19, 18, 17, 16, 14);

volatile int pulseCount = 0;

void IRAM_ATTR countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);

  pinMode(FLOW_SENSOR, INPUT);
  pinMode(VIBRATION_1, INPUT);
  pinMode(VIBRATION_2, INPUT);

  attachInterrupt(
    digitalPinToInterrupt(FLOW_SENSOR),
    countPulse,
    FALLING
  );

  lcd.begin(16, 2);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PIPE-SENSE");
  lcd.setCursor(0, 1);
  lcd.print("SYSTEM READY");

  delay(2000);
}

void loop() {

  pulseCount = 0;

  delay(1000);

  int vibration1 = digitalRead(VIBRATION_1);
  int vibration2 = digitalRead(VIBRATION_2);

  bool flowDetected = pulseCount > 0;

  lcd.clear();

  lcd.setCursor(0, 0);

  if (flowDetected) {
    lcd.print("Flow: YES");
  } else {
    lcd.print("Flow: NO");
  }

  lcd.setCursor(0, 1);

  if (flowDetected && vibration1 == 0) {
    lcd.print("LEAK: ZONE 1");
  }
  else if (flowDetected && vibration2 == 0) {
    lcd.print("LEAK: ZONE 2");
  }
  else {
    lcd.print("STATUS: NORMAL");
  }

  Serial.print("Flow: ");
  Serial.print(flowDetected ? "YES" : "NO");

  Serial.print(" | Pulses: ");
  Serial.print(pulseCount);

  Serial.print(" | V1: ");
  Serial.print(vibration1 == 0 ? "VIBRATION" : "NORMAL");

  Serial.print(" | V2: ");
  Serial.print(vibration2 == 0 ? "VIBRATION" : "NORMAL");

  Serial.println();
}