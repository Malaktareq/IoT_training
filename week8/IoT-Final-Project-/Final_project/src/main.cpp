#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <U8g2lib.h>

/* ======================================================
   WIFI
   ====================================================== */
#define WIFI_SSID "CYBER_EXT"
#define WIFI_PASSWORD "cyberap2025"

/* ======================================================
   MQTT CONFIGURATION
   Mosquitto Broker = Windows Laptop
   ====================================================== */

#define MQTT_BROKER_HOST "192.168.1.18"
#define MQTT_BROKER_PORT 1883

#define MQTT_CLIENT_ID "smart-cnc-esp32"

#define MQTT_USERNAME "smartcnc"
#define MQTT_PASSWORD "awesomproj"

/* ======================================================
   MQTT TOPICS
   ====================================================== */

// ESP32 -> MQTT
#define TOPIC_STATE         "job/status/state"
#define TOPIC_JOB_START     "job/status/start"
#define TOPIC_JOB_END       "job/status/end"

#define TOPIC_DEVICE_STATUS "device/esp32/status"

#define TOPIC_TEMP_X        "sensor/bme280_x/temperature"
#define TOPIC_TEMP_Y        "sensor/bme280_y/temperature"

// MQTT -> ESP32
#define TOPIC_VIBRATION     "sensor/mpu6050/vibration"
#define TOPIC_ESTOP_COMMAND "estop/command"

/* ======================================================
   BME280 PINS
   ====================================================== */

// BME X
#define SDA_BUS1 21
#define SCL_BUS1 23

// BME Y
#define SDA_BUS2 32
#define SCL_BUS2 33

/* ======================================================
   OLED
   ====================================================== */

#define OLED_SDA 4
#define OLED_SCL 16

/* ======================================================
   RDC6445S INPUTS
   ====================================================== */

#define RDC_OUT1_PIN 19
#define RDC_OUT2_PIN 18

/* ======================================================
   RELAY
   ES32C14 Relay 1
   ====================================================== */

#define RELAY_PIN 12

#define RELAY_ACTIVE_LEVEL HIGH
#define RELAY_INACTIVE_LEVEL LOW

/* ======================================================
   I2C
   ====================================================== */

#define BME_X_ADDRESS 0x76
#define BME_Y_ADDRESS 0x76

#define OLED_ADDRESS 0x3C

#define I2C_CLOCK_HZ 100000

/* ======================================================
   TEMPERATURE SAFETY
   ====================================================== */

#define TEMP_HIGH_THRESHOLD_C 50.0f
#define TEMP_RESET_THRESHOLD_C 48.0f

/* ======================================================
   VIBRATION SAFETY
   ====================================================== */

#define VIBRATION_THRESHOLD_G 1.0f

/* ======================================================
   TIMING
   ====================================================== */

#define SENSOR_INTERVAL_MS 1000
#define MQTT_PUBLISH_INTERVAL_MS 1000
#define OLED_INTERVAL_MS 250
#define SERIAL_INTERVAL_MS 1000

#define WIFI_RECONNECT_INTERVAL_MS 10000
#define MQTT_RECONNECT_INTERVAL_MS 3000

#define DONE_DISPLAY_TIME_MS 2000

/* ======================================================
   I2C BUSES
   ====================================================== */

TwoWire BME_X_BUS = TwoWire(0);
TwoWire BME_Y_BUS = TwoWire(1);

/* ======================================================
   BME280
   ====================================================== */

Adafruit_BME280 bmeX;
Adafruit_BME280 bmeY;

bool bmeXOK = false;
bool bmeYOK = false;

float tempX = NAN;
float tempY = NAN;

/* ======================================================
   OLED
   ====================================================== */

U8G2_SSD1306_128X64_NONAME_F_SW_I2C oled(
    U8G2_R0,
    OLED_SCL,
    OLED_SDA,
    U8X8_PIN_NONE
);

bool oledOK = false;

/* ======================================================
   SAFETY STATES
   ====================================================== */

bool relayActive = false;

bool thermalTripActive = false;

/*
   Vibration fault is LATCHED.

   Once vibration > 1.0 g,
   restart/reset ESP32 to clear it.
*/
bool vibrationTripActive = false;

/*
   Dashboard MQTT emergency stop.

   Can be cleared with RESET/OFF/FALSE/0.
*/
bool mqttEstopActive = false;

