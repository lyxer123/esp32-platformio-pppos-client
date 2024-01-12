#ifndef MyModbusMaster_h
#define MyModbusMaster_h

#include "Arduino.h"
#include <ModbusMaster.h>
#include <ArduinoJson.h> // Include the ArduinoJson library
#include <vector>

struct Node
{
    ModbusMaster instance;
    uint8_t id;
};


void begin(int serialPort, long baudRate, uint32_t config, uint8_t rxd, uint8_t txd);
void addNode(uint8_t id);

bool setupModbusFromConfig(const JsonObject &config);

uint32_t serialConfig(const String &config);

String read(uint8_t nodeId, uint8_t functionCode, uint16_t address, uint8_t quantity, String customKey = "");
bool write(uint8_t nodeId, uint8_t functionCode, uint16_t address, uint16_t quantity, uint16_t *data);
// void processModbusOperations(const JsonArray& ops);
String processModbusOperations(const JsonArray &ops);

String processAllInOneOperations(const JsonArray &allInOneArray, uint8_t slaveId);

#endif
