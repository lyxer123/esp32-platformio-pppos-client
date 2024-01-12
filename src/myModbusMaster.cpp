// myModbusMaster.cpp
#include "myModbusMaster.h"
#include <ArduinoJson.h>
#include "FS.h"
#include "LittleFS.h"

std::vector<unsigned long> modbuspreviousMillis; // 存储每个操作的上一个时间戳
HardwareSerial *_mySerial;
std::vector<Node> nodes;

uint32_t serialConfig(const String &config)
{
    if (config == "SERIAL_5N1")
        return SERIAL_5N1;
    if (config == "SERIAL_6N1")
        return SERIAL_6N1;
    if (config == "SERIAL_7N1")
        return SERIAL_7N1;
    if (config == "SERIAL_8N1")
        return SERIAL_8N1;
    if (config == "SERIAL_5N2")
        return SERIAL_5N2;
    if (config == "SERIAL_6N2")
        return SERIAL_6N2;
    if (config == "SERIAL_7N2")
        return SERIAL_7N2;
    if (config == "SERIAL_8N2")
        return SERIAL_8N2;
    if (config == "SERIAL_5E1")
        return SERIAL_5E1;
    if (config == "SERIAL_6E1")
        return SERIAL_6E1;
    if (config == "SERIAL_7E1")
        return SERIAL_7E1;
    if (config == "SERIAL_8E1")
        return SERIAL_8E1;
    if (config == "SERIAL_5E2")
        return SERIAL_5E2;
    if (config == "SERIAL_6E2")
        return SERIAL_6E2;
    if (config == "SERIAL_7E2")
        return SERIAL_7E2;
    if (config == "SERIAL_8E2")
        return SERIAL_8E2;
    if (config == "SERIAL_5O1")
        return SERIAL_5O1;
    if (config == "SERIAL_6O1")
        return SERIAL_6O1;
    if (config == "SERIAL_7O1")
        return SERIAL_7O1;
    if (config == "SERIAL_8O1")
        return SERIAL_8O1;
    if (config == "SERIAL_5O2")
        return SERIAL_5O2;
    if (config == "SERIAL_6O2")
        return SERIAL_6O2;
    if (config == "SERIAL_7O2")
        return SERIAL_7O2;
    if (config == "SERIAL_8O2")
        return SERIAL_8O2;

    return SERIAL_8N1; // 默认配置
}

bool setupModbusFromConfig(const JsonObject& config) {
    if (config.isNull()) {
        Serial.println("Error: config is not a valid object.");
        return false;
    }

    // 从配置中解析出 Modbus 参数
    uint32_t baudRate = config["BaudRate"] | 9600;  // 默认值为 9600
    uint8_t rxPin = config["RxPin"];
    uint8_t txPin = config["TxPin"];
    String serialConfigStr = config["Config"] | "SERIAL_8N1";
    uint32_t Serconfig = serialConfig(serialConfigStr); // 此函数应将字符串转换为适当的 SERIAL_8N1 等格式
    uint8_t hardwareSerial = config["HardwareSerial"] | 2; // 默认使用 Serial2

    // 初始化 Modbus 通信
    begin(hardwareSerial, baudRate, Serconfig, rxPin, txPin);

    // 添加从设备（节点）
    JsonArray ops = config["Ops"];
    for (JsonObject op : ops) {
        uint8_t slaveId = op["SlaveId"];
        addNode(slaveId); // 此方法应添加一个新的 Modbus 节点
    }

    return true;
}

void begin(int serialPort, long baudRate, uint32_t config, uint8_t rxd, uint8_t txd)
{
    // 根据指定的serialPort参数初始化硬件串口
    switch (serialPort)
    {
    case 1:
        _mySerial = &Serial1;
        break;
    case 2:
        _mySerial = &Serial2;
        break;
    default:
        _mySerial = &Serial2; // 默认使用Serial2
    }

    // 配置并启动硬件串口
    _mySerial->begin(baudRate, config, rxd, txd);

    // 如果需要配置全局Modbus参数，可以在这里进行
    // 例如：设置超时时间，重试次数等
    // 注意：这取决于您的Modbus库功能和需求
}

void addNode(uint8_t id)
{
    ModbusMaster node;
    node.begin(id, *_mySerial);
    nodes.push_back({node, id});
}


String read(uint8_t nodeId, uint8_t functionCode, uint16_t address, uint8_t quantity, String customKey)
{ // 新参数

    DynamicJsonDocument doc(1024);
    String jsonString;
    for (Node &node : nodes)
    {
        if (node.id == nodeId)
        {
            uint8_t result;
            switch (functionCode)
            {
            case 0x01:
                result = node.instance.readCoils(address, quantity);
                break;
            case 0x02:
                result = node.instance.readDiscreteInputs(address, quantity);
                break;
            case 0x03:
                result = node.instance.readHoldingRegisters(address, quantity);
                break;
            case 0x04:
                result = node.instance.readInputRegisters(address, quantity);
                break;
            default:
                return "{\"Error\":\"Invalid function code\"}";
            }

            if (result == node.instance.ku8MBSuccess)
            {
                String key;
                if (customKey != "")
                {
                    key = customKey; // 如果提供了自定义键，则使用它
                }
                else
                {
                    key = String("Read-") + (functionCode < 10 ? "0" : "") + String(functionCode) + "-" + String(address) + "-" + String(quantity); // 否则，使用默认的键
                }
                JsonArray data = doc.createNestedArray(key);
                for (uint8_t i = 0; i < quantity; i++)
                {
                    data.add(node.instance.getResponseBuffer(i));
                }
            }
            else
            {
                doc[String("Error")] = String("Communication error: ") + String(result);
            }
            serializeJson(doc, jsonString);
            return jsonString;
        }
    }
    return "{\"Error\":\"Node not found\"}";
}

