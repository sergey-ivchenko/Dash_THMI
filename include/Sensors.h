#pragma once
#include "AnalogSensor.h"

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

AnalogSensor waterTempSensor("Water temp",tempResistance, tempValues, tempLen, sensorsPullup, 10000/*Ohm (9900?)*/, 20, &ads0, 0, AnalogSensor::Type::RESISTIVE);
AnalogSensor oilTempSensor("Oil temp",tempResistance, tempValues, tempLen, sensorsPullup, 10000/*Ohm (9900?)*/, 20, &ads0, 1, AnalogSensor::Type::RESISTIVE);
AnalogSensor voltage12v("Voltage", voltage12VVoltage, voltage12VValues, voltage12VLen, -1, -1, 0, &ads0, 3, AnalogSensor::Type::VOLTAGE);
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

AnalogSensor fuelLevelSensor("Fuel level", fuelLevelResistance, fuelLevelValues, fuelLevelLen, sensorsPullup, 200/*Ohm*/, 500, &ads1, 0,  AnalogSensor::Type::RESISTIVE);
AnalogSensor fuelPressureSensor("Fuel Pressure", fuelPresVoltage, fuelPresValues, fuelPresLen, -1, -1, 0, &ads1, 1,  AnalogSensor::Type::VOLTAGE);
AnalogSensor oilPressureSensor("Oil Pressure", oilPresVoltage, oilPresValues, oilPresLen, -1, -1, 0, &ads1, 2, AnalogSensor::Type::VOLTAGE);
AnalogSensor voltage5v("Voltage5V", voltage5VValues, voltage5VValues, voltage5VLen, -1, -1, 0, &ads1, 3, AnalogSensor::Type::VOLTAGE);
//------------------------------------------------------------
std::vector<AnalogSensor*> sensorsWithWarnings;
//------------------------------------------------------------