#pragma once
#include <functional>

#define web_ssid "DashTHMI"
#define web_password "12345678"

void WebConfSetSensors(std::vector<class AnalogSensor*> sens);
void WebConfStartWiFiAP();
void WebConfStopWiFiAP();
void WebConfListenClients();
bool WebConfIsClientConnected();
void WebConfInit();
void WebConfOnSave(std::function<void(void)> cb);