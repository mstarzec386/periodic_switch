#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// Define constants
#define RELAY_PIN D2  // Relay connected to D2
#define POT_PIN A0    // Potentiometer connected to A0
#define BUTTON_PIN D7 // Button to enable AP mode
#define EEPROM_INTERVAL_ADDR 0
#define EEPROM__RELAY_ON_ADDR 1
#define EEPROM_SIZE 4096

const char *apSSID = "Relay";
const char *apPassword = "12345678";
ESP8266WebServer server(80);

unsigned long previousMillis = 0;
bool relayOn = false;
unsigned long relayOnTime = 0;
unsigned long relayStartMillis = 0;
unsigned long INTERVAL = 28800000;  // Default 8 hours
unsigned long MAX_RELAY_ON = 10000; // Default 10s

void saveIntervalToEEPROM(unsigned long interval)
{
    EEPROM.put(EEPROM_INTERVAL_ADDR, interval);
    EEPROM.commit();
}

void loadIntervalFromEEPROM()
{
    EEPROM.get(EEPROM_INTERVAL_ADDR, INTERVAL);
    if (INTERVAL == 0xFFFFFFFF)
    { // Uninitialized EEPROM
        INTERVAL = 28800000;
        saveIntervalToEEPROM(INTERVAL);
    }
}

void saveMaxRelayOnToEEPROM(unsigned long time)
{
    EEPROM.put(EEPROM__RELAY_ON_ADDR, time);
    EEPROM.commit();
}

void loadRelayOnFromEEPROM()
{
    EEPROM.get(EEPROM__RELAY_ON_ADDR, MAX_RELAY_ON);
    if (MAX_RELAY_ON == 0xFFFFFFFF)
    { // Uninitialized EEPROM
        MAX_RELAY_ON = 10000;
        saveMaxRelayOnToEEPROM(MAX_RELAY_ON);
    }
}

void handleRoot()
{
    String html = "<html><body>"
                  "<h2>Set Relay Interval (ms)</h2>"
                  "<p>"
                  "<form action='/set_interval' method='POST'>"
                  "<input type='number' name='interval' min='1000' step='1000' value='" +
                  String(INTERVAL) + "'>"
                                     "<input type='submit' value='Set Interval'>"
                                     "</form>"
                                     "<h2>Set Max Relay on (ms)</h2>"
                                     "<p>"
                                     "<form action='/set_relay_on' method='POST'>"
                                     "<input type='number' name='relay_on' min='500' step='1000' value='" +
                  String(MAX_RELAY_ON) + "'>"
                                         "<input type='submit' value='Set Max Relay On'>"
                                         "</form>"
                                         "</body></html>";
    server.send(200, "text/html", html);
}

void handleSetInterval()
{
    if (server.hasArg("interval"))
    {
        INTERVAL = server.arg("interval").toInt();
        saveIntervalToEEPROM(INTERVAL);
    }
    server.send(200, "text/html", "<html><body><h2>Interval updated!</h2><a href='/'>Go Back</a></body></html>");
}

void handleSetRelayOn()
{
    if (server.hasArg("relay_on"))
    {
        MAX_RELAY_ON = server.arg("relay_on").toInt();
        saveIntervalToEEPROM(MAX_RELAY_ON);
    }
    server.send(200, "text/html", "<html><body><h2>Relay On updated!</h2><a href='/'>Go Back</a></body></html>");
}

void setup()
{
    Serial.begin(9600);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(RELAY_PIN, LOW); // Ensure relay is off at start

    EEPROM.begin(EEPROM_SIZE);
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
    server.begin();
    Serial.println("Web server started");
}

void loop()
{
    ArduinoOTA.handle();   // Handle OTA updates
    server.handleClient(); // Handle web requests
    unsigned long currentMillis = millis();

    if ((!relayOn && (currentMillis - previousMillis >= INTERVAL)) || digitalRead(BUTTON_PIN) == LOW)
    {
        previousMillis = currentMillis;

        // Read potentiometer value (0 - 1023)
        int potValue = analogRead(POT_PIN);
        relayOnTime = map(potValue, 0, 1023, 500, 10000);

        // Turn relay ON
        digitalWrite(RELAY_PIN, HIGH);
        relayOn = true;
        relayStartMillis = currentMillis;
    }

    // Turn relay OFF after relayOnTime has passed
    if (relayOn && (currentMillis - relayStartMillis >= relayOnTime))
    {
        digitalWrite(RELAY_PIN, LOW);
        relayOn = false;
    }
}
