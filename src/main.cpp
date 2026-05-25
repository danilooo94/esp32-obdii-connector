#include <Arduino.h>
#include <ArduinoJson.h>

// Set to 1 to emit fake data without connecting to OBD (for dashboard testing)
#define SIMULATE_OBD 0

#if !SIMULATE_OBD
#include <BLEClientSerial.h>
#include <ELMduino.h>

// Change to match your BLE OBD adapter name (use nRF Connect app to check)
// Common names: "OBDII", "OBD2", "V-LINK", "OBDLink CX", "VEEPEAK"
#define OBD_DEVICE_NAME "OBDII"

BLEClientSerial BLESerial;
ELM327          myELM327;

enum OBDState { READ_COOLANT, READ_OIL, READ_INTAKE, READ_FUEL, READ_BATTERY };
OBDState obdState = READ_COOLANT;
#endif

// Sensor values — updated by the PID state machine, sent together as one JSON line
float coolantTemp    = 0;
float oilTemp        = 0;
float intakeTemp     = 0;
float fuelLevel      = 0;
float batteryVoltage = 0;

void sendJson()
{
    StaticJsonDocument<160> doc;
    doc["coolant_temp"] = coolantTemp;
    doc["oil_temp"]     = oilTemp;
    doc["intake_temp"]  = intakeTemp;
    doc["fuel_pct"]     = fuelLevel;
    doc["battery_v"]    = batteryVoltage;
    serializeJson(doc, Serial);
    Serial.println();
}

// =============================================================================
// SIMULATION MODE
// =============================================================================
#if SIMULATE_OBD

static float simCoolant = 20.0f;
static float simOil     = 18.0f;
static float simFuel    = 72.0f;

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("{\"status\":\"simulating\"}");
}

void loop()
{
    if (simCoolant < 90.0f) simCoolant += 0.3f;
    if (simOil     < 95.0f) simOil     += 0.2f;

    coolantTemp    = simCoolant;
    oilTemp        = simOil;
    intakeTemp     = 28.0f + (random(-20, 20) / 10.0f);
    fuelLevel      = simFuel;
    batteryVoltage = 12.4f + (random(0, 40) / 100.0f);

    sendJson();
    delay(500);
}

// =============================================================================
// REAL OBD MODE
// =============================================================================
#else

// Scans for the BLE adapter, connects, and initialises ELMduino.
// Returns true on success, false if the adapter was not found or refused to connect.
bool connectToOBD()
{
    Serial.println("{\"status\":\"scanning\"}");

    if (!BLESerial.begin(OBD_DEVICE_NAME))
    {
        Serial.println("{\"status\":\"not_found\"}");
        return false;
    }

    Serial.println("{\"status\":\"connecting\"}");

    if (!BLESerial.connect())
    {
        Serial.println("{\"status\":\"connect_failed\"}");
        return false;
    }

    myELM327.begin(BLESerial, false, 2000);
    obdState = READ_COOLANT;

    Serial.println("{\"status\":\"connected\"}");
    return true;
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    while (!connectToOBD())
        delay(3000);
}

void loop()
{
    if (!BLESerial.connected())
    {
        Serial.println("{\"status\":\"disconnected\"}");
        delay(2000);
        while (!connectToOBD())
            delay(3000);
        return;
    }

    switch (obdState)
    {
        case READ_COOLANT: {
            float val = myELM327.engineCoolantTemp();
            if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
                if (myELM327.nb_rx_state == ELM_SUCCESS) coolantTemp = val;
                obdState = READ_OIL;
            }
            break;
        }
        case READ_OIL: {
            float val = myELM327.oilTemp();
            if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
                if (myELM327.nb_rx_state == ELM_SUCCESS) oilTemp = val;
                obdState = READ_INTAKE;
            }
            break;
        }
        case READ_INTAKE: {
            float val = myELM327.intakeAirTemp();
            if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
                if (myELM327.nb_rx_state == ELM_SUCCESS) intakeTemp = val;
                obdState = READ_FUEL;
            }
            break;
        }
        case READ_FUEL: {
            float val = myELM327.fuelLevel();
            if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
                if (myELM327.nb_rx_state == ELM_SUCCESS) fuelLevel = val;
                obdState = READ_BATTERY;
            }
            break;
        }
        case READ_BATTERY: {
            float val = myELM327.batteryVoltage();
            if (myELM327.nb_rx_state != ELM_GETTING_MSG) {
                if (myELM327.nb_rx_state == ELM_SUCCESS) batteryVoltage = val;
                sendJson();
                obdState = READ_COOLANT;
            }
            break;
        }
    }
}

#endif
