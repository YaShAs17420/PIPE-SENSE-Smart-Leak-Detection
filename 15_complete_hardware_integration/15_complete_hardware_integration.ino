#include <Wire.h>
#include <LiquidCrystal.h>
#include <math.h>

// ======================================================
// PIPE-SENSE
// COMPLETE HARDWARE INTEGRATION TEST
// ======================================================


// ======================================================
// 1. FLOW SENSORS
// ======================================================

#define YF_FLOW_SENSOR 27
#define ZJ_FLOW_SENSOR 32

volatile int yfPulseCount = 0;
volatile int zjPulseCount = 0;


// YF-S201 interrupt
void IRAM_ATTR countYFPulse()
{
  yfPulseCount++;
}


// ZJ-S201 interrupt
void IRAM_ATTR countZJPulse()
{
  zjPulseCount++;
}


// ======================================================
// 2. MPU6050 I2C BUSES
// ======================================================

// MPU6050 #1
// SDA -> GPIO4
// SCL -> GPIO22
// AD0 -> GND
// Address -> 0x68

TwoWire I2C_0 = TwoWire(0);


// MPU6050 #2
// SDA -> GPIO25
// SCL -> GPIO26
// AD0 -> 3.3V
// Address -> 0x69

TwoWire I2C_1 = TwoWire(1);


#define MPU1_ADDR 0x68
#define MPU2_ADDR 0x69


// MPU6050 registers
#define PWR_MGMT_1   0x6B
#define ACCEL_CONFIG 0x1C
#define ACCEL_XOUT_H 0x3B


// ======================================================
// 3. LCD
// ======================================================

// Your LCD:
//
// GND
// 5V
// D7 -> GPIO5
// D6 -> GPIO16
// D5 -> GPIO17
// D4 -> GPIO18
// EN -> GPIO19
// RS -> GPIO21

#define LCD_RS 21
#define LCD_EN 19

#define LCD_D4 18
#define LCD_D5 17
#define LCD_D6 16
#define LCD_D7 5


LiquidCrystal lcd(
  LCD_RS,
  LCD_EN,
  LCD_D4,
  LCD_D5,
  LCD_D6,
  LCD_D7
);


// ======================================================
// 4. BUZZER
// ======================================================

#define BUZZER 23


// ======================================================
// MPU6050 FUNCTIONS
// ======================================================

bool writeRegister(
  TwoWire &bus,
  uint8_t address,
  uint8_t reg,
  uint8_t value
)
{
  bus.beginTransmission(address);

  bus.write(reg);
  bus.write(value);

  return bus.endTransmission() == 0;
}


// ------------------------------------------------------
// Check MPU6050 connection
// ------------------------------------------------------

bool checkMPU(
  TwoWire &bus,
  uint8_t address
)
{
  bus.beginTransmission(address);

  return bus.endTransmission() == 0;
}


// ------------------------------------------------------
// Read accelerometer
// ------------------------------------------------------

bool readAccel(
  TwoWire &bus,
  uint8_t address,
  float &x,
  float &y,
  float &z
)
{
  bus.beginTransmission(address);

  bus.write(ACCEL_XOUT_H);

  if (bus.endTransmission(false) != 0)
  {
    return false;
  }


  if (bus.requestFrom(address, (uint8_t)6) != 6)
  {
    return false;
  }


  int16_t rawX =
    (bus.read() << 8) | bus.read();

  int16_t rawY =
    (bus.read() << 8) | bus.read();

  int16_t rawZ =
    (bus.read() << 8) | bus.read();


  // ±2g range
  // 16384 LSB/g

  x = rawX / 16384.0;
  y = rawY / 16384.0;
  z = rawZ / 16384.0;


  return true;
}


// ------------------------------------------------------
// Calculate acceleration magnitude
// ------------------------------------------------------

float accelerationMagnitude(
  float x,
  float y,
  float z
)
{
  return sqrt(
    x * x +
    y * y +
    z * z
  );
}


