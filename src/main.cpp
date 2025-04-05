#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// Define constants
#define RELAY_PIN D2
#define POT_PIN A0
#define BUTTON_PIN D7
#define EEPROM_INTERVAL_ADDR 0
#define EEPROM_RELAY_ON_ADDR sizeof(unsigned long)
#define EEPROM_SIZE 4096

const char *apSSID = "Relay";
const char *apPassword = "12345678";
ESP8266WebServer server(80);

bool relayOn = false;
bool relayOnButton = false;
unsigned long relayOnTime = 0;
unsigned long relayStartMillis = 0;
unsigned long intervalMs = 28800000;
unsigned long relayOnMs = 12000;

void clearEEPROM()
{
    for (int i = 0; i < EEPROM_SIZE; i++)
    {
        EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
}

void saveIntervalToEEPROM()
{
    EEPROM.put(EEPROM_INTERVAL_ADDR, intervalMs);
    EEPROM.commit();
}

void loadIntervalFromEEPROM()
{
    EEPROM.get(EEPROM_INTERVAL_ADDR, intervalMs);
    if (intervalMs == 0xFFFFFFFF)
    {
        intervalMs = 28800000;
        saveIntervalToEEPROM();
    }
}

void saveRelayOnToEEPROM()
{
    EEPROM.put(EEPROM_RELAY_ON_ADDR, relayOnMs);
    EEPROM.commit();
}

void loadRelayOnFromEEPROM()
{
    EEPROM.get(EEPROM_RELAY_ON_ADDR, relayOnMs);
    if (relayOnMs == 0xFFFFFFFF)
    {
        relayOnMs = 12000;
        saveRelayOnToEEPROM();
    }
}

void handleRoot()
{
    String html = "<html><body>"
                  "<h2>Set Relay Interval (ms)</h2>"
                  "<p>"
                  "<form action='/set_interval' method='POST'>"
                  "<input type='number' name='interval' min='1000' step='1000' value='" +
                  String(intervalMs) + "'>"
                                       "<input type='submit' value='Set Interval'>"
                                       "</form>"
                                       "</p>"
                                       "<p>"
                                       "<h2>Set Max Relay on (ms)</h2>"
                                       "<p>"
                                       "<form action='/set_relay_on' method='POST'>"
                                       "<input type='number' name='relay_on' min='500' step='10' value='" +
                  String(relayOnMs) + "'>"
                                      "<input type='submit' value='Set Relay On'>"
                                      "</form>"
                                      "</p>"
                                      "</body></html>";
    server.send(200, "text/html", html);
}

void handleSetInterval()
{
    if (server.hasArg("interval"))
    {
        intervalMs = server.arg("interval").toInt();
        saveIntervalToEEPROM();
    }
    server.send(200, "text/html", "<html><body><h2>Interval updated!</h2><a href='/'>Go Back</a></body></html>");
}

void handleSetRelayOn()
{
    if (server.hasArg("relay_on"))
    {
        relayOnMs = server.arg("relay_on").toInt();
        saveRelayOnToEEPROM();
    }
    server.send(200, "text/html", "<html><body><h2>Relay On updated!</h2><a href='/'>Go Back</a></body></html>");
}

void handleInfo()
{
    int potValue = analogRead(POT_PIN);
    int delta = map(potValue, 0, 1023, -2000, 2000);
    unsigned long currentMillis = millis();

    server.send(200, "text/html",
                "<html><body>"
                "<table border='1'>"
                "<tr><th>Parameter</th><th>Value</th></tr>"
                "<tr><td>relayOn</td><td>" +
                    String(relayOn) + "</td></tr>"
                                      "<tr><td>currentMillis</td><td>" +
                    String(currentMillis) + "</td></tr>"
                                            "<tr><td>relayStartMillis</td><td>" +
                    String(relayStartMillis) + "</td></tr>"
                                               "<tr><td>intervalMs</td><td>" +
                    String(intervalMs) + "</td></tr>"
                                         "<tr><td>relayOnTime</td><td>" +
                    String(relayOnTime) + "</td></tr>"
                                          "<tr><td>relayOnMs</td><td>" +
                    String(relayOnMs) + "</td></tr>"
                                        "<tr><td>delta</td><td>" +
                    String(delta) + "</td></tr>"
                                    "<tr><td>Button</td><td>" +
                    String(digitalRead(BUTTON_PIN)) + "</td></tr>"
                                                      "<tr><td>currentMillis - relayStartMillis</td><td>" +
                    String(currentMillis - relayStartMillis) + "</td></tr>"
                                                               "</table>"
                                                               "</body></html>");
}

void setup()
{
    Serial.begin(9600);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(RELAY_PIN, LOW);

    EEPROM.begin(EEPROM_SIZE);
    // clearEEPROM();
    loadIntervalFromEEPROM();
    loadRelayOnFromEEPROM();

    WiFi.softAP(apSSID, apPassword);
    Serial.println("AP Mode Enabled");

    // OTA Setup
    ArduinoOTA.setHostname("D1Mini-Relay");
    ArduinoOTA.begin();

    // Start web server
    server.on("/", handleRoot);
    server.on("/set_interval", HTTP_POST, handleSetInterval);
    server.on("/set_relay_on", HTTP_POST, handleSetRelayOn);
    server.on("/info", HTTP_GET, handleInfo);
    server.begin();
    Serial.println("Web server started");
}

void loop()
{
    ArduinoOTA.handle();
    server.handleClient();
    unsigned long currentMillis = millis();

    if (!relayOn && ((currentMillis - relayStartMillis >= intervalMs) || digitalRead(BUTTON_PIN) == LOW))
    {
        relayStartMillis = currentMillis;

        int potValue = analogRead(POT_PIN);
        int delta = map(potValue, 0, 1023, -2000, 2000);
        relayOnTime = constrain(relayOnMs + delta, 500, 60000);

        digitalWrite(RELAY_PIN, HIGH);
        relayOn = true;
    }

    if (relayOn && (currentMillis - relayStartMillis >= relayOnTime))
    {
        digitalWrite(RELAY_PIN, LOW);
        relayOn = false;
    }
}
