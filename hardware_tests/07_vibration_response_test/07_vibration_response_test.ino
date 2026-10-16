#define VIBRATION_1 26
#define VIBRATION_2 25

void setup() {
  Serial.begin(115200);

  pinMode(VIBRATION_1, INPUT);
  pinMode(VIBRATION_2, INPUT);

  Serial.println("PIPE-SENSE VIBRATION RESPONSE TEST");
}

void loop() {

  int vibration1 = digitalRead(VIBRATION_1);
  int vibration2 = digitalRead(VIBRATION_2);

  Serial.print("V1: ");
  Serial.print(vibration1);

  Serial.print(" | V2: ");
  Serial.println(vibration2);

  delay(50);
}