#include "BLEClientSerial.h"

// ---------------------------------------------------------------------------
// File-scope state shared between the BLE callback classes and BLEClientSerial
// ---------------------------------------------------------------------------

static std::string  s_deviceName  = "";
static std::string  s_rxBuffer    = "";
static bool         s_connected   = false;
static BLEAdvertisedDevice *s_device = nullptr;

static BLEUUID s_serviceUUID("FFF0");
static BLEUUID s_rxUUID("FFF1");   // notifications from adapter → ESP32
static BLEUUID s_txUUID("FFF2");   // commands from ESP32 → adapter

// ---------------------------------------------------------------------------
// BLE notification callback — appends incoming bytes to the receive buffer
// ---------------------------------------------------------------------------

static void notifyCallback(BLERemoteCharacteristic *, uint8_t *pData, size_t length, bool)
{
    std::string chunk((char *)pData, length);
    if (s_rxBuffer.size() < length ||
        s_rxBuffer.substr(s_rxBuffer.size() - length) != chunk)
    {
        s_rxBuffer += chunk;
    }
}

// ---------------------------------------------------------------------------
// BLE callback classes
// ---------------------------------------------------------------------------

class MyClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *)    override { s_connected = true;  }
    void onDisconnect(BLEClient *) override { s_connected = false; }
};

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice dev) override
    {
        if (dev.getName() == s_deviceName)
        {
            Serial.printf("[BLE] Found: %s\n", s_deviceName.c_str());
            BLEDevice::getScan()->stop();
            s_device = new BLEAdvertisedDevice(dev);
        }
    }
};

// ---------------------------------------------------------------------------
// BLEClientSerial
// ---------------------------------------------------------------------------

BLEClientSerial::BLEClientSerial()  {}
BLEClientSerial::~BLEClientSerial() {}

bool BLEClientSerial::begin(const char *deviceName)
{
    s_deviceName = deviceName;
    s_device     = nullptr;
    s_connected  = false;
    s_rxBuffer.clear();

    BLEDevice::init("");

    BLEScan *scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    scan->setInterval(1349);
    scan->setWindow(449);
    scan->setActiveScan(true);
    scan->start(5, false);   // blocks for up to 5 s

    return s_device != nullptr;
}

bool BLEClientSerial::connect()
{
    if (s_device == nullptr)
    {
        Serial.println("[BLE] connect() called but no device found — run begin() first.");
        return false;
    }

    BLEClient *client = BLEDevice::createClient();
    client->setClientCallbacks(new MyClientCallback());

    if (!client->connect(s_device))
    {
        Serial.println("[BLE] connect() failed.");
        return false;
    }

    BLERemoteService *service = client->getService(s_serviceUUID);
    if (service == nullptr)
    {
        Serial.println("[BLE] Service FFF0 not found.");
        client->disconnect();
        return false;
    }

    pRxCharacteristic = service->getCharacteristic(s_rxUUID);
    pTxCharacteristic = service->getCharacteristic(s_txUUID);

    if (pRxCharacteristic == nullptr || pTxCharacteristic == nullptr)
    {
        Serial.println("[BLE] Required characteristics not found.");
        client->disconnect();
        return false;
    }

    if (pRxCharacteristic->canNotify())
        pRxCharacteristic->registerForNotify(notifyCallback, true);

    Serial.printf("[BLE] Connected to %s\n", s_deviceName.c_str());
    return true;
}

bool BLEClientSerial::connected()
{
    return s_connected;
}

int BLEClientSerial::available()
{
    return (int)s_rxBuffer.size();
}

int BLEClientSerial::peek()
{
    if (s_rxBuffer.empty()) return -1;
    return (uint8_t)s_rxBuffer[0];
}

int BLEClientSerial::read()
{
    if (s_rxBuffer.empty()) return -1;
    uint8_t c = (uint8_t)s_rxBuffer[0];
    s_rxBuffer.erase(0, 1);
    return c;
}

size_t BLEClientSerial::write(uint8_t c)
{
    if (pTxCharacteristic == nullptr) return 0;
    pTxCharacteristic->writeValue(&c, 1, false);
    return 1;
}

size_t BLEClientSerial::write(const uint8_t *buffer, size_t size)
{
    if (pTxCharacteristic == nullptr) return 0;
    pTxCharacteristic->writeValue(const_cast<uint8_t *>(buffer), size, false);
    return size;
}

void BLEClientSerial::flush()
{
    s_rxBuffer.clear();
}

void BLEClientSerial::end()
{
    // nothing — BLEDevice lifecycle is managed externally
}