// ======================================================
// SETUP
// ======================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);


  Serial.println();
  Serial.println("==========================================");
  Serial.println("     PIPE-SENSE HARDWARE TEST");
  Serial.println("==========================================");


  // ====================================================
  // FLOW SENSOR SETUP
  // ====================================================

  pinMode(
    YF_FLOW_SENSOR,
    INPUT
  );

  attachInterrupt(
    digitalPinToInterrupt(YF_FLOW_SENSOR),
    countYFPulse,
    FALLING
  );


  pinMode(
    ZJ_FLOW_SENSOR,
    INPUT_PULLUP
  );

  attachInterrupt(
    digitalPinToInterrupt(ZJ_FLOW_SENSOR),
    countZJPulse,
    FALLING
  );


  Serial.println();
  Serial.println("Flow sensors initialized.");
  Serial.println("YF-S201 -> GPIO27");
  Serial.println("ZJ-S201 -> GPIO32");


  // ====================================================
  // MPU6050 SETUP
  // ====================================================

  I2C_0.begin(
    4,
    22
  );

  I2C_1.begin(
    25,
    26
  );


  I2C_0.setClock(100000);
  I2C_1.setClock(100000);


  // ----------------------------------------------------
  // MPU #1
  // ----------------------------------------------------

  Serial.println();
  Serial.println("Checking MPU6050 #1...");


  if (checkMPU(
        I2C_0,
        MPU1_ADDR
      ))
  {
    Serial.println("MPU #1 FOUND");
    Serial.println("Address: 0x68");
    Serial.println("SDA: GPIO4");
    Serial.println("SCL: GPIO22");


    writeRegister(
      I2C_0,
      MPU1_ADDR,
      PWR_MGMT_1,
      0x00
    );


    writeRegister(
      I2C_0,
      MPU1_ADDR,
      ACCEL_CONFIG,
      0x00
    );
  }
  else
  {
    Serial.println("MPU #1 NOT FOUND");
  }


  // ----------------------------------------------------
  // MPU #2
  // ----------------------------------------------------

  Serial.println();
  Serial.println("Checking MPU6050 #2...");


  if (checkMPU(
        I2C_1,
        MPU2_ADDR
      ))
  {
    Serial.println("MPU #2 FOUND");
    Serial.println("Address: 0x69");
    Serial.println("SDA: GPIO25");
    Serial.println("SCL: GPIO26");


    writeRegister(
      I2C_1,
      MPU2_ADDR,
      PWR_MGMT_1,
      0x00
    );


    writeRegister(
      I2C_1,
      MPU2_ADDR,
      ACCEL_CONFIG,
      0x00
    );
  }
  else
  {
    Serial.println("MPU #2 NOT FOUND");
  }


  // ====================================================
  // LCD SETUP
  // ====================================================

  lcd.begin(
    16,
    2
  );


  lcd.clear();

  lcd.setCursor(
    0,
    0
  );

  lcd.print("PIPE-SENSE");


  lcd.setCursor(
    0,
    1
  );

  lcd.print("Starting...");


  delay(2000);


  // ====================================================
  // BUZZER SETUP
  // ====================================================

  pinMode(
    BUZZER,
    OUTPUT
  );


  digitalWrite(
    BUZZER,
    LOW
  );


  // Startup beep

  digitalWrite(
    BUZZER,
    HIGH
  );

  delay(300);

  digitalWrite(
    BUZZER,
    LOW
  );


  // ====================================================
  // STARTUP COMPLETE
  // ====================================================

  Serial.println();
  Serial.println("==========================================");
  Serial.println("ALL HARDWARE INITIALIZED");
  Serial.println("==========================================");
  Serial.println();

  delay(1000);
}


// ======================================================
// LOOP
// ======================================================