float vibrationRMS = 0.0f;

/* ======================================================
   CNC STATE
   ====================================================== */

enum CNCState
{
    CNC_IDLE,
    CNC_RUNNING,
    CNC_DONE,
    CNC_FAULT
};

CNCState currentState = CNC_IDLE;
CNCState previousState = CNC_IDLE;

bool wasRunning = false;

unsigned long doneStartTime = 0;

/* ======================================================
   NETWORK
   ====================================================== */

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

/* ======================================================
   TIMERS
   ====================================================== */

unsigned long lastSensorRead = 0;
unsigned long lastMQTTPublish = 0;
unsigned long lastOLEDUpdate = 0;
unsigned long lastSerialPrint = 0;

unsigned long lastWiFiAttempt = 0;
unsigned long lastMQTTAttempt = 0;

/* ======================================================
   FUNCTION DECLARATIONS
   ====================================================== */

const char *stateToString(CNCState state);

void setRelay(bool active);
void applyRelayState();

void readTemperatures();
void checkTemperatureSafety();

void triggerVibrationTrip(float value);

void triggerMQTTEstop();
void resetMQTTEstop();

void updateCNCState();
void handleStateChange();

void connectWiFi();
void connectMQTT();

void mqttCallback(
    char *topic,
    byte *payload,
    unsigned int length
);

void publishTemperatures();
void publishState();

void publishJobEnd(
    const char *outcome,
    const char *reason
);

void updateOLED();
void printStatus();

/* ======================================================
   CNC STATE NAME
   ====================================================== */

const char *stateToString(CNCState state)
{
    switch (state)
    {
        case CNC_IDLE:
            return "IDLE";

        case CNC_RUNNING:
            return "RUNNING";

        case CNC_DONE:
            return "DONE";

        case CNC_FAULT:
            return "FAULTED";

        default:
            return "UNKNOWN";
    }
}

/* ======================================================
   RELAY CONTROL
   ====================================================== */

void setRelay(bool active)
{
    relayActive = active;

    digitalWrite(
        RELAY_PIN,
        active
            ? RELAY_ACTIVE_LEVEL
            : RELAY_INACTIVE_LEVEL
    );
}

/* ======================================================
   FINAL RELAY LOGIC
   ====================================================== */

void applyRelayState()
{
    /*
       Relay energizes if ANY condition is true:

       1. Temperature >= 50 C
       2. Vibration > 1.0 g
       3. Dashboard MQTT E-stop
    */

    bool shouldEnergize =
        thermalTripActive ||
        vibrationTripActive ||
        mqttEstopActive;

    setRelay(
        shouldEnergize
    );
}

/* ======================================================
   BME280 READ
   ====================================================== */

void readTemperatures()
{
    if (bmeXOK)
    {
        tempX =
            bmeX.readTemperature();

        if (!isfinite(tempX))
        {
            tempX = NAN;
        }
    }

    if (bmeYOK)
    {
        tempY =
            bmeY.readTemperature();

        if (!isfinite(tempY))
        {
            tempY = NAN;
        }
    }
}

/* ======================================================
   TEMPERATURE PROTECTION
   ====================================================== */

