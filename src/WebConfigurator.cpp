#include <WiFi.h>
#include <vector>
#include <WebServer.h>
#include "AnalogSensor.h"
#include "WebConfigurator.h"

static WebServer server(80);
static std::vector<AnalogSensor*> sensors;
static std::function<void(void)> onSaveCallback;

const char* pageHeader = R"""(
<!DOCTYPE html>
<html>
<head>
<style>
html { font-family: monospace; text-align: center; background-color: black; color: white;}
table {border: 4pt solid #888888; border-radius: 10pt;}
th {height: 80pt; font-size: 24pt; text-align: Center; vertical-align: top;}
td {height: 80pt; font-size: 24pt; text-align: Center; border-top: 2px dotted silver; padding: 5pt;}

input[type=submit]
{
    background-color: #04AA6D;
    border: none;
    border-radius: 10pt;
    color: white;
    padding: 14pt;
    font-size: 24pt;
    text-decoration: none;
}

input[type="number"]
{
    background-color: black; vertical-align: top; outline: none; color: silver;
    height: 50pt; 
    text-indent: 10pt;
    width: 120pt;
    font-size: 24pt;
    border-radius: 10pt;
} 

select
{
	background-color: black; vertical-align: top; outline: none; color: silver;
	height: 50pt; 
	text-indent: 10pt;
	width: 160pt;
	font-size: 24pt;
	border-radius: 10pt;
} 
</style>
</head>

<body><h1>Dash T-HMI warnings configuration</h1>
     
<div align='center'>
<form action='save' method='post' enctype='application/x-www-form-urlencoded'>
<table><tr><th>Sensor</th><th>Warning<br> Value</th><th>Warning<br> Condition</th></tr>)""";

const char* pageFooter = R"""(
</table>
<br>
<div align='center'><input type='submit' value='Save'/></div>
</form>
</div>
</body></html>)""";


const char* savePageContent = R"""(
<!DOCTYPE html>
<html>
<head>
    <meta http-equiv="refresh"
          content="2; url =/" />
<style>
html { font-family: monospace; text-align: center; background-color: #202020; color: white;}
</style>
</head>
<body>
    <div align='center'><h1>Saved.</h1></body>
</html>

    )""";

void HandleSave() 
{
    uint8_t sensorIndex = 0;
    for(auto sensor:sensors)
    {
        String th = server.arg("th" + String(sensorIndex));
        sensor->warningSettings.threshold = th.toDouble();

        String cond = server.arg("cond" + String(sensorIndex));
        sensor->warningSettings.condition = (AnalogSensor::WarningCondition)cond.toInt();

        sensorIndex++;
    }
    server.send(200, "text/html", savePageContent);
    onSaveCallback();
}

void HandleIndex() 
{
    String content;
    content += pageHeader;
    uint8_t sensorIndex = 0;
    for(auto sensor:sensors)
    {
        content += "<tr><td>";
        content += sensor->Name();
        content += "</td><td><input type='number' name='th";
        content += String(sensorIndex);
        content += "' min='0' max='150' step='0.1' value ='";
        content += String(sensor->warningSettings.threshold, 1);
        content += "'></td><td><select name='cond";
        content += String(sensorIndex);
        content += "'>";
        
        content += "<option value='0'";
        if(((uint8_t)sensor->warningSettings.condition) == 0)
        {
            content += " selected";
        }
        content += ">Disabled</option>";

        content += "<option value='1'";
        if(((uint8_t)sensor->warningSettings.condition) == 1)
        {
            content += " selected";
        }
        content += ">Below</option>";

        content += "<option value='2'";
        if(((uint8_t)sensor->warningSettings.condition) == 2)
        {
            content += " selected";
        }
        content += ">Above</option>";
        content += "</select></td></tr>";
        sensorIndex++;
    }
    content += pageFooter;
    server.send(200, "text/html", content);
}

void WebConfInit() 
{     
    server.on("/save", HTTP_POST, HandleSave); 
    server.on("/", HTTP_GET, HandleIndex);     
}

void WebConfStartWiFiAP()
{
    WiFi.softAP(web_ssid, web_password);
    server.begin();
}

void WebConfStopWiFiAP()
{
    server.stop();
    WiFi.softAPdisconnect(true);
}

void WebConfSetSensors(std::vector<AnalogSensor*> sens)
{
   sensors = sens;
}

void WebConfListenClients()
{
    server.handleClient();
}

bool WebConfIsClientConnected()
{
    if(WiFi.softAPgetStationNum())
        return true;

    return false;
}

void WebConfOnSave(std::function<void(void)> cb)
{
    onSaveCallback = cb;
}