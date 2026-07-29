#include <SPIFFS.h>
#include <FS.h>
#include <Adafruit_BME280.h>
#include "config.h"
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "esp32_ap";
const char* password = "malak1234";

void createCSV();
void saveData();
void initWiFi();
void setupServerFiles();

WebServer server(80);

Adafruit_BME280 bme;

unsigned long lastReadTime = 0;
const unsigned long interval = 10000; // 10 seconds

void setup()
{
  Serial.begin(115200);
   if (!bme.begin(BME280_I2C_ADDRESS)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1); // Halt execution if sensor is not found
  }
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
  createCSV();
  initWiFi();
  setupServerFiles();
  server.begin();
}

void loop() 
{
  unsigned long currentTime;
  currentTime = millis();
      server.handleClient();

  if (currentTime - lastReadTime >= interval)
  {
    saveData();
    lastReadTime = currentTime;
  }
}

void createCSV()
{
    File file;
    if (SPIFFS.exists("/data.csv")) {
        Serial.println("CSV file already exists, skipping creation");
        return;
    }
    file = SPIFFS.open("/data.csv", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    
    file.println("timestamp,Temperature,Humidity,Pressure");
    file.close();
    Serial.println("CSV file created successfully");              
}

void saveData()
{
  float temperature;
  float humidity; 
  float pressure; 
  unsigned long timestamp;
  File file;

  timestamp = millis() / 1000; // Convert milliseconds to seconds
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F; // Convert Pa to hPa
  file = SPIFFS.open("/data.csv", FILE_APPEND);
  if(!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Humidity: ");
  Serial.println(humidity);
  Serial.print("Pressure: ");
  Serial.println(pressure);
  file.print(timestamp);
  file.print(",");
  file.print(temperature);
  file.print(",");
  file.print(humidity);
  file.print(",");
  file.println(pressure);
  file.close();
  Serial.print("SSID: ");
  Serial.println(ssid);

  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Data saved to CSV file");
}



void initWiFi()
{
    WiFi.mode(WIFI_AP);

    if (!WiFi.softAP(ssid, password))
    {
        Serial.println("Failed to start Access Point");
        return;
    }

    Serial.println("Access Point Started");
    Serial.print("SSID: ");
    Serial.println(ssid);

    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
}

void setupServerFiles()
{
  // 1. Serve the index.html page stored in SPIFFS
  server.on("/", []() {
    if (SPIFFS.exists("/index.html")) {
      File file = SPIFFS.open("/index.html", "r");
      server.streamFile(file, "text/html");
      file.close();
    } else {
      server.send(404, "text/plain", "index.html not found in SPIFFS. Please upload it.");
    }
  });

  // 2. Serve JSON formatted sensor data endpoint requested by your JS poll function
  server.on("/data", []() {
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F;

    String json = "{";
    json += "\"temperature\":" + String(temperature, 2) + ",";
    json += "\"humidity\":" + String(humidity, 2) + ",";
    json += "\"pressure\":" + String(pressure, 2);
    json += "}";

    server.send(200, "application/json", json);
  });

  // 3. Serve the CSV file download endpoint requested by the download button
  server.on("/download", []() {
    if (SPIFFS.exists("/data.csv")) {
      File file = SPIFFS.open("/data.csv", "r");
      server.sendHeader("Content-Type", "text/csv");
      server.sendHeader("Content-Disposition", "attachment; filename=sensor_data.csv");
      server.streamFile(file, "text/csv");
      file.close();
    } else {
      server.send(404, "text/plain", "File not found");
    }
  });
}