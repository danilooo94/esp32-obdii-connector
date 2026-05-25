#include <Arduino.h>
#include <ArduinoJson.h>

// Set to 1 to output fake data without connecting to OBD (for dashboard testing)
#define SIMULATE_OBD 0

#if !SIMULATE_OBD
#include <BLEClientSerial.h>
#include <ELMduino.h>

// Change to match your BLE OBD adapter name (check with nRF Connect app)
// Common names: "OBDII", "OBD2", "V-LINK", "OBDLink CX", "VEEPEAK"
#define OBD_DEVICE_NAME "OBDII"

BLEClientSerial BLESerial;
ELM327          myELM327;

enum OBDState { READ_COOLANT, READ_OIL, READ_INTAKE, READ_FUEL, READ_BATTERY };
OBDState obdState = READ_COOLANT;
#endif

float coolantTemp    = 0;
float oilTemp        = 0;
float intakeTemp     = 0;
float fuelLevel      = 0;
float batteryVoltage = 0;

void sendJson() {
    StaticJsonDocument<160> doc;
    doc["coolant_temp"] = coolantTemp;
    doc["oil_temp"]     = oilTemp;
    doc["intake_temp"]  = intakeTemp;
    doc["fuel_pct"]     = fuelLevel;
    doc["battery_v"]    = batteryVoltage;
    serializeJson(doc, Serial);
    Serial.println();
}

// --- Simulation mode ---
#if SIMULATE_OBD

static float simCoolant  = 20.0;
static float simOil      = 18.0;
static float simFuel     = 72.0;
static float simBattery  = 12.6;

void simulateTick() {
    // Gradually warm up engine, simulate small random drift
    if (simCoolant < 90.0) simCoolant += 0.3;
    if (simOil     < 95.0) simOil     += 0.2;
    simBattery = 12.4 + (random(0, 40) / 100.0);

    coolantTemp    = simCoolant;
    oilTemp        = simOil;
    intakeTemp     = 28.0 + (random(-20, 20) / 10.0);
    fuelLevel      = simFuel;
    batteryVoltage = simBattery;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("{\"status\":\"simulating\"}");
}

void loop() {
    simulateTick();
    sendJson();
    delay(500);
}

// --- Real OBD mode ---
#else

void connectToOBD() {
    Serial.println("{\"status\":\"connecting\"}");
    BLESerial.begin(OBD_DEVICE_NAME);
    while (!BLESerial.connected()) {
        delay(500);
    }
    Serial.println("{\"status\":\"connected\"}");
    myELM327.begin(BLESerial, false, 2000);
    obdState = READ_COOLANT;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    connectToOBD();
}

void loop() {
    if (!BLESerial.connected()) {
        Serial.println("{\"status\":\"disconnected\"}");
        delay(2000);
        connectToOBD();
        return;
    }

    switch (obdState) {
        case READ_COOLANT: {
            float val = myELM327.engineCoolantTemp();
            if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
                if (myELM327.nb_rx_state == ELM_SUCCESS)
                    coolantTemp = val;
                obdState = READ_OIL;
            }
            break;
        }
        case READ_OIL: {
            float val = myELM327.oilTemp();
            if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
                if (myELM327.nb_rx_state == ELM_SUCCESS)
                    oilTemp = val;
                obdState = READ_INTAKE;
            }
            break;
        }
        case READ_INTAKE: {
            float val = myELM327.intakeAirTemp();
            if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
                if (myELM327.nb_rx_state == ELM_SUCCESS)
                    intakeTemp = val;
                obdState = READ_FUEL;
            }
            break;
        }
        case READ_FUEL: {
            float val = myELM327.fuelLevel();
            if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
                if (myELM327.nb_rx_state == ELM_SUCCESS)
                    fuelLevel = val;
                obdState = READ_BATTERY;
            }
            break;
        }
        case READ_BATTERY: {
            float val = myELM327.batteryVoltage();
            if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
                if (myELM327.nb_rx_state == ELM_SUCCESS)
                    batteryVoltage = val;
                sendJson();
                obdState = READ_COOLANT;
            }
            break;
        }
    }
}

#endif
