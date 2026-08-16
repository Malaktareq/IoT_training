# ESP32 Local Broker Motor Control Project

This project is a PlatformIO firmware project for an ESP32 development board that connects to Wi‑Fi and an MQTT broker to control a DC motor and read environmental sensor data.

It uses:
- ESP32 Dev Module
- L298N motor driver
- BME280 temperature, humidity, and pressure sensor
- MQTT messaging for command and telemetry topics

## Features

- Motor control via MQTT commands
- Speed and direction control using a PWM-enabled L298N driver
- Sensor publishing for temperature, humidity, and pressure
- Wi‑Fi reconnection and MQTT reconnection handling
- MQTT status reporting for the motor and device state

## Project structure

```text
Day18_Tuesday/
├── README.md
├── local_broker/
│   ├── platformio.ini
│   ├── include/
│   │   ├── config.h
│   │   ├── L298N.h
│   │   └── project.h
│   ├── src/
│   │   ├── L298N.cpp
│   │   ├── main.cpp
│   │   └── motor.cpp
│   └── test/
│       └── README
└── reports/
```

## Hardware connections

The project currently configures the following pins in `local_broker/include/config.h`:

- SCL: GPIO 22
- SDA: GPIO 21
- ENA: GPIO 18
- IN2: GPIO 3
- IN1: GPIO 19

The motor is connected through an L298N driver using the `IN1`, `IN2`, and `EN` pins.

## MQTT topics

### Commands sent to the ESP32

- `esp32/motor/cmd/power`  
  Values: `1`, `0`, `on`, `off`
- `esp32/motor/cmd/direction`  
  Values: `forward`, `backward`
- `esp32/motor/cmd/speed`  
  Values: integer from `0` to `100`

### Device and motor status published by the ESP32

- `esp32/status`  
  Online/offline status
- `esp32/motor/speed`  
  Current motor speed percentage
- `esp32/motor/direction`  
  `forward`, `backward`, or `stopped`
- `esp32/motor/power`  
  `running` or `stopped`

### Sensor readings published by the ESP32

- `esp32/sensor/temperature`
- `esp32/sensor/humidity`
- `esp32/sensor/pressure`

## Configuration

Before compiling and uploading, update the Wi‑Fi and MQTT settings in:

- `local_broker/include/config.h`

The current values are:

```cpp
#define WIFI_SSID "CYBER_EXT"
#define WIFI_PASSWORD "cyberap2025"
#define MQTT_BROKER "192.168.1.107"
const uint16_t MQTT_PORT = 1883;
```

Update these values to match your local network and broker.

## Building and uploading

Open the project in VS Code with the PlatformIO extension installed, then work from the `local_broker` folder.

Use the PlatformIO upload button in VS Code to compile and flash the firmware to the ESP32.

If the board is not detected, ensure that:
- the ESP32 is connected to the correct USB port,
- the correct board target is selected in PlatformIO,
- the serial port is available in the operating system,
- the project is opened from the `local_broker` folder rather than the workspace root.

The build and upload steps can be performed directly from the VS Code PlatformIO interface without using terminal commands.

## Notes

- The project expects a functioning MQTT broker on the configured IP address.
- The BME280 sensor is initialized over I2C.
- If the ESP32 loses Wi‑Fi or MQTT connectivity, the code attempts to reconnect automatically.
- Motor behavior is controlled by the MQTT command topics and applied in the main loop.

## Typical workflow

1. Configure your Wi‑Fi SSID and password.
2. Set the MQTT broker IP and port.
3. Flash the firmware to the ESP32.
4. Publish commands to the relevant MQTT topics.
5. Monitor the sensor and motor topics from any MQTT client.

## License

This project is intended for educational and experimental use.
