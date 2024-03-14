#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>

#include "pins.h"
#include "Sensors.h"
#include "font.h"
#include "bg.h"
#include "bg_nums.h"
#include "DrawUtils.h"

#define I2C_SDA 17
#define I2C_SCL 18

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite backbuffer = TFT_eSprite(&tft);

//-------------Sensors----------------------------------------
using namespace ADS1X15;

ADS1115<TwoWire> ads(Wire);

//------------------------------------------------------------
double tempResistance[] = {900,1170, 1510, 1970, 2550, 3320, 4430, 6140, 8610, 12320, 17950, 26650, 40340, 62320};
double tempValues[] = {150, 140, 130, 120, 110, 100, 90, 80, 70, 60, 50, 40, 30, 20};
uint16_t tempLen = 14;

AnalogSensor waterTempSensor(tempResistance, tempValues, tempLen, 3.29, 1200, 20, &ads, (uint8_t)0, AnalogSensor::SensorType::RESISTIVE);
AnalogSensor oilTempSensor(tempResistance, tempValues, tempLen, 3.29, 47000, 20, &ads, (uint8_t)1, AnalogSensor::SensorType::RESISTIVE);

//------------------------------------------------------------

double fuelPresVoltage[] = {0.5,4.5};
double fuelPresValues[] = {0, 6.89};//100psi sensor
uint16_t fuelPresLen = 2;

double oilPresVoltage[] = {0,5};
double oilPresValues[] = {0, 10.34};//150psi sensor
uint16_t oilPresLen = 2;

AnalogSensor fuelPressureSensor(fuelPresVoltage, fuelPresValues, fuelPresLen, -1, -1, 2, &ads, 2,  AnalogSensor::SensorType::VOLTAGE);
AnalogSensor oilPressureSensor(oilPresVoltage, oilPresValues, oilPresLen, -1, -1, 0, &ads, 3, AnalogSensor::SensorType::VOLTAGE);
//------------------------------------------------------------


void setBrightness(uint8_t value)
{
    static uint8_t steps = 16;
    static uint8_t _brightness = 0;

    if (_brightness == value) {
        return;
    }

    if (value > 16) {
        value = 16;
    }
    if (value == 0) {
        digitalWrite(BK_LIGHT_PIN, 0);
        delay(3);
        _brightness = 0;
        return;
    }
    if (_brightness == 0) {
        digitalWrite(BK_LIGHT_PIN, 1);
        _brightness = steps;
        delayMicroseconds(30);
    }
    int from = steps - _brightness;
    int to = steps - value;
    int num = (steps + to - from) % steps;
    for (int i = 0; i < num; i++) {
        digitalWrite(BK_LIGHT_PIN, 0);
        digitalWrite(BK_LIGHT_PIN, 1);
    }
    _brightness = value;
}

void ScanI2C()
{
  Wire.begin();
  int nDevices = 0;
  Serial.println("Scanning...");

  for (uint8_t address = 1; address < 127; address++)
  {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    Serial.println(address, HEX);
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");
      nDevices++;
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

void UpdateSensors()
{
    oilPressureSensor.UpdateSensor();
    fuelPressureSensor.UpdateSensor();
    oilTempSensor.UpdateSensor();
    waterTempSensor.UpdateSensor();
}

void InitTft()
{
    tft.begin();
    tft.init();
    tft.setRotation(1);

    backbuffer.createSprite(320, 240);
    backbuffer.setSwapBytes(true);
    setBrightness(16);
}

void InitI2C()
{
    Wire.setPins(I2C_SDA, I2C_SCL);
    //ScanI2C();

    ads.begin();
    ads.setGain(Gain::ONE_4096MV);
    ads.setDataRate(Rate::ADS1115_250SPS);
}

void setup()
{
    Serial.begin(9600);
    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, HIGH);

    InitTft();
    InitI2C();
}

unsigned long lastFrameTime = 0;
float FPS = 0;
void DrawFPS()
{
    unsigned long time = millis();
    uint32_t frameTime = time - lastFrameTime;
    lastFrameTime = time;
    FPS = (FPS * 9 + 1000.f/frameTime) / 10;

    backbuffer.setTextDatum(TL_DATUM);
    backbuffer.loadFont(Square721_25);
    backbuffer.drawString(String(FPS), 0, 0);
}
uint16_t percent = 0;

void DrawMainScreen()
{
    if(percent == 100 || percent == 0)
    {
        delay(3000);
    }
    

    backbuffer.pushImage(0,0, 320, 240, bg);
    backbuffer.loadFont(Square721_44);
    backbuffer.setTextColor(TFT_WHITE);
    backbuffer.setTextDatum(TR_DATUM);

    backbuffer.drawString(waterTempSensor.StrValue(0), 186, 24);
    backbuffer.drawString(oilTempSensor.StrValue(0), 186, 95);
    backbuffer.drawString(oilPressureSensor.StrValue(1), 186, 156);

    percent++;
    percent %= 101;
    DrawProgressBar(backbuffer, P_VERTICAL, 257, 183, 16, 155, 
    10, 3, 2, 
    percent, TFT_WHITE, TFT_DARKGREY);

    backbuffer.loadFont(Square721_25);
    backbuffer.setTextDatum(BC_DATUM);
    backbuffer.drawString(String(percent) + "%", 277, 231);
}

void DrawDevScreen()
{

    backbuffer.fillScreen(TFT_BLACK);
    backbuffer.drawLine(160, 0, 160, 240, TFT_DARKGREY);
    backbuffer.drawLine(0, 130, 320, 130, TFT_DARKGREY);

    backbuffer.loadFont(Square721_25);
    backbuffer.setTextColor(TFT_WHITE);
    backbuffer.setTextDatum(TL_DATUM);

    backbuffer.drawString("Water temp:", 0, 20);
    backbuffer.drawString("Oil Temp:", 0, 145);
    backbuffer.drawString("Fuel Press:", 162, 20);
    backbuffer.drawString("Oil Press:", 162, 145);

    backbuffer.setTextColor(TFT_BLUE);
    backbuffer.drawString(waterTempSensor.StrValueRaw(2) + " *C", 0, 45);
    backbuffer.drawString(oilTempSensor.StrValueRaw(2) + " *C", 0, 170);
    backbuffer.drawString(fuelPressureSensor.StrValueRaw(2) + " Bar", 162, 45);
    backbuffer.drawString(oilPressureSensor.StrValueRaw(2) + " Bar", 162, 170);

    backbuffer.setTextColor(TFT_YELLOW);
    backbuffer.drawString(waterTempSensor.StrResistance() + " R", 0, 70);
    backbuffer.drawString(oilTempSensor.StrResistance() + " R", 0, 195);

    backbuffer.setTextColor(TFT_GREEN);
    backbuffer.drawString(waterTempSensor.StrVoltage() + " V", 0, 95);
    backbuffer.drawString(oilTempSensor.StrVoltage() + " V", 0, 220);
    backbuffer.drawString(fuelPressureSensor.StrVoltage() + " V", 162, 95);
    backbuffer.drawString(oilPressureSensor.StrVoltage() + " V", 162, 220);
}

void loop()
{
    UpdateSensors();

    DrawMainScreen();
    //DrawDevScreen();
    DrawFPS();

    backbuffer.pushSprite(0,0);
}