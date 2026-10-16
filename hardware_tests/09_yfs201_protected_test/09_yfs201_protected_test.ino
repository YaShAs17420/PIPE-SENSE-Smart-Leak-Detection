#define FLOW_SENSOR 27

volatile int pulseCount = 0;

void IRAM_ATTR countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);

  pinMode(FLOW_SENSOR, INPUT);

  attachInterrupt(
    digitalPinToInterrupt(FLOW_SENSOR),
    countPulse,
    FALLING
  );

  Serial.println("PIPE-SENSE YF-S201 PROTECTED TEST");
}

void loop() {

  pulseCount = 0;

  delay(1000);

  Serial.print("Pulses: ");
  Serial.println(pulseCount);

  delay(500);
}