void checkTemperatureSafety()
{
    bool xHigh =
        isfinite(tempX) &&
        tempX >= TEMP_HIGH_THRESHOLD_C;

    bool yHigh =
        isfinite(tempY) &&
        tempY >= TEMP_HIGH_THRESHOLD_C;

    /* ==================================================
       TRIGGER TEMPERATURE STOP
       ================================================== */

    if (
        !thermalTripActive &&
        (xHigh || yHigh)
    )
    {
        bool jobWasRunning =
            currentState == CNC_RUNNING;

        thermalTripActive = true;

        wasRunning = false;

        applyRelayState();

        Serial.println();
        Serial.println(
            "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
        );

        Serial.println(
            "HIGH TEMPERATURE"
        );

        if (xHigh)
        {
            Serial.print(
                "Temperature X = "
            );

            Serial.print(
                tempX,
                2
            );

            Serial.println(
                " C"
            );
        }

        if (yHigh)
        {
            Serial.print(
                "Temperature Y = "
            );

            Serial.print(
                tempY,
                2
            );

            Serial.println(
                " C"
            );
        }

        Serial.println(
            "RELAY ENERGIZED"
        );

        Serial.println(
            "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
        );

        if (
            mqttClient.connected() &&
            jobWasRunning
        )
        {
            if (xHigh && yHigh)
            {
                publishJobEnd(
                    "stopped",
                    "high_temperature_x_y"
                );
            }

            else if (xHigh)
            {
                publishJobEnd(
                    "stopped",
                    "high_temperature_x"
                );
            }

            else
            {
                publishJobEnd(
                    "stopped",
                    "high_temperature_y"
                );
            }
        }
    }

    /* ==================================================
       TEMPERATURE RESET
       ================================================== */

    if (thermalTripActive)
    {
        bool xSafe =
            !isfinite(tempX) ||
            tempX <= TEMP_RESET_THRESHOLD_C;

        bool ySafe =
            !isfinite(tempY) ||
            tempY <= TEMP_RESET_THRESHOLD_C;

        if (
            xSafe &&
            ySafe
        )
        {
            thermalTripActive = false;

            applyRelayState();

            Serial.println();
            Serial.println(
                "TEMPERATURE TRIP CLEARED"
            );

            if (relayActive)
            {
                Serial.println(
                    "Relay remains energized because another fault is active."
                );
            }
        }
    }
}

/* ======================================================
   VIBRATION TRIP
   ====================================================== */

