#pragma once
#include <Wire.h>
#include "ADS1X15.h"

class AnalogSensor
{
public:

    enum class SensorType : uint16_t 
    {
    RESISTIVE = 0,
    VOLTAGE
    };

    enum class WarningCondition : uint8_t 
    {
        BELOW = 0,
        ABOVE
    };

    struct WarningSettings
    {
        WarningCondition condition;
        double threshold;
    };

private:
    float pullupResistor = 1190;
    float pullupVoltage = 3.33;
    uint16_t smoothProbesCount = 10;

    double* inTable = nullptr;
    double* outTable = nullptr;
    uint16_t tableLen = 0;

    double curValue = 0;
    double smoothValue = -999999;
    float curVoltage = 0;

    float curResistance = 45000;
    SensorType type = SensorType::RESISTIVE;

    ADS1X15::ADS1115<TwoWire>* ads;
    uint8_t channel;

    WarningSettings warningSettings;

    bool bWarningEnabled = true;

private:

    double InterpolateLinear(double xValues[], double yValues[], int numValues, double pointX, bool trim)
    {
        if (trim)
        {
            if (pointX <= xValues[0]) return yValues[0];
            if (pointX >= xValues[numValues - 1]) return yValues[numValues - 1];
        }

        auto i = 0;
        double rst = 0;
        if (pointX <= xValues[0])
        {
            i = 0;
            auto t = (pointX - xValues[i]) / (xValues[i + 1] - xValues[i]);
            rst = yValues[i] * (1 - t) + yValues[i + 1] * t;
        }
        else if (pointX >= xValues[numValues - 1])
        {
            auto t = (pointX - xValues[numValues - 2]) / (xValues[numValues - 1] - xValues[numValues - 2]);
            rst = yValues[numValues - 2] * (1 - t) + yValues[numValues - 1] * t;
        }
        else
        {
            while (pointX >= xValues[i + 1]) i++;
            auto t = (pointX - xValues[i]) / (xValues[i + 1] - xValues[i]);
            rst = yValues[i] * (1 - t) + yValues[i + 1] * t;
        }

        return rst;

    }

public:
    AnalogSensor(double* inTable, double* outTable, uint16_t tableLen, float V, float R, uint16_t smooth, ADS1X15::ADS1115<TwoWire>* ads, uint8_t channel, SensorType type = SensorType::RESISTIVE)
    {
        this->outTable = outTable;
        this->inTable = inTable;
        this->tableLen = tableLen;
        this->type = type;
        this->ads = ads;
        this->channel = channel;


        pullupResistor = R;
        pullupVoltage = V;
        smoothProbesCount = smooth;
    }

    void UpdateSensor()
    {
        curVoltage = ads->computeVolts(ads->readADCSingleEnded(channel));

        curResistance = pullupResistor * (1/(pullupVoltage/curVoltage - 1));

        if(!outTable || !tableLen)
        {
            curValue = 0;
        }
        else
        {
            double inValue = type==SensorType::RESISTIVE ? curResistance : curVoltage;
            curValue = InterpolateLinear(inTable, outTable, tableLen, inValue, true);

            //curValue = Interpolation::CatmullSpline(inTable, outTable, tableLen, inValue);
        }

        if(smoothValue < -888888 || smoothProbesCount == 0)
        {
            smoothValue = curValue;
        }
        else
        {
            smoothValue = (smoothValue * smoothProbesCount + curValue)/(smoothProbesCount + 1);
        }
    }

    String StrValue(uint16_t decimals = 2)
    {
        if(type== SensorType::RESISTIVE && (Resistance() > 1000000 || Resistance() < 1))
            return "---";
        return String(smoothValue, decimals);
    }

    String StrValueRaw(uint16_t decimals = 2)
    {
        if(type== SensorType::RESISTIVE)
        {
            if(curVoltage >= pullupVoltage - 0.01) 
            {
                return "Open";
            }
            if(curVoltage < 0.01)
            {
                return "Short";
            }
        }
        return String(curValue, decimals);
    }

    String StrVoltage()
    {
        return String(curVoltage,3);
    }

    String StrResistance()
    {
        return String(curResistance, 0);
    }

    String StrPullUpVoltage()
    {
        return String(pullupVoltage, 3);
    }

    
    
    float Value()
    {
        return smoothValue;
    }

    float ValueRaw()
    {
        return curValue;
    }

    float Voltage()
    {
        return curVoltage;
    }

    float Resistance()
    {
        return curResistance;
    }

    void SetWarningSettings(const WarningSettings& settings)
    {
        warningSettings = settings;
    }

    bool IsWarning()
    {
        if(!bWarningEnabled)
        {
            return false;
        }
        switch (warningSettings.condition)
        {
        case WarningCondition::ABOVE:
            return smoothValue >= warningSettings.threshold;
        case WarningCondition::BELOW:
            return smoothValue <= warningSettings.threshold;
        default:
            return false;
        }
    }

    void SetPullUpVoltage(float v)
    {
        pullupVoltage = v + 0.003; //increase a bit because of adc tolerance
    }
};
