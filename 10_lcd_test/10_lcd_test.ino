#include <LiquidCrystal.h>

LiquidCrystal lcd(21, 19, 18, 17, 16, 14);

void setup() {
  lcd.begin(16, 2);

  lcd.setCursor(0, 0);
  lcd.print("PIPE-SENSE");

  lcd.setCursor(0, 1);
  lcd.print("LCD WORKING");
}

void loop() {
}