bool write(uint8_t nodeId, uint8_t functionCode, uint16_t address, uint16_t quantity, uint16_t *data)

{
    Node *nodePtr = nullptr;
    for (auto &node : nodes)
    {
        if (node.id == nodeId)
        {
            nodePtr = &node;
            break;
        }
    }

    if (nodePtr == nullptr)
    {
        Serial.println("Node not found.");
        return false;
    }

    ModbusMaster &node = nodePtr->instance;
    uint8_t result = 0; // 用于存储Modbus通信结果
    switch (functionCode)
    {
    case 0x06: // 写单个寄存器
        result = node.writeSingleRegister(address, *data);
        break;
    case 0x10: // 写多个寄存器
        // 首先，我们需要将数据放入传输缓冲区
        for (uint16_t i = 0; i < quantity; ++i)
        {
            node.setTransmitBuffer(i, data[i]);
        }
        // 然后，我们调用writeMultipleRegisters
        result = node.writeMultipleRegisters(address, quantity);
        break;
    // 根据需要添加其他功能码...
    default:
        Serial.print("Unsupported function code: ");
        Serial.println(functionCode);
        return false;
    }

    // 根据ModbusMaster库，成功的操作应返回ku8MBSuccess
    if (result == node.ku8MBSuccess)
    {
        return true;
    }
    else
    {
        Serial.print("Error writing data. Modbus result code: ");
        Serial.println(result);
        return false;
    }
}


String processModbusOperations(const JsonArray& ops) {
    String output;

    // 确保previousMillis向量足够大以容纳所有的操作
    if (modbuspreviousMillis.size() < ops.size()) {
        modbuspreviousMillis.resize(ops.size(), 0); // 新条目初始化为0
    }

    int opIndex = 0; // 用于跟踪当前操作的索引

    for (JsonObject op : ops) {
        unsigned long currentMillis = millis();
        unsigned long pollingInterval = op["PollingInterval"];

        // 检查是否到达了轮询间隔
        if (currentMillis - modbuspreviousMillis[opIndex] >= pollingInterval) {
            modbuspreviousMillis[opIndex] = currentMillis;  // 更新时间戳

            // Serial.println("processModbusOperations");

            uint8_t slaveId = op["SlaveId"];

            if (op.containsKey("AllinOne")) {
                // 如果是AllinOne操作
                JsonArray allInOneArray = op["AllinOne"].as<JsonArray>();
                String result = processAllInOneOperations(allInOneArray, slaveId);
                output += result + "\n"; // 添加结果到输出字符串
            } else {
                // 如果是单一操作
                uint8_t function = op["Function"];
                uint16_t address = op["Address"];
                uint8_t len = op["Len"];
                String customKey = op["DisplayName"]; // 使用DisplayName字段作为自定义键

                String readResult = read(slaveId, function, address, len, customKey);
                if (!readResult.isEmpty()) {
                    output += readResult + "\n"; // 添加结果到输出字符串
                } else {
                    output += "Error reading data from Modbus slave or no data returned.\n";
                }
            }
        }
        opIndex++; // 移动到下一个操作
    }

    return output;
}

String processAllInOneOperations(const JsonArray& allInOneArray, uint8_t slaveId) {
    DynamicJsonDocument doc(1024);
    JsonObject result = doc.to<JsonObject>();

    // 用于收集单个AllinOne操作中所有数据的容器
    DynamicJsonDocument allInOneDataDoc(1024);
    JsonObject allInOneData = allInOneDataDoc.to<JsonObject>();

    for (JsonObject op : allInOneArray) {
        uint8_t function = op["Function"];
        uint16_t address = op["Address"];
        uint8_t len = op["Len"];
        String customKey = op.containsKey("DisplayName") ? op["DisplayName"].as<String>() : "";

        String readResult = read(slaveId, function, address, len, customKey);

        if (!readResult.isEmpty()) {
            DynamicJsonDocument readDoc(1024);
            deserializeJson(readDoc, readResult);
            JsonObject readObj = readDoc.as<JsonObject>();

            // 选择正确的键用于组合最终JSON对象
            String key = customKey.isEmpty() ? ("Read-" + String(function) + "-" + String(address) + "-" + String(len)) : customKey;

            // 将单次读取的数据添加到AllinOne操作的数据集中
            allInOneData[key] = readObj[key];
        }
        delay(1000);   // 为了避免Modbus通信错误，我们在每次读取之间添加了100ms的延迟
    }

    // 如果AllinOne操作包含多个数据点，则这些数据都包含在allInOneData中
    String output;
    serializeJson(allInOneData, output);
    return output;
}