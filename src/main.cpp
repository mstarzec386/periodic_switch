#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// Define constants
#define RELAY_PIN D2  // Relay connected to D2
#define POT_PIN A0    // Potentiometer connected to A0
#define BUTTON_PIN D7 // Button to enable AP mode
#define EEPROM_ADDR 0 // EEPROM address to store interval

const char *apSSID = "Relay";
const char *apPassword = "12345678";
ESP8266WebServer server(80);

unsigned long previousMillis = 0;
bool relayOn = false;
unsigned long relayOnTime = 0;
unsigned long relayStartMillis = 0;
unsigned long INTERVAL = 28800000; // Default 8 hours

void saveIntervalToEEPROM(unsigned long interval)
{
    EEPROM.begin(4);
    EEPROM.put(EEPROM_ADDR, interval);
    EEPROM.commit();
}

void loadIntervalFromEEPROM()
{
    EEPROM.begin(4);
    EEPROM.get(EEPROM_ADDR, INTERVAL);
    if (INTERVAL == 0xFFFFFFFF)
    { // Uninitialized EEPROM
        INTERVAL = 28800000;
        saveIntervalToEEPROM(INTERVAL);
    }
}

void handleRoot()
{
    String html = "<html><body>"
                  "<h2>Set Relay Interval (ms)</h2>"
                  "<p>"
                  "<form action='/set' method='POST'>"
                  "<input type='number' name='interval' min='1000' step='1000' value='" +
                  String(INTERVAL) + "'>"
                                     "<input type='submit' value='Set Interval'>"
                                     "</form></body></html>";
    server.send(200, "text/html", html);
}

void handleSet()
{
    if (server.hasArg("interval"))
    {
        INTERVAL = server.arg("interval").toInt();
        saveIntervalToEEPROM(INTERVAL);
    }
    server.send(200, "text/html", "<html><body><h2>Interval updated!</h2><a href='/'>Go Back</a></body></html>");
}

void setup()
{
    Serial.begin(9600);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(RELAY_PIN, LOW); // Ensure relay is off at start

    loadIntervalFromEEPROM();

    WiFi.softAP(apSSID, apPassword);
    Serial.println("AP Mode Enabled");

    // OTA Setup
    ArduinoOTA.setHostname("D1Mini-Relay");
    ArduinoOTA.begin();
    // Start web server
    server.on("/", handleRoot);
    server.on("/set", HTTP_POST, handleSet);
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