void triggerVibrationTrip(float value)
{
    if (vibrationTripActive)
    {
        return;
    }

    bool jobWasRunning =
        currentState == CNC_RUNNING;

    vibrationTripActive = true;

    wasRunning = false;

    applyRelayState();

    Serial.println();
    Serial.println(
        "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    );

    Serial.println(
        "ABNORMAL VIBRATION"
    );

    Serial.print(
        "Received vibration = "
    );

    Serial.print(
        value,
        3
    );

    Serial.println(
        " g"
    );

    Serial.print(
        "Threshold = "
    );

    Serial.print(
        VIBRATION_THRESHOLD_G,
        2
    );

    Serial.println(
        " g"
    );

    Serial.println(
        "RELAY ENERGIZED"
    );

    Serial.println(
        "VIBRATION TRIP LATCHED"
    );

    Serial.println(
        "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    );

    if (
        mqttClient.connected() &&
        jobWasRunning
    )
    {
        publishJobEnd(
            "stopped",
            "high_vibration"
        );
    }
}

/* ======================================================
   MQTT DASHBOARD EMERGENCY STOP
   ====================================================== */

void triggerMQTTEstop()
{
    if (mqttEstopActive)
    {
        return;
    }

    bool jobWasRunning =
        currentState == CNC_RUNNING;

    mqttEstopActive = true;

    wasRunning = false;

    applyRelayState();

    Serial.println();
    Serial.println(
        "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    );

    Serial.println(
        "DASHBOARD EMERGENCY STOP"
    );

    Serial.println(
        "GPIO27 = HIGH"
    );

    Serial.println(
        "RELAY ENERGIZED"
    );

    Serial.println(
        "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    );

    if (
        mqttClient.connected() &&
        jobWasRunning
    )
    {
        publishJobEnd(
            "stopped",
            "mqtt_estop"
        );
    }
}

/* ======================================================
   MQTT DASHBOARD RESET
   ====================================================== */

void resetMQTTEstop()
{
    mqttEstopActive = false;

    applyRelayState();

    Serial.println();
    Serial.println(
        "DASHBOARD E-STOP RESET"
    );

    /*
       Only release the relay if there are
       no other safety faults.
    */

    if (vibrationTripActive)
    {
        Serial.println(
            "RELAY STAYS ON: VIBRATION TRIP ACTIVE"
        );
    }

    else if (thermalTripActive)
    {
        Serial.println(
            "RELAY STAYS ON: TEMPERATURE TRIP ACTIVE"
        );
    }

    else
    {
        Serial.println(
            "GPIO27 = LOW"
        );

        Serial.println(
            "RELAY DE-ENERGIZED"
        );
    }
}

/* ======================================================
   RDC6445S STATE
   ====================================================== */

void updateCNCState()
{
    int out1 =
        digitalRead(
            RDC_OUT1_PIN
        );

    int out2 =
        digitalRead(
            RDC_OUT2_PIN
        );

    /*
       OUT1:
       LOW  = IDLE
       HIGH = RUNNING

       OUT2:
       LOW  = RDC FAULT
       HIGH = NO FAULT
    */

    bool idleSignal =
        (out1 == LOW);

    bool rdcFaultSignal =
        (out2 == LOW);

    /* ==================================================
       SAFETY FAULT
       ================================================== */

    if (
        thermalTripActive ||
        vibrationTripActive ||
        mqttEstopActive
    )
    {
        currentState =
            CNC_FAULT;

        wasRunning =
            false;

        return;
    }

    /* ==================================================
       RDC FAULT
       ================================================== */

    if (rdcFaultSignal)
    {
        currentState =
            CNC_FAULT;

        wasRunning =
            false;

        return;
    }

    /* ==================================================
       DONE TIMER
       ================================================== */

    if (
        currentState ==
        CNC_DONE
    )
    {
        if (
            millis() -
                doneStartTime <
            DONE_DISPLAY_TIME_MS
        )
        {
            return;
        }

        currentState =
            CNC_IDLE;
    }

    /* ==================================================
       IDLE / DONE
       ================================================== */

    if (idleSignal)
    {
        if (wasRunning)
        {
            currentState =
                CNC_DONE;

            doneStartTime =
                millis();

            wasRunning =
                false;
        }

        else
        {
            currentState =
                CNC_IDLE;
        }
    }

    /* ==================================================
       RUNNING
       ================================================== */

    else
    {
        currentState =
            CNC_RUNNING;

        wasRunning =
            true;
    }
}

/* ======================================================
   CNC STATE CHANGE
   ====================================================== */

void handleStateChange()
{
    if (
        currentState ==
        previousState
    )
    {
        return;
    }

    Serial.println();

    Serial.println(
        "=============================="
    );

    Serial.print(
        "CNC STATE: "
    );

    Serial.println(
        stateToString(
            currentState
        )
    );

    Serial.println(
        "=============================="
    );

    if (mqttClient.connected())
    {
        mqttClient.publish(
            TOPIC_STATE,
            stateToString(
                currentState
            ),
            true
        );

        /* ==================================================
           JOB START
           ================================================== */

        if (
            currentState ==
            CNC_RUNNING
        )
        {
            mqttClient.publish(
                TOPIC_JOB_START,
                "started"
            );
        }

        /* ==================================================
           NORMAL JOB END
           ================================================== */

        else if (
            currentState ==
            CNC_DONE
        )
        {
            publishJobEnd(
                "completed",
                "normal_completion"
            );
        }

        /* ==================================================
           RDC FAULT
           ================================================== */

        else if (
            currentState ==
                CNC_FAULT &&
            !thermalTripActive &&
            !vibrationTripActive &&
            !mqttEstopActive
        )
        {
            publishJobEnd(
                "faulted",
                "rdc_fault"
            );
        }
    }

    previousState =
        currentState;
}

/* ======================================================
   JOB END JSON
   ====================================================== */

void publishJobEnd(
    const char *outcome,
    const char *reason
)
{
    if (!mqttClient.connected())
    {
        return;
    }

    char payload[160];

    snprintf(
        payload,
        sizeof(payload),
        "{\"outcome\":\"%s\",\"reason\":\"%s\"}",
        outcome,
        reason
    );

    mqttClient.publish(
        TOPIC_JOB_END,
        payload
    );
}

/* ======================================================
   MQTT CALLBACK
   ====================================================== */

void mqttCallback(
    char *topic,
    byte *payload,
    unsigned int  length
)
{
    char message[128];

    unsigned int copyLength =
        length;

    if (
        copyLength >=
        sizeof(message)
    )
    {
        copyLength =
            sizeof(message) - 1;
    }

    memcpy(
        message,
        payload,
        copyLength
    );

    message[copyLength] =
        '\0';

    String command =
        String(message);

    command.trim();

    Serial.println();

    Serial.print(
        "MQTT RX ["
    );

    Serial.print(
        topic
    );

    Serial.print(
        "] = "
    );

    Serial.println(
        command
    );

    /* ==================================================
       VIBRATION MQTT
       ================================================== */

    if (
        strcmp(
            topic,
            TOPIC_VIBRATION
        ) == 0
    )
    {
        char *endPointer;

        float receivedValue =
            strtof(
                message,
                &endPointer
            );

        if (
            endPointer ==
            message
        )
        {
            Serial.println(
                "INVALID VIBRATION PAYLOAD"
            );

            return;
        }

        vibrationRMS =
            receivedValue;

        Serial.print(
            "Vibration = "
        );

        Serial.print(
            vibrationRMS,
            3
        );

        Serial.print(
            " g | Threshold = "
        );

        Serial.print(
            VIBRATION_THRESHOLD_G,
            2
        );

        Serial.println(
            " g"
        );

        /*
           Trip ONLY if greater than 1.0 g.
        */

        if (
            vibrationRMS >
            VIBRATION_THRESHOLD_G
        )
        {
            triggerVibrationTrip(
                vibrationRMS
            );
        }

        return;
    }

    /* ==================================================
       DASHBOARD E-STOP COMMAND
       ================================================== */

    if (
        strcmp(
            topic,
            TOPIC_ESTOP_COMMAND
        ) == 0
    )
    {
        /*
           Supports common Node-RED button payloads.

           STOP:
           STOP
           ON
           TRUE
           TRIGGER
           ESTOP
           EMERGENCY
           1

           RESET:
           RESET
           OFF
           FALSE
           RELEASE
           0
        */

        if (
            command.equalsIgnoreCase("STOP") ||
            command.equalsIgnoreCase("ON") ||
            command.equalsIgnoreCase("TRUE") ||
            command.equalsIgnoreCase("TRIGGER") ||
            command.equalsIgnoreCase("ESTOP") ||
            command.equalsIgnoreCase("EMERGENCY") ||
            command == "1"
        )
        {
            Serial.println(
                "VALID DASHBOARD STOP COMMAND"
            );

            triggerMQTTEstop();

            return;
        }

        if (
            command.equalsIgnoreCase("RESET") ||
            command.equalsIgnoreCase("OFF") ||
            command.equalsIgnoreCase("FALSE") ||
            command.equalsIgnoreCase("RELEASE") ||
            command == "0"
        )
        {
            Serial.println(
                "VALID DASHBOARD RESET COMMAND"
            );

            resetMQTTEstop();

            return;
        }

        Serial.print(
            "UNKNOWN E-STOP PAYLOAD: "
        );

        Serial.println(
            command
        );
    }
}

/* ======================================================
   WIFI
   ====================================================== */

void connectWiFi()
{
    if (
        WiFi.status() ==
        WL_CONNECTED
    )
    {
        return;
    }

    Serial.println();
    Serial.println(
        "Connecting to WiFi..."
    );

    WiFi.mode(
        WIFI_STA
    );

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );

    unsigned long start =
        millis();

    while (
        WiFi.status() !=
            WL_CONNECTED &&
        millis() - start <
            15000
    )
    {
        delay(500);

        Serial.print(".");
    }

    Serial.println();

    if (
        WiFi.status() ==
        WL_CONNECTED
    )
    {
        Serial.println(
            "WiFi CONNECTED"
        );

        Serial.print(
            "ESP32 IP: "
        );

        Serial.println(
            WiFi.localIP()
        );

        Serial.print(
            "Broker: "
        );

        Serial.print(
            MQTT_BROKER_HOST
        );

        Serial.print(":");

        Serial.println(
            MQTT_BROKER_PORT
        );
    }

    else
    {
        Serial.println(
            "WiFi CONNECTION FAILED"
        );
    }
}

/* ======================================================
   MQTT CONNECTION
   ====================================================== */

void connectMQTT()
{
    if (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        return;
    }

    if (
        mqttClient.connected()
    )
    {
        return;
    }

    Serial.println();

    Serial.print(
        "Connecting MQTT -> "
    );

    Serial.print(
        MQTT_BROKER_HOST
    );

    Serial.print(":");

    Serial.println(
        MQTT_BROKER_PORT
    );

    bool connected =
        mqttClient.connect(
            MQTT_CLIENT_ID,
            MQTT_USERNAME,
            MQTT_PASSWORD,

            /*
               Last Will
            */
            TOPIC_DEVICE_STATUS,
            0,
            true,
            "offline"
        );

    if (connected)
    {
        Serial.println();
        Serial.println(
            "MQTT CONNECTED"
        );

        /* ==================================================
           DEVICE ONLINE
           ================================================== */

        mqttClient.publish(
            TOPIC_DEVICE_STATUS,
            "online",
            true
        );

        /* ==================================================
           VIBRATION SUBSCRIPTION
           ================================================== */

        bool vibrationSubscription =
            mqttClient.subscribe(
                TOPIC_VIBRATION
            );

        Serial.print(
            "SUB "
        );

        Serial.print(
            TOPIC_VIBRATION
        );

        Serial.print(
            " -> "
        );

        Serial.println(
            vibrationSubscription
                ? "OK"
                : "FAILED"
        );

        /* ==================================================
           DASHBOARD E-STOP SUBSCRIPTION
           ================================================== */

        bool estopSubscription =
            mqttClient.subscribe(
                TOPIC_ESTOP_COMMAND
            );

        Serial.print(
            "SUB "
        );

        Serial.print(
            TOPIC_ESTOP_COMMAND
        );

        Serial.print(
            " -> "
        );

        Serial.println(
            estopSubscription
                ? "OK"
                : "FAILED"
        );

        /* ==================================================
           INITIAL DATA
           ================================================== */

        publishState();

        publishTemperatures();
    }

    else
    {
        Serial.print(
            "MQTT CONNECTION FAILED | State = "
        );

        Serial.println(
            mqttClient.state()
        );
    }
}

/* ======================================================
   PUBLISH TEMPERATURES
   ====================================================== */

void publishTemperatures()
{
    if (!mqttClient.connected())
    {
        return;
    }

    char value[16];

    /* ==================================================
       X
       ================================================== */

    if (isfinite(tempX))
    {
        snprintf(
            value,
            sizeof(value),
            "%.2f",
            tempX
        );

        mqttClient.publish(
            TOPIC_TEMP_X,
            value
        );
    }

    /* ==================================================
       Y
       ================================================== */

    if (isfinite(tempY))
    {
        snprintf(
            value,
            sizeof(value),
            "%.2f",
            tempY
        );

        mqttClient.publish(
            TOPIC_TEMP_Y,
            value
        );
    }
}

/* ======================================================
   PUBLISH STATE
   ====================================================== */

void publishState()
{
    if (!mqttClient.connected())
    {
        return;
    }

    mqttClient.publish(
        TOPIC_STATE,
        stateToString(
            currentState
        ),
        true
    );
}

/* ======================================================
   OLED
   ====================================================== */

void updateOLED()
{
    if (!oledOK)
    {
        return;
    }

    oled.clearBuffer();

    oled.setFont(
        u8g2_font_6x10_tf
    );

    /* ==================================================
       TITLE
       ================================================== */

    oled.setCursor(
        0,
        10
    );

    oled.print(
        "SMART CNC STATION"
    );

    /* ==================================================
       TEMPERATURE X
       ================================================== */

    oled.setCursor(
        0,
        25
    );

    oled.print(
        "X:"
    );

    if (isfinite(tempX))
    {
        oled.print(
            tempX,
            1
        );

        oled.print(
            "C"
        );
    }

    else
    {
        oled.print(
            "ERR"
        );
    }

    /* ==================================================
       TEMPERATURE Y
       ================================================== */

    oled.setCursor(
        64,
        25
    );

    oled.print(
        "Y:"
    );

    if (isfinite(tempY))
    {
        oled.print(
            tempY,
            1
        );

        oled.print(
            "C"
        );
    }

    else
    {
        oled.print(
            "ERR"
        );
    }

    /* ==================================================
       CNC STATE
       ================================================== */

    oled.setCursor(
        0,
        40
    );

    oled.print(
        "CNC:"
    );

    oled.print(
        stateToString(
            currentState
        )
    );

    /* ==================================================
       SAFETY STATUS
       ================================================== */

    oled.setCursor(
        0,
        57
    );

    if (mqttEstopActive)
    {
        oled.print(
            "DASHBOARD E-STOP"
        );
    }

    else if (vibrationTripActive)
    {
        oled.print(
            "VIB >1G - STOP"
        );
    }

    else if (thermalTripActive)
    {
        oled.print(
            "TEMP HIGH - STOP"
        );
    }

    else
    {
        oled.print(
            "V:"
        );

        oled.print(
            vibrationRMS,
            2
        );

        oled.print(
            "g "
        );

        oled.print(
            mqttClient.connected()
                ? "MQTT"
                : "NO MQTT"
        );
    }

    oled.sendBuffer();
}

/* ======================================================
   SERIAL STATUS
   ====================================================== */

void printStatus()
{
    Serial.println();

    Serial.println(
        "--------------------------------"
    );

    /* ==================================================
       NETWORK
       ================================================== */

    Serial.print(
        "MQTT: "
    );

    Serial.println(
        mqttClient.connected()
            ? "CONNECTED"
            : "DISCONNECTED"
    );

    /* ==================================================
       TEMP X
       ================================================== */

    Serial.print(
        "Temp X: "
    );

    if (isfinite(tempX))
    {
        Serial.print(
            tempX,
            2
        );

        Serial.println(
            " C"
        );
    }

    else
    {
        Serial.println(
            "ERROR"
        );
    }

    /* ==================================================
       TEMP Y
       ================================================== */

    Serial.print(
        "Temp Y: "
    );

    if (isfinite(tempY))
    {
        Serial.print(
            tempY,
            2
        );

        Serial.println(
            " C"
        );
    }

    else
    {
        Serial.println(
            "ERROR"
        );
    }

    /* ==================================================
       VIBRATION
       ================================================== */

    Serial.print(
        "Vibration: "
    );

    Serial.print(
        vibrationRMS,
        3
    );

    Serial.println(
        " g"
    );

    Serial.print(
        "Vibration Trip: "
    );

    Serial.println(
        vibrationTripActive
            ? "ACTIVE"
            : "NORMAL"
    );

    /* ==================================================
       MQTT E-STOP
       ================================================== */

    Serial.print(
        "Dashboard E-Stop: "
    );

    Serial.println(
        mqttEstopActive
            ? "ACTIVE"
            : "OFF"
    );

    /* ==================================================
       TEMPERATURE TRIP
       ================================================== */

    Serial.print(
        "Temperature Trip: "
    );

    Serial.println(
        thermalTripActive
            ? "ACTIVE"
            : "NORMAL"
    );

    /* ==================================================
       RDC
       ================================================== */

    Serial.print(
        "RDC OUT1: "
    );

    Serial.println(
        digitalRead(
            RDC_OUT1_PIN
        )
    );

    Serial.print(
        "RDC OUT2: "
    );

    Serial.println(
        digitalRead(
            RDC_OUT2_PIN
        )
    );

    /* ==================================================
       CNC
       ================================================== */

    Serial.print(
        "CNC State: "
    );

    Serial.println(
        stateToString(
            currentState
        )
    );

    /* ==================================================
       RELAY
       ================================================== */

    Serial.print(
        "Relay GPIO27: "
    );

    Serial.println(
        relayActive
            ? "ENERGIZED / HIGH"
            : "OFF / LOW"
    );

    Serial.println(
        "--------------------------------"
    );
}

/* ======================================================
   SETUP
   ====================================================== */

void setup()
{
    Serial.begin(
        115200
    );

    delay(
        1000
    );

    Serial.println();

    Serial.println(
        "================================"
    );

    Serial.println(
        "SMART CNC STATION"
    );

    Serial.println(
        "================================"
    );

    /* ==================================================
       RDC INPUTS
       ================================================== */

    pinMode(
        RDC_OUT1_PIN,
        INPUT
    );

    pinMode(
        RDC_OUT2_PIN,
        INPUT
    );

    /* ==================================================
       RELAY
       ================================================== */

    pinMode(
        RELAY_PIN,
        OUTPUT
    );

    setRelay(
        false
    );

    Serial.println(
        "Relay GPIO27 initialized OFF"
    );

    /* ==================================================
       BME X
       ================================================== */

    BME_X_BUS.begin(
        SDA_BUS1,
        SCL_BUS1,
        I2C_CLOCK_HZ
    );

    bmeXOK =
        bmeX.begin(
            BME_X_ADDRESS,
            &BME_X_BUS
        );

    Serial.print(
        "BME X [21/23]: "
    );

    Serial.println(
        bmeXOK
            ? "FOUND"
            : "NOT FOUND"
    );

    /* ==================================================
       BME Y
       ================================================== */

    BME_Y_BUS.begin(
        SDA_BUS2,
        SCL_BUS2,
        I2C_CLOCK_HZ
    );

    bmeYOK =
        bmeY.begin(
            BME_Y_ADDRESS,
            &BME_Y_BUS
        );

    Serial.print(
        "BME Y [32/33]: "
    );

    Serial.println(
        bmeYOK
            ? "FOUND"
            : "NOT FOUND"
    );

    /* ==================================================
       OLED
       ================================================== */

    oled.setI2CAddress(
        OLED_ADDRESS << 1
    );

    oled.begin();

    oledOK =
        true;

    oled.clearBuffer();

    oled.setFont(
        u8g2_font_6x10_tf
    );

    oled.setCursor(
        0,
        15
    );

    oled.print(
        "SMART CNC STATION"
    );

    oled.setCursor(
        0,
        35
    );

    oled.print(
        "Starting..."
    );

    oled.sendBuffer();

    /* ==================================================
       INITIAL VALUES
       ================================================== */

    readTemperatures();

    checkTemperatureSafety();

    updateCNCState();

    previousState =
        currentState;

    /* ==================================================
       WIFI
       ================================================== */

    connectWiFi();

    /* ==================================================
       MQTT
       ================================================== */

    mqttClient.setServer(
        MQTT_BROKER_HOST,
        MQTT_BROKER_PORT
    );

    mqttClient.setCallback(
        mqttCallback
    );

    mqttClient.setBufferSize(
        512
    );

    connectMQTT();

    updateOLED();

    /* ==================================================
       READY
       ================================================== */

    Serial.println();

    Serial.println(
        "================================"
    );

    Serial.println(
        "SYSTEM READY"
    );

    Serial.println(
        "================================"
    );

    Serial.println();

    Serial.print(
        "Broker: "
    );

    Serial.print(
        MQTT_BROKER_HOST
    );

    Serial.print(":");

    Serial.println(
        MQTT_BROKER_PORT
    );

    Serial.println();

    Serial.print(
        "SUB vibration: "
    );

    Serial.println(
        TOPIC_VIBRATION
    );

    Serial.print(
        "SUB dashboard E-stop: "
    );

    Serial.println(
        TOPIC_ESTOP_COMMAND
    );

    Serial.println();

    Serial.println(
        "RELAY ENERGIZES WHEN:"
    );

    Serial.println(
        "- Temp >= 50 C"
    );

    Serial.println(
        "- Vibration > 1.00 g"
    );

    Serial.println(
        "- Dashboard E-stop command"
    );
}

/* ======================================================
   LOOP
   ====================================================== */

void loop()
{
    /* ==================================================
       WIFI RECONNECT
       ================================================== */

    if (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        if (
            millis() -
                lastWiFiAttempt >=
            WIFI_RECONNECT_INTERVAL_MS
        )
        {
            lastWiFiAttempt =
                millis();

            connectWiFi();
        }
    }

    /* ==================================================
       MQTT RECONNECT
       ================================================== */

    if (
        WiFi.status() ==
            WL_CONNECTED &&
        !mqttClient.connected()
    )
    {
        if (
            millis() -
                lastMQTTAttempt >=
            MQTT_RECONNECT_INTERVAL_MS
        )
        {
            lastMQTTAttempt =
                millis();

            connectMQTT();
        }
    }

    /* ==================================================
       MQTT
       ================================================== */

    if (mqttClient.connected())
    {
        mqttClient.loop();
    }

    /* ==================================================
       CNC STATE
       ================================================== */

    updateCNCState();

    handleStateChange();

    /* ==================================================
       TEMPERATURE
       ================================================== */

    if (
        millis() -
            lastSensorRead >=
        SENSOR_INTERVAL_MS
    )
    {
        lastSensorRead =
            millis();

        readTemperatures();

        checkTemperatureSafety();
    }

    /* ==================================================
       MQTT TEMPERATURE PUBLISH
       ================================================== */

    if (
        millis() -
            lastMQTTPublish >=
        MQTT_PUBLISH_INTERVAL_MS
    )
    {
        lastMQTTPublish =
            millis();

        publishTemperatures();
    }

    /* ==================================================
       OLED
       ================================================== */

    if (
        millis() -
            lastOLEDUpdate >=
        OLED_INTERVAL_MS
    )
    {
        lastOLEDUpdate =
            millis();

        updateOLED();
    }

    /* ==================================================
       SERIAL
       ================================================== */

    if (
        millis() -
            lastSerialPrint >=
        SERIAL_INTERVAL_MS
    )
    {
        lastSerialPrint =
            millis();

        printStatus();
    }

    delay(
        20
    );
}