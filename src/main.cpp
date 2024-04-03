#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <vector>
#include <EEPROM.h>

#include "pins.h"
#include "Sensors.h"
#include "fonts.h"
#include "bg1.h"
#include "bg2.h"
#include "DrawUtils.h"
#include "WebConfigurator.h"

#define EEP_INIT_ADDR 0
#define EEP_START_ADDR 1
#define EEP_INIT_VAL 55

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite backbuffer = TFT_eSprite(&tft);

enum class Mode : uint8_t
{
    MAIN1 = 0,
    MAIN2,
    DEBUG,
    WiFi
};

Mode mode = Mode::MAIN1;
//------------------------------------------------------------
bool buttonIsDown = false;
u_long buttonDownTimestamp = 0;
const u_long buttonDebugPressTime = 5000;
//------------------------------------------------------------
//Time before warnings are enabled after ignition is turned on.
//This is to avoid low voltage warning when starting the car.
const u_long warningDelayTime = 10000;
u_long startTime = 0;   
//------------------------------------------------------------

bool Blink(uint16_t period)
{
    return millis() % period > (period>>1);
}

void LoadSensorsWarnings()
{
    uint16_t addr = EEP_START_ADDR;
    const uint16_t settingsSize = sizeof(AnalogSensor::WarningSettings);
    for(auto sensor : sensorsWithWarnings)
    {
        EEPROM.readBytes(addr, &(sensor->warningSettings), settingsSize);
        addr += settingsSize;
    }
}

void SaveSensorsWarnings()
{
    uint16_t addr = EEP_START_ADDR;
    const uint16_t settingsSize = sizeof(AnalogSensor::WarningSettings);
    for(auto sensor : sensorsWithWarnings)
    {
        EEPROM.put(addr, sensor->warningSettings);
        addr += settingsSize;
    }
    EEPROM.commit();
}

void InitSensorsWarnings()
{  

    sensorsWithWarnings.push_back(&waterTempSensor);
    sensorsWithWarnings.push_back(&oilTempSensor);
    sensorsWithWarnings.push_back(&oilPressureSensor);
    sensorsWithWarnings.push_back(&fuelPressureSensor);
    sensorsWithWarnings.push_back(&fuelLevelSensor);
    sensorsWithWarnings.push_back(&voltage12v);

    uint16_t eepSize = sensorsWithWarnings.size() * sizeof(AnalogSensor::WarningSettings) + 1;
    EEPROM.begin(eepSize);
    uint8_t initByte = EEPROM.read(EEP_INIT_ADDR);

    if (initByte == EEP_INIT_VAL)
    {
        LoadSensorsWarnings();
    }
    else
    {
        //initialize default warning conditions
        AnalogSensor::WarningSettings ws;
        ws.condition = AnalogSensor::WarningCondition::ABOVE;
        ws.threshold = 105;
        waterTempSensor.SetWarningSettings(ws);

        ws.condition = AnalogSensor::WarningCondition::ABOVE;
        ws.threshold = 120;
        oilTempSensor.SetWarningSettings(ws);

        ws.condition = AnalogSensor::WarningCondition::BELOW;
        ws.threshold = 0.5;
        oilPressureSensor.SetWarningSettings(ws);

        ws.condition = AnalogSensor::WarningCondition::BELOW;
        ws.threshold = 3.0;
        fuelPressureSensor.SetWarningSettings(ws);

        ws.condition = AnalogSensor::WarningCondition::BELOW;
        ws.threshold = 3.0;
        fuelPressureSensor.SetWarningSettings(ws);

        ws.condition = AnalogSensor::WarningCondition::BELOW;
        ws.threshold = 12.5;
        voltage12v.SetWarningSettings(ws);

        EEPROM.write(EEP_INIT_ADDR, EEP_INIT_VAL); //mark EEP as initialized

        SaveSensorsWarnings();
    }

}

void SetBrightness(uint8_t value)
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
    SetBrightness(16);
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
    if(millis() - startTime < warningDelayTime)
    {
        return;
    }

    bool bHasAnyWarning = oilTempSensor.IsWarning() ||
        waterTempSensor.IsWarning() ||
        voltage12v.IsWarning() ||
        fuelLevelSensor.IsWarning() ||
        fuelPressureSensor.IsWarning() ||
        oilPressureSensor.IsWarning();

    if (mode != Mode::DEBUG && mode != Mode::WiFi && bHasAnyWarning)
    {
        bool blinkState = Blink(1000);
        digitalWrite(PIN_WARNING, blinkState ? HIGH : LOW);
    }
    else
    {
        digitalWrite(PIN_WARNING, LOW);
    }
}

void InitWebConfig()
{
    WebConfInit();
    WebConfOnSave(SaveSensorsWarnings);
    WebConfSetSensors(sensorsWithWarnings);
}

