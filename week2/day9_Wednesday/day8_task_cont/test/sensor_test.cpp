
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
// put function declarations here:

Adafruit_BME280 bme; // I2C

#define BME280_I2C_ADDRESS 0x76

unsigned long lastReadTime = 0;
const unsigned long interval = 10000; // 10 seconds
void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial monitor to open

  // Initialize the BME280 sensor
  if (!bme.begin(BME280_I2C_ADDRESS)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1); // Halt execution if sensor is not found
  }

  Serial.println("BME280 sensor initialized successfully!");
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastReadTime >= 10000) { // Read every
  // Read and print temperature, humidity, and pressure
  Serial.print("Temperature: ");
  Serial.print(bme.readTemperature());
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  Serial.print("Pressure: ");
  Serial.print(bme.readPressure() / 100.0F); // Convert Pa to hPa
  Serial.println(" hPa");

  Serial.println();
  lastReadTime = currentTime; // Update the last read time
  } // Wait 10 seconds before the next reading
}