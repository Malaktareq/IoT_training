#include <SPIFFS.h>
#include <FS.h>
#include <Adafruit_BME280.h>
#include "config.h"

void createCSV();
void saveData();

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

  Serial.println("BME280 sensor initialized successfully!");
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
  createCSV();
}

void loop() 
{
  unsigned long currentTime;
  currentTime = millis();

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
    pressure = bme.readPressure() / 100.0F; // Convert Pa to
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
    Serial.println("Data saved to CSV file");
}
