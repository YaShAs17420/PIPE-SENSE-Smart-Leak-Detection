#define FLOW_SENSOR 27
#define VIBRATION_1 26
#define VIBRATION_2 25

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

  Serial.println("PIPE-SENSE COMBINED SENSOR TEST");
  Serial.println("--------------------------------");
}

void loop() {

  pulseCount = 0;

  delay(1000);

  int vibration1 = digitalRead(VIBRATION_1);
  int vibration2 = digitalRead(VIBRATION_2);

  bool flowDetected = pulseCount > 0;

  Serial.print("Flow: ");
  Serial.print(flowDetected ? "YES" : "NO");

  Serial.print(" | Pulses: ");
  Serial.print(pulseCount);

  Serial.print(" | V1: ");
  Serial.print(vibration1 == 0 ? "VIBRATION" : "NORMAL");

  Serial.print(" | V2: ");
  Serial.println(vibration2 == 0 ? "VIBRATION" : "NORMAL");

  if (flowDetected && vibration1 == 0) {
    Serial.println("STATUS: POSSIBLE LEAK | ZONE 1");
  }
  else if (flowDetected && vibration2 == 0) {
    Serial.println("STATUS: POSSIBLE LEAK | ZONE 2");
  }
  else {
    Serial.println("STATUS: NORMAL");
  }

  Serial.println();
}