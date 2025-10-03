#include <Wire.h>
#include <TFT_eSPI.h>

#define SDA_PIN 21
#define SCL_PIN 22
#define TFT_BL   4
#define BAT_ADC  36   // jetzt GPIO36 für externen Spannungsteiler

TFT_eSPI tft = TFT_eSPI();

// Gerätezuordnung (I2C-Adresse → Gerätename)
struct DeviceName {
  uint8_t address;
  const char* name;
};

const DeviceName knownDevices[] = {
  {0x29, "TSL2591 / VL53L0X"},
  {0x3C, "OLED SSD1306"},
  {0x40, "INA219"},
  {0x48, "ADS1115 / TMP102"},
  {0x76, "BME280 / DPS310"},
  {0x77, "BMP280 / BME680"},
  {0x1E, "HMC5883L"},
  {0x68, "MPU6050"},
  {0x5A, "MLX90614"},
  {0x23, "BH1750"},
  {0x57, "MAX30100"},
  {0x0D, "GY271"},
};
const int knownDeviceCount = sizeof(knownDevices) / sizeof(knownDevices[0]);

const char* identifyDevice(uint8_t address) {
  for (int i = 0; i < knownDeviceCount; i++) {
    if (knownDevices[i].address == address) {
      return knownDevices[i].name;
    }
  }
  return "unbekanntes Geraet";
}

// Akkuspannung auslesen mit Kalibrierung
float readBatteryVoltage() {
  int raw = analogRead(BAT_ADC);
  float voltage = (raw / 4095.0) * 3.3 * 2.0;
  return voltage * 1.070;  // Kalibrierung (z. B. bei 4.07 V → 3.97 V Anzeige)
}

// Akkuzustand anzeigen
void drawBatteryStatus() {
  float voltage = readBatteryVoltage();
  float percentF = (voltage - 3.0) / (4.2 - 3.0) * 100.0;
  percentF = constrain(percentF, 0.0, 100.0);
  int percent = round(percentF);

  // Akkusymbol (unten rechts)
  int x = 180, y = 115;
  int w = 40, h = 15;
  int fill = map(percent, 0, 100, 0, w - 6);

  uint16_t fillColor = TFT_RED;
  if (percent > 70) fillColor = TFT_GREEN;
  else if (percent > 30) fillColor = TFT_YELLOW;

  // Rechteck leeren
  tft.fillRect(0, y - 2, 240, h + 4, TFT_BLACK);

  // Spannung links
  char voltStr[10];
  sprintf(voltStr, "%.2fV", voltage);
  tft.setTextDatum(BL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString(voltStr, 5, y + h);

  // Prozentanzeige daneben
  char batStr[8];
  sprintf(batStr, "%d%%", percent);
  tft.drawString(batStr, 85, y + h);

  // Akkusymbol rechts
  tft.drawRect(x, y, w, h, TFT_WHITE);
  tft.fillRect(x + w, y + 4, 3, h - 8, TFT_WHITE);  // Pluspol
  tft.fillRect(x + 3, y + 3, fill, h - 6, fillColor);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Titelzeile
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("I2C-Scan laeuft...", 120, 20);
  delay(1000);

  // I2C-Gerätesuche
  uint8_t foundAddress = 0;
  bool found = false;

  for (uint8_t address = 1; address < 127; address++) {
    if (address == 0x3C) continue;  // OLED ignorieren

    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      foundAddress = address;
      found = true;
      break;
    }
    delay(10);
  }

  // Anzeige I2C-Ergebnis
  tft.fillScreen(TFT_BLACK);
  if (found) {
    char addrStr[20];
    sprintf(addrStr, "Adresse: 0x%02X", foundAddress);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString(addrStr, 120, 30);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString(identifyDevice(foundAddress), 120, 70);
  } else {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("Kein Geraet", 120, 50);
  }

  // Erste Akkuanzeige
  drawBatteryStatus();
}

void loop() {
  drawBatteryStatus();
  delay(2000);  // alle 2 Sekunden aktualisieren
}
