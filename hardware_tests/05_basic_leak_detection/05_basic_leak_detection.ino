#define FLOW_SENSOR 27
#define VIBRATION_1 26
#define VIBRATION_2 25

volatile int pulseCount = 0;

void IRAM_ATTR countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);

  pinMode(FLOW_SENSOR, INPUT_PULLUP);
  pinMode(VIBRATION_1, INPUT);
  pinMode(VIBRATION_2, INPUT);

  attachInterrupt(
    digitalPinToInterrupt(FLOW_SENSOR),
    countPulse,
    FALLING
  );

  Serial.println("PIPE-SENSE LEAK DETECTION TEST");
}

void loop() {

  pulseCount = 0;

  delay(1000);

  float flowRate = pulseCount / 7.5;

  int vibration1 = digitalRead(VIBRATION_1);
  int vibration2 = digitalRead(VIBRATION_2);

  Serial.print("Flow: ");
  Serial.print(flowRate, 2);
  Serial.print(" L/min");

  Serial.print(" | V1: ");
  Serial.print(vibration1);

  Serial.print(" | V2: ");
  Serial.print(vibration2);

  // Leak detection logic
  if (flowRate > 0 && (vibration1 == 0 || vibration2 == 0)) {
    Serial.println(" | STATUS: POSSIBLE LEAK");
  }
  else {
    Serial.println(" | STATUS: NORMAL");
  }
}