void loop()
{
  // ====================================================
  // 1. GET FLOW PULSES
  // ====================================================

  noInterrupts();

  int yfPulses = yfPulseCount;
  int zjPulses = zjPulseCount;

  yfPulseCount = 0;
  zjPulseCount = 0;

  interrupts();


  // ====================================================
  // 2. ZJ-S201 FLOW CALCULATION
  // ====================================================

  float zjFlowRate =
    zjPulses / 7.5;


  // ====================================================
  // 3. READ MPU #1
  // ====================================================

  float x1 = 0;
  float y1 = 0;
  float z1 = 0;

  bool mpu1OK =
    readAccel(
      I2C_0,
      MPU1_ADDR,
      x1,
      y1,
      z1
    );


  // ====================================================
  // 4. READ MPU #2
  // ====================================================

  float x2 = 0;
  float y2 = 0;
  float z2 = 0;

  bool mpu2OK =
    readAccel(
      I2C_1,
      MPU2_ADDR,
      x2,
      y2,
      z2
    );


  // ====================================================
  // 5. CALCULATE VIBRATION MAGNITUDE
  // ====================================================

  float magnitude1 = 0;
  float magnitude2 = 0;


  if (mpu1OK)
  {
    magnitude1 =
      accelerationMagnitude(
        x1,
        y1,
        z1
      );
  }


  if (mpu2OK)
  {
    magnitude2 =
      accelerationMagnitude(
        x2,
        y2,
        z2
      );
  }


  // ====================================================
  // 6. SERIAL MONITOR
  // ====================================================

  Serial.println(
    "------------------------------------------"
  );


  Serial.print(
    "YF-S201 Pulses: "
  );

  Serial.println(
    yfPulses
  );


  Serial.print(
    "ZJ-S201 Pulses: "
  );

  Serial.print(
    zjPulses
  );


  Serial.print(
    " | Flow: "
  );

  Serial.print(
    zjFlowRate,
    2
  );

  Serial.println(
    " L/min"
  );


  // ----------------------------------------------------
  // MPU #1
  // ----------------------------------------------------

  Serial.print(
    "MPU #1: "
  );


  if (mpu1OK)
  {
    Serial.print(
      "X="
    );

    Serial.print(
      x1,
      3
    );


    Serial.print(
      "  Y="
    );

    Serial.print(
      y1,
      3
    );


    Serial.print(
      "  Z="
    );

    Serial.print(
      z1,
      3
    );


    Serial.print(
      "  | Mag="
    );

    Serial.print(
      magnitude1,
      3
    );


    Serial.println(
      " g"
    );
  }
  else
  {
    Serial.println(
      "READ ERROR"
    );
  }


  // ----------------------------------------------------
  // MPU #2
  // ----------------------------------------------------

  Serial.print(
    "MPU #2: "
  );


  if (mpu2OK)
  {
    Serial.print(
      "X="
    );

    Serial.print(
      x2,
      3
    );


    Serial.print(
      "  Y="
    );

    Serial.print(
      y2,
      3
    );


    Serial.print(
      "  Z="
    );

    Serial.print(
      z2,
      3
    );


    Serial.print(
      "  | Mag="
    );

    Serial.print(
      magnitude2,
      3
    );


    Serial.println(
      " g"
    );
  }
  else
  {
    Serial.println(
      "READ ERROR"
    );
  }


  // ====================================================
  // 7. LCD DISPLAY
  // ====================================================

  lcd.clear();


  // First line
  lcd.setCursor(
    0,
    0
  );

  lcd.print(
    "YF:"
  );

  lcd.print(
    yfPulses
  );


  lcd.print(
    " ZJ:"
  );

  lcd.print(
    zjPulses
  );


  // Second line
  lcd.setCursor(
    0,
    1
  );

  lcd.print(
    "V1:"
  );

  lcd.print(
    magnitude1,
    1
  );


  lcd.print(
    " V2:"
  );

  lcd.print(
    magnitude2,
    1
  );


  // ====================================================
  // 8. UPDATE EVERY SECOND
  // ====================================================

  delay(1000);
}