void setup()
{
    startTime = millis();
    Serial.begin(9600);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    pinMode(PIN_WARNING, OUTPUT);

    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, HIGH);

    InitTft();
    InitI2C();
    InitSensorsWarnings();

    UpdateSensors();
    InitWebConfig();
}

void DrawFPS()
{
    static unsigned long lastFrameTime = 0;
    static float avgFrameTime = 0;
    static float FPS = 0;

    unsigned long time = millis();
    uint32_t frameTime = time - lastFrameTime;
    lastFrameTime = time;
    
    avgFrameTime = (avgFrameTime * 9 + frameTime) / 10;
    FPS = 1000/avgFrameTime;

    backbuffer.setTextDatum(TL_DATUM);
    backbuffer.loadFont(font_Arial_16);
    backbuffer.drawString(String(FPS), 16, 16);
}

void DrawMainScreen()
{
    bool blinkRed = Blink(1000) && (millis() - startTime > warningDelayTime);

    backbuffer.setTextColor(TFT_WHITE);
    backbuffer.setTextDatum(TR_DATUM);
    backbuffer.loadFont(font_Square721_44);
    switch (mode)
    {
    case Mode::MAIN1:
        backbuffer.pushImage(0,0, 320, 240, bg1);
        backbuffer.setTextColor(blinkRed && oilTempSensor.IsWarning() ? TFT_RED : TFT_WHITE);
        backbuffer.drawString(oilTempSensor.StrValue(0), 186, 95);

        backbuffer.setTextColor(blinkRed && oilPressureSensor.IsWarning() ? TFT_RED : TFT_WHITE);
        backbuffer.drawString(oilPressureSensor.StrValue(1), 186, 156);
        break;

    case Mode::MAIN2:
        backbuffer.pushImage(0,0, 320, 240, bg2);
        backbuffer.setTextColor(blinkRed && fuelPressureSensor.IsWarning() ? TFT_RED : TFT_WHITE);
        backbuffer.drawString(fuelPressureSensor.StrValue(1), 186, 95);
        
        backbuffer.setTextColor(blinkRed && voltage12v.IsWarning() ? TFT_RED : TFT_WHITE);
        backbuffer.drawString(voltage12v.StrValue(1), 198, 156);
        break;
    
    default:
        break;
    }

    backbuffer.setTextColor(blinkRed && waterTempSensor.IsWarning() ? TFT_RED : TFT_WHITE);
    backbuffer.drawString(waterTempSensor.StrValue(0), 186, 24);

    DrawProgressBar(backbuffer, P_VERTICAL, 257, 183, 16, 155, 
    10, 3, 2,
    (int)fuelLevelSensor.Value(), TFT_WHITE, TFT_DARKGREY);

    backbuffer.loadFont(font_Square721_25);
    backbuffer.setTextDatum(BC_DATUM);
    backbuffer.setTextColor(blinkRed && fuelLevelSensor.IsWarning() ? TFT_RED : TFT_WHITE);
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

    backbuffer.loadFont(font_Arial_16);
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

void DrawWiFiScreen()
{
    backbuffer.loadFont(font_Arial_16);
    backbuffer.setTextColor(TFT_WHITE);
    backbuffer.setTextDatum(CC_DATUM);

    backbuffer.fillSprite(TFT_BLACK);
    if(WebConfIsClientConnected())
    {
        backbuffer.drawString("WiFi client is connected", 160, 75);
        backbuffer.drawString("Open 192.168.4.1 in your browser", 160, 100);
        backbuffer.drawString("to configure the dashboard", 160, 125);
    }
    else
    {
        backbuffer.drawString("Listening for Wi-Fi connections", 160, 75);
        backbuffer.drawString("SSID: " + String(web_ssid), 160, 100);
        backbuffer.drawString("Password: " + String(web_password) , 160, 125);
    }
}

void UpdateButton()
{
    if(digitalRead(PIN_BUTTON) == 0)
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
                mode = Mode::MAIN1;
                break;
            case Mode::DEBUG:
                mode = Mode::WiFi;
                WebConfStartWiFiAP();
                break;
            case Mode::WiFi:
                mode = Mode::MAIN1;
                WebConfStopWiFiAP();
                break;
            }
        }
        else 
        {
            if(millis() > buttonDebugPressTime + buttonDownTimestamp)
            {
                mode = Mode::DEBUG;
            }
        }
    }
    else
    {
        buttonIsDown = false;
    }
}

void Draw()
{
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
        case Mode::WiFi:
            DrawWiFiScreen();
            break;
        }
        //DrawFPS();
    }
}

void loop()
{
    UpdateButton();
    if(mode == Mode::WiFi)
    {
        WebConfListenClients();
    }
    else
    {
        UpdateSensors();
        UpdateWarnings();
    }
    Draw();

    backbuffer.pushSprite(0,0);
}