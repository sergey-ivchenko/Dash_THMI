#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>

#include "pins.h"
#include "sensors.h"
#include "fonts.h"
#include "bg1.h"
#include "bg2.h"
#include "DrawUtils.h"

#define I2C_SDA 43
#define I2C_SCL 44

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite backbuffer = TFT_eSprite(&tft);
bool bEnableWarnings = false;
//-------------Sensors----------------------------------------
using namespace ADS1X15;
//------------------------------------------------------------
const double sensorsPullup = 5.0;
//ads0 sensors
ADS1115<TwoWire> ads0(Wire);//addr 2 gnd

double tempResistance[] = {900,1170, 1510, 1970, 2550, 3320, 4430, 6140, 8610, 12320, 17950, 26650, 40340, 62320};
double tempValues[] = {150, 140, 130, 120, 110, 100, 90, 80, 70, 60, 50, 40, 30, 20};
uint16_t tempLen = 14;

double voltage12VVoltage[] = {0,6};
double voltage12VValues[] = {0,22.78};
uint16_t voltage12VLen = 2;

AnalogSensor waterTempSensor(tempResistance, tempValues, tempLen, sensorsPullup, 10000/*Ohm*/, 20, &ads0, 0, AnalogSensor::SensorType::RESISTIVE);
AnalogSensor oilTempSensor(tempResistance, tempValues, tempLen, sensorsPullup, 10000/*Ohm*/, 20, &ads0, 1, AnalogSensor::SensorType::RESISTIVE);
AnalogSensor voltage12v(voltage12VVoltage, voltage12VValues, voltage12VLen, -1, -1, 0, &ads0, 3, AnalogSensor::SensorType::VOLTAGE);
//------------------------------------------------------------
//ads1 sensors
ADS1115<TwoWire> ads1(Wire);

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
AnalogSensor fuelPressureSensor(fuelPresVoltage, fuelPresValues, fuelPresLen, -1, -1, 0, &ads1, 1,  AnalogSensor::SensorType::VOLTAGE);
AnalogSensor oilPressureSensor(oilPresVoltage, oilPresValues, oilPresLen, -1, -1, 0, &ads1, 2, AnalogSensor::SensorType::VOLTAGE);
AnalogSensor voltage5v(voltage5VValues, voltage5VValues, voltage5VLen, -1, -1, 0, &ads1, 3, AnalogSensor::SensorType::VOLTAGE);
//------------------------------------------------------------

enum class Mode : uint8_t
{
    MAIN1 = 0,
    MAIN2,
    DEBUG
};

Mode mode = Mode::MAIN1;
//------------------------------------------------------------

bool buttonIsDown = false;
u_long buttonDownTimestamp = 0;
const u_long buttonLongPressTime = 5000;
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
    voltage5v.UpdateSensor();

    fuelLevelSensor.SetPullUpVoltage(voltage5v.Value());
    oilTempSensor.SetPullUpVoltage(voltage5v.Value());
    waterTempSensor.SetPullUpVoltage(voltage5v.Value());

    fuelPressureSensor.UpdateSensor();
    oilPressureSensor.UpdateSensor();
    fuelLevelSensor.UpdateSensor();


    oilTempSensor.UpdateSensor();
    waterTempSensor.UpdateSensor();
    voltage12v.UpdateSensor();

    
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
    // delay(3000);
    // ScanI2C();

    ads0.begin(0x48);   //addr pin to ground
    ads0.setGain(Gain::TWOTHIRDS_6144MV);
    ads0.setDataRate(Rate::ADS1115_250SPS);

    ads1.begin(0x49);   //addr pin to vcc
    ads1.setGain(Gain::TWOTHIRDS_6144MV);
    ads1.setDataRate(Rate::ADS1115_250SPS);
}

void UpdateWarnings()
{
    if(!bEnableWarnings)
    {
        tft.invertDisplay(false);
    }
    else if (
        oilTempSensor.IsWarning() ||
        waterTempSensor.IsWarning() ||
        voltage12v.IsWarning() ||
        fuelLevelSensor.IsWarning() ||
        fuelPressureSensor.IsWarning() ||
        oilPressureSensor.IsWarning())
    {
        tft.invertDisplay(millis() % 1000 > 500);
    }
}

void setup()
{
    Serial.begin(9600);
    pinMode(BUTTON1_PIN, INPUT_PULLUP);

    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, HIGH);

    InitTft();
    InitI2C();
    InitWarnings();

    UpdateSensors();
}

void DrawFPS()
{
    static unsigned long lastFrameTime = 0;
    static float FPS = 0;

    unsigned long time = millis();
    uint32_t frameTime = time - lastFrameTime;
    lastFrameTime = time;
    FPS = (FPS * 9 + 1000.f/frameTime) / 10;

    backbuffer.setTextDatum(TL_DATUM);
    backbuffer.loadFont(Arial_16);
    backbuffer.drawString(String(FPS), 0, 0);
}

void DrawMainScreen()
{
    backbuffer.setTextColor(TFT_WHITE);
    backbuffer.setTextDatum(TR_DATUM);
    backbuffer.loadFont(Square721_44);
    switch (mode)
    {
    case Mode::MAIN1:
        backbuffer.pushImage(0,0, 320, 240, bg1);
        backbuffer.drawString(oilTempSensor.StrValue(0), 186, 95);
        backbuffer.drawString(oilPressureSensor.StrValue(1), 186, 156);
        break;

    case Mode::MAIN2:
        backbuffer.pushImage(0,0, 320, 240, bg2);
        backbuffer.drawString(fuelPressureSensor.StrValue(1), 186, 95);
        backbuffer.drawString(voltage12v.StrValue(1), 198, 156);
        break;
    
    default:
        break;
    }

    backbuffer.drawString(waterTempSensor.StrValue(0), 186, 24);

    DrawProgressBar(backbuffer, P_VERTICAL, 257, 183, 16, 155, 
    10, 3, 2,
    (int)fuelLevelSensor.Value(), TFT_WHITE, TFT_DARKGREY);

    backbuffer.loadFont(Square721_25);
    backbuffer.setTextDatum(BC_DATUM);
    backbuffer.drawString(fuelLevelSensor.StrValue(0) + "%", 277, 231);
}

void DrawDebugScreen()
{

    const uint16_t x0 = 10;
    const uint16_t x1 = 113;
    const uint16_t x2 = 216;
    uint16_t y = 40;
    uint16_t off = 18;
    backbuffer.fillSprite(TFT_BLACK);
    // backbuffer.drawLine(160, 0, 160, 240, TFT_DARKGREY);
    // backbuffer.drawLine(0, 130, 320, 130, TFT_DARKGREY);

    backbuffer.loadFont(Arial_16);
    backbuffer.setTextColor(TFT_WHITE);
    backbuffer.setTextDatum(TL_DATUM);

    backbuffer.drawString("Water temp:", x0, y);
    backbuffer.drawString("Oil Temp:", x1, y);
    backbuffer.drawString("Fuel level:", x2, y);
    y+=off;

    backbuffer.setTextColor(TFT_YELLOW);
    backbuffer.drawString("R:" + waterTempSensor.StrResistance(), x0, y);
    backbuffer.drawString("R:" + oilTempSensor.StrResistance(), x1, y);
    backbuffer.drawString("R:" + fuelLevelSensor.StrResistance(), x2, y);
    y+=off;

    backbuffer.setTextColor(TFT_SILVER);
    backbuffer.drawString(waterTempSensor.StrValueRaw(2) + " *C", x0, y);
    backbuffer.drawString(oilTempSensor.StrValueRaw(2) + " *C", x1, y);
    backbuffer.drawString(fuelLevelSensor.StrValueRaw(2) + " %", x2, y);
    y+=off;

    backbuffer.setTextColor(TFT_GREEN);
    backbuffer.drawString("U: "+ waterTempSensor.StrVoltage() + " V", x0, y);
    backbuffer.drawString("U: "+ oilTempSensor.StrVoltage() + " V", x1, y);
    backbuffer.drawString("U: "+ fuelLevelSensor.StrVoltage() + " V", x2, y);
    y+=off;

    backbuffer.setTextColor(TFT_DARKGREEN);
    backbuffer.drawString("Up:" + waterTempSensor.StrPullUpVoltage() + " V", x0, y);
    backbuffer.drawString("Up:" + oilTempSensor.StrPullUpVoltage() + " V", x1, y);
    backbuffer.drawString("Up:" + fuelLevelSensor.StrPullUpVoltage() + " V", x2, y);
    y+=off;


    y+=off;
    y+=20;

    backbuffer.setTextColor(TFT_WHITE);
    backbuffer.drawString("Fuel Press:", x0, y);
    backbuffer.drawString("Oil Press:", x1, y);
    backbuffer.drawString("Voltage 12:", x2, y);
    y+=off;

    backbuffer.setTextColor(TFT_SILVER);
    backbuffer.drawString(fuelPressureSensor.StrValueRaw(2) + " Bar", x0, y);
    backbuffer.drawString(oilPressureSensor.StrValueRaw(2) + " Bar", x1, y);
    y+=off;

    backbuffer.setTextColor(TFT_GREEN);
    backbuffer.drawString("U: "+ fuelPressureSensor.StrVoltage() + " V", x0, y);
    backbuffer.drawString("U: "+ oilPressureSensor.StrVoltage() + " V", x1, y);
    backbuffer.drawString("U: "+ voltage12v.StrValueRaw() + " V", x2, y);
}

void UpdateButton()
{
    if(digitalRead(BUTTON1_PIN) == 0)
    {
        if(!buttonIsDown)
        {
            buttonIsDown = true;
            buttonDownTimestamp = millis();
            switch (mode)
            {
            case Mode::MAIN1:
                mode = Mode::MAIN2;
                break;
            case Mode::MAIN2:
            case Mode::DEBUG:
                mode = Mode::MAIN1;
                break;
            }
        }
        else if(buttonLongPressTime + buttonDownTimestamp < millis())
        {
            mode = Mode::DEBUG;
        }
    }
    else
    {
        buttonIsDown = false;
    }
}

void Draw()
{
    switch (mode)
    {
    case Mode::MAIN1:
    case Mode::MAIN2:
        DrawMainScreen();
        break;
    case Mode::DEBUG:
        DrawDebugScreen();
        break;
    }
    //DrawFPS();
}

void loop()
{
    UpdateButton();
    UpdateSensors();
    UpdateWarnings();
    Draw();

    backbuffer.pushSprite(0,0);

    //delay(500);
}