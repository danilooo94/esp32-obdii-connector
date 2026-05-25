#ifndef _BLE_CLIENT_SERIAL_H_
#define _BLE_CLIENT_SERIAL_H_

#include "Arduino.h"
#include "Stream.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

class BLEClientSerial : public Stream
{
public:
    BLEClientSerial(void);
    ~BLEClientSerial(void);

    bool   begin(const char *deviceName);
    bool   connect(void);
    bool   connected(void);
    void   end(void);

    int    available(void) override;
    int    peek(void)      override;
    int    read(void)      override;
    size_t write(uint8_t c) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    void   flush() override;

private:
    BLERemoteCharacteristic *pTxCharacteristic = nullptr;
    BLERemoteCharacteristic *pRxCharacteristic = nullptr;
};

#endif
