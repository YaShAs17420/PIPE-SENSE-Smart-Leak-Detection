#define FLOW_SENSOR 32

volatile int pulseCount = 0;

void IRAM_ATTR countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);

  pinMode(FLOW_SENSOR, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(FLOW_SENSOR),
    countPulse,
    FALLING
  );

  Serial.println("PIPE-SENSE ZJ-S201 FLOW SENSOR TEST");
}

void loop() {

  pulseCount = 0;

  delay(1000);

  float flowRate = pulseCount / 7.5;

  Serial.print("Pulses: ");
  Serial.print(pulseCount);

  Serial.print(" | Flow Rate: ");
  Serial.print(flowRate, 2);

  Serial.println(" L/min");
}