#include <Wire.h>

// MPU #1
TwoWire I2C_0 = TwoWire(0);

// MPU #2
TwoWire I2C_1 = TwoWire(1);

#define MPU1_ADDR 0x68
#define MPU2_ADDR 0x69

#define WHO_AM_I     0x75
#define PWR_MGMT_1   0x6B
#define ACCEL_CONFIG 0x1C
#define ACCEL_XOUT_H 0x3B

// --------------------------------------------------
// Read one register
// --------------------------------------------------
uint8_t readRegister(TwoWire &bus, uint8_t address, uint8_t reg)
{
  bus.beginTransmission(address);
  bus.write(reg);

  if (bus.endTransmission(false) != 0)
  {
    return 0xFF;
  }

  bus.requestFrom(address, (uint8_t)1);

  if (bus.available())
  {
    return bus.read();
  }

  return 0xFF;
}

// --------------------------------------------------
// Write one register
// --------------------------------------------------
bool writeRegister(TwoWire &bus, uint8_t address,
                   uint8_t reg, uint8_t value)
{
  bus.beginTransmission(address);
  bus.write(reg);
  bus.write(value);

  return bus.endTransmission() == 0;
}

// --------------------------------------------------
// Check device
// --------------------------------------------------
bool devicePresent(TwoWire &bus, uint8_t address)
{
  bus.beginTransmission(address);
  return bus.endTransmission() == 0;
}

// --------------------------------------------------
// Read 6 accelerometer bytes
// --------------------------------------------------
void readRawAccel(TwoWire &bus, uint8_t address)
{
  bus.beginTransmission(address);
  bus.write(ACCEL_XOUT_H);

  if (bus.endTransmission(false) != 0)
  {
    Serial.println("ACCEL REGISTER READ FAILED");
    return;
  }

  int count = bus.requestFrom(address, (uint8_t)6);

  if (count != 6)
  {
    Serial.print("Only received ");
    Serial.print(count);
    Serial.println(" bytes");
    return;
  }

  int16_t x = (bus.read() << 8) | bus.read();
  int16_t y = (bus.read() << 8) | bus.read();
  int16_t z = (bus.read() << 8) | bus.read();

  Serial.print("RAW X = ");
  Serial.print(x);

  Serial.print("   RAW Y = ");
  Serial.print(y);

  Serial.print("   RAW Z = ");
  Serial.println(z);
}

// ==================================================
// SETUP
// ==================================================
void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" PIPE-SENSE MPU6050 DIAGNOSTIC TEST");
  Serial.println("========================================");

  // MPU #1
  I2C_0.begin(4, 22);

  // MPU #2
  I2C_1.begin(25, 26);

  I2C_0.setClock(100000);
  I2C_1.setClock(100000);

  // -----------------------------------------------
  // MPU #1
  // -----------------------------------------------
  Serial.println();
  Serial.println("========== MPU #1 ==========");

  if (devicePresent(I2C_0, MPU1_ADDR))
  {
    Serial.println("I2C DEVICE FOUND at 0x68");

    uint8_t who1 = readRegister(I2C_0, MPU1_ADDR, WHO_AM_I);

    Serial.print("WHO_AM_I = 0x");
    Serial.println(who1, HEX);

    writeRegister(I2C_0, MPU1_ADDR, PWR_MGMT_1, 0x00);
    writeRegister(I2C_0, MPU1_ADDR, ACCEL_CONFIG, 0x00);

    Serial.println("Raw accelerometer:");
    readRawAccel(I2C_0, MPU1_ADDR);
  }
  else
  {
    Serial.println("MPU #1 NOT FOUND");
  }

  // -----------------------------------------------
  // MPU #2
  // -----------------------------------------------
  Serial.println();
  Serial.println("========== MPU #2 ==========");

  if (devicePresent(I2C_1, MPU2_ADDR))
  {
    Serial.println("I2C DEVICE FOUND at 0x69");

    uint8_t who2 = readRegister(I2C_1, MPU2_ADDR, WHO_AM_I);

    Serial.print("WHO_AM_I = 0x");
    Serial.println(who2, HEX);

    writeRegister(I2C_1, MPU2_ADDR, PWR_MGMT_1, 0x00);
    writeRegister(I2C_1, MPU2_ADDR, ACCEL_CONFIG, 0x00);

    Serial.println("Raw accelerometer:");
    readRawAccel(I2C_1, MPU2_ADDR);
  }
  else
  {
    Serial.println("MPU #2 NOT FOUND at 0x69");
  }

  Serial.println();
  Serial.println("========================================");
  Serial.println("DIAGNOSTIC COMPLETE");
  Serial.println("========================================");
}

// ==================================================
// LOOP
// ==================================================
void loop()
{
  Serial.println();
  Serial.println("-------- LIVE TEST --------");

  Serial.print("MPU #1: ");
  readRawAccel(I2C_0, MPU1_ADDR);

  Serial.print("MPU #2: ");
  readRawAccel(I2C_1, MPU2_ADDR);

  delay(1000);
}