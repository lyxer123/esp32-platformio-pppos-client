// mySystem.cpp

#include "mySystem.h"
#include "Arduino.h"

#include "FS.h"
#include <LittleFS.h>

#include "ArduinoJson.h"

#include <ESP32Time.h>                            //rtc需要使用，lib库中的ESP32TIME库
ESP32Time esp32IntRTC;

String myAPName;                // 主板WiFi热点名称

int _watchdogPin;
Preferences SysPre;
PreferEnum _preferNamespace; // 保存传入的 Preferences 命名空间

struct BoardParameterinEEPROM bpinfo;

// 实现 initBoardVersion 函数
bool externalHardwareWatchdog()
{
    // 在这里执行外部硬件看门狗操作
    // 如果操作成功，返回true；否则返回false
    return true;
}

bool feedExternalWatchdog(int watchdogPin)
{
    digitalWrite(watchdogPin, HIGH); // 设置GPIO引脚高电平
    delay(50);                              // 延迟50ms
    digitalWrite(watchdogPin, LOW);  // 设置GPIO引脚低电平
    delay(50);                              // 延迟50ms
    return true;
}

String GetESP32ShortId()
{
    uint32_t id = 0;
    uint32_t Tempid;

    for (int i = 0; i < 17; i = i + 8)
    {
        id |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    Tempid = id >> 2; // 前面两位不需要

    return String(Tempid);
}

void monitorMemorysimple(bool DetailInformation)
{
    if (DetailInformation)
    {
        Serial.printf("Total heap    : %6d\n", ESP.getHeapSize());
        Serial.printf("Free heap     : %6d\n", ESP.getFreeHeap());
        Serial.printf("PSRAM found   : %s\n", psramFound() ? "Yes" : "No");
        Serial.printf("Total PSRAM   : %6d\n", ESP.getPsramSize());
        Serial.printf("Free PSRAM    : %6d\n", ESP.getFreePsram());
        Serial.printf("Used PSRAM    : %d\n", ESP.getPsramSize() - ESP.getFreePsram());
        Serial.printf("get Max Alloc Heap:  %6d\n", ESP.getMaxAllocHeap());
    }
}

String RWBoardParameter(const char *method, const char *myParameter, const char *myvalue, PreferEnum preferNamespace) {
    String myvalue1;

    // 使用 getNamespaceName 函数获取命名空间名称
    String namespaceName = getNamespaceName(preferNamespace);

    Preferences tempPre;
    tempPre.begin(namespaceName.c_str(), false);

    if (String(method) == "READ")
    {
        myvalue1 = tempPre.getString(myParameter, "");
        DEBUG_PRINT("READ ");
    }
    else if (String(method) == "WRITE")
    {
        myvalue1 = tempPre.getString(myParameter, "");
        if (myvalue1 != myvalue)
        {
            tempPre.putString(myParameter, myvalue);
            DEBUG_PRINT("WRITE ");
        }
        else
        {
            DEBUG_PRINT("SAME.NO Write:");
        }
    }

    DEBUG_PRINT(myParameter);
    DEBUG_PRINT(" IS:");
    DEBUG_PRINTLN(myvalue1);

    tempPre.end(); // 结束 Preferences
    return myvalue1;
}

// 实现 initBoardVersion 函数
String initBoardVersion(PreferEnum _preferNamespace)
{
    String Version;
    char retStr[32];
    sprintf(retStr, "%02d%02d%02d%02d%02d", OS_YEAR1, OS_MONTH, OS_DAY, OS_HOUR, OS_MINUTE);

    // 使用选择的命名空间进行写入
    RWBoardParameter("WRITE", "VERSION", retStr, _preferNamespace);

    return retStr;
}

// 实现选择 Preferences 命名空间的函数
String getNamespaceName(PreferEnum _preferNamespace)
{
    String namespaceName;
    switch (_preferNamespace)
    {
    case Modbus:
        namespaceName = "Modbus";
        break;
    case IOTMQTTServer:
        namespaceName = "IOTMQTTServer";
        break;
    case CRON:
        namespaceName = "CRON";
        break;
    case General:
        namespaceName = "General";
        break;
    case MODULE4G:
        namespaceName = "MODULE4G";
        break;
    case MQTTGeneral:
        namespaceName = "MQTTGeneral";
        break;
    case MQTTSUB:
        namespaceName = "MQTTSUB";
        break;
    case Blockchain:
        namespaceName = "Blockchain";
        break;

    // 添加其他 Preferences 命名空间的处理逻辑
    // ...
    default:
        // 默认使用 General 命名空间
        namespaceName = "General";
        break;
    }
    return namespaceName;
}

long getEsp32TimeEpoch(void)
{
  //return esp32IntRTC.getLocalEpoch();    
  return esp32IntRTC.getEpoch();                  //秒为单位
}

String getApName(String prefix)
{
  uint64_t chipid;
  String myAPName;

  // myAPName = prefix + GetESP32ID(chipid);
  myAPName = prefix + GetESP32ShortId();
  return myAPName;
}


/**
 * 更新存储在LittleFS文件系统中的JSON配置文件中指定键的值。
 *
 * @param filePath 指向存储JSON数据的文件的路径字符串。
 * @param key 想要更新的JSON键。
 * @param newValue 要为指定键设置的新值。
 * @return 如果成功更新了键的值则返回true，如果文件无法读取、键不存在、或无法写入新值则返回false。
 *
 * 函数执行的操作步骤包括：
 * 1. 打开指定路径的配置文件。
 * 2. 从文件中读取JSON数据到内存中。
 * 3. 解析JSON数据并检查指定的键是否存在。
 * 4. 如果键存在，更新其值。
 * 5. 删除原始文件并创建一个新文件以写入更新后的JSON数据。
 * 6. 写入新数据并关闭文件。
 * 
 * 注意：此函数假设JSON文件大小不超过1024字节。
 * 如果JSON文件可能更大，需要增加DynamicJsonDocument的大小。
 * 同时，因为涉及文件操作和JSON解析，调用此函数可能需要较多的堆内存。
 */
bool updateJsonKeyValue(const char* filePath, const char* key, const char* newValue) {
  // 打开文件
  File configFile = LittleFS.open(filePath, "r");
  if (!configFile) {
    Serial.println("Failed to open config file for reading");
    return false;
  }
 
  // 分配足够大的缓冲区来存储文件内容
  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }
  
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  configFile.close();

  // 解析JSON文档
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, buf.get());
  
  if (error) {
    Serial.println("Failed to parse config file");
    return false;
  }

  // 检查键是否存在
  if (!doc.containsKey(key)) {
    Serial.println("Key does not exist in the JSON document");
    return false;
  }

  // 更新键值
  doc[key] = newValue;

  // 删除旧文件
  LittleFS.remove(filePath);

  // 打开文件用于写入
  configFile = LittleFS.open(filePath, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  // 序列化JSON到文件
  if (serializeJson(doc, configFile) == 0) {
    Serial.println(F("Failed to write to config file"));
    return false;
  }

  // 关闭文件
  configFile.close();
  return true;
}