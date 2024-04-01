#pragma once
#include "sensors.h"
#include <functional>

#define web_ssid "DashTHMI"
#define web_password "12345678"

void WebConfSetSensors(std::vector<AnalogSensor*> sens);
void WebConfStartWiFiAP();
void WebConfStopWiFiAP();
void WebConfListenClients();
bool WebConfIsClientConnected();
void WebConfInit();
void WebConfOnSave(std::function<void(void)> cb);