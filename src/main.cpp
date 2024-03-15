#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>

#include "pins.h"
#include "Sensors.h"
#include "font.h"
#include "bg.h"
#include "bg_nums.h"
#include "DrawUtils.h"

#define I2C_SDA 43
#define I2C_SCL 44

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite backbuffer = TFT_eSprite(&tft);
const double sensorsPullup = 5.0;
//-------------Sensors----------------------------------------
using namespace ADS1X15;
//------------------------------------------------------------
//ads0 sensors
ADS1115<TwoWire> ads0(Wire);//addr 2 gnd

double tempResistance[] = {900,1170, 1510, 1970, 2550, 3320, 4430, 6140, 8610, 12320, 17950, 26650, 40340, 62320};
double tempValues[] = {150, 140, 130, 120, 110, 100, 90, 80, 70, 60, 50, 40, 30, 20};
uint16_t tempLen = 14;

double voltage12VVoltage[] = {0,6};
double voltage12VValues[] = {0,6};
uint16_t voltage12VLen = 2;

AnalogSensor waterTempSensor(tempResistance, tempValues, tempLen, sensorsPullup, 10000/*Ohm*/, 20, &ads0, 0, AnalogSensor::SensorType::RESISTIVE);
AnalogSensor oilTempSensor(tempResistance, tempValues, tempLen, sensorsPullup, 10000/*Ohm*/, 20, &ads0, 1, AnalogSensor::SensorType::RESISTIVE);
AnalogSensor voltage12v(voltage12VVoltage, voltage12VValues, voltage12VLen, -1, -1, 0, &ads0, 3, AnalogSensor::SensorType::VOLTAGE);
//------------------------------------------------------------
//ads1 sensors
ADS1115<TwoWire> ads1(Wire);//addr 2 vcc

double fuelLevelResistance[] = {5, 32, 110};//stock EF fuel sensor
double fuelLevelValues[] = {100, 50, 0};
uint16_t fuelLevelLen = 3;

double fuelPresVoltage[] = {0.5, 4.5};
double fuelPresValues[] = {0, 6.89};//100psi sensor
uint16_t fuelPresLen = 2;

double oilPresVoltage[] = {0.5, 4.5};
double oilPresValues[] = {0, 10.34};//150psi sensor
uint16_t oilPresLen = 2;

double voltage5VValues[] = {0,6};
uint16_t voltage5VLen = 2;

AnalogSensor fuelLevelSensor(fuelLevelResistance, fuelLevelValues, fuelLevelLen, sensorsPullup, 200/*Ohm*/, 500, &ads1, 0,  AnalogSensor::SensorType::RESISTIVE);
AnalogSensor fuelPressureSensor(fuelPresVoltage, fuelPresValues, fuelPresLen, -1, -1, 2, &ads1, 1,  AnalogSensor::SensorType::VOLTAGE);
AnalogSensor oilPressureSensor(oilPresVoltage, oilPresValues, oilPresLen, -1, -1, 0, &ads1, 2, AnalogSensor::SensorType::VOLTAGE);
AnalogSensor voltage5v(voltage5VValues, voltage5VValues, voltage5VLen, -1, -1, 0, &ads1, 3, AnalogSensor::SensorType::VOLTAGE);
//------------------------------------------------------------

void InitWarnings()
{
    AnalogSensor::WarningSettings ws;

    ws.condition = AnalogSensor::WarningCondition::ABOVE;
    ws.threshold = 105;
    waterTempSensor.SetWarningSettings(ws);

    ws.condition = AnalogSensor::WarningCondition::ABOVE;
    ws.threshold = 125;
    oilTempSensor.SetWarningSettings(ws);

    ws.condition = AnalogSensor::WarningCondition::BELOW;
    ws.threshold = 0.5;
    oilPressureSensor.SetWarningSettings(ws);

    ws.condition = AnalogSensor::WarningCondition::BELOW;
    ws.threshold = 3.0;
    fuelPressureSensor.SetWarningSettings(ws);
}

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
    oilTempSensor.UpdateSensor();
    waterTempSensor.UpdateSensor();
    voltage12v.UpdateSensor();

    fuelLevelSensor.UpdateSensor();
    fuelPressureSensor.UpdateSensor();
    oilPressureSensor.UpdateSensor();
    voltage5v.UpdateSensor();
    
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

    ads0.begin(0x48);   //addr pin to ground
    ads0.setGain(Gain::TWOTHIRDS_6144MV);
    ads0.setDataRate(Rate::ADS1115_250SPS);

    ads1.begin(0x49);   //addr pin to vcc
    ads1.setGain(Gain::TWOTHIRDS_6144MV);
    ads1.setDataRate(Rate::ADS1115_250SPS);
}

void UpdateWarnings()
{

    if (oilTempSensor.IsWarning() ||
        waterTempSensor.IsWarning() ||
        voltage12v.IsWarning() ||
        fuelLevelSensor.IsWarning() ||
        fuelPressureSensor.IsWarning() ||
        oilPressureSensor.IsWarning())
    {
        //light up warning led or activate buzzer
    }
}

void setup()
{
    Serial.begin(9600);
    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, HIGH);

    InitTft();
    InitI2C();
    InitWarnings();
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

void DrawDebugScreen()
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
    //DrawDebugScreen();
    DrawFPS();

    backbuffer.pushSprite(0,0);
}