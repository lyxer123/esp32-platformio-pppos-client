#ifndef MYSYSTEM_H
#define MYSYSTEM_H

#include <Arduino.h> // 添加这行以引入 Arduino 库

#include <Preferences.h>

// 调试宏定义
#define DEBUG_ON

#ifdef DEBUG_ON
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTDEC(x) Serial.print(x, DEC)
#define DEBUG_PRINTHEX(x) Serial.print(x, HEX)

#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTLNDEC(x) Serial.println(x, DEC)
#define DEBUG_PRINTLNHEX(x) Serial.println(x, HEX)

#define PRINT(...) Serial.print(__VA_ARGS__)
#define PRINTF(...) Serial.printf(__VA_ARGS__)
#define PRINTLN(...) Serial.println(__VA_ARGS__)

#else

#define DEBUG_PRINT(x)
#define DEBUG_PRINTDEC(x)
#define DEBUG_PRINTHEX(x)

#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTLNDEC(x)
#define DEBUG_PRINTLNHEX(x)

#define PRINT(...)
#define PRINTF(...)
#define PRINTLN(...)

#endif

// 声明 Preferences 命名空间枚举
enum PreferEnum
{
    Modbus,
    IOTMQTTServer,
    CRON,
    General,
    MODULE4G,
    MQTTGeneral,
    MQTTSUB,
    Blockchain
};

// 相关的全局变量都放在这个结构体里面
// 前缀bp,bp前缀的是全局变量。后面EEPROM里面对应的是存芯片里面的数据
struct BoardParameterinEEPROM   // 存在EEPROM芯片里面的参数信息
{
  String webDirTable;
};
extern struct BoardParameterinEEPROM bpinfo; // 相关的全局变量都放在这个结构体里面

extern String myAPName; // 声明主板WiFi热点名称

// 定义用于版本控制的宏
#define OS_YEAR ((((__DATE__[7] - '0') * 10 + (__DATE__[8] - '0')) * 10 + (__DATE__[9] - '0')) * 10 + (__DATE__[10] - '0')) // 2022
#define OS_YEAR1 (((__DATE__[9] - '0')) * 10 + (__DATE__[10] - '0'))                                                        // 22
#define OS_MONTH (__DATE__[2] == 'n' ? (__DATE__[1] == 'a' ? 1 : 6) : __DATE__[2] == 'b' ? 2                            \
                                                                  : __DATE__[2] == 'r'   ? (__DATE__[0] == 'M' ? 3 : 4) \
                                                                  : __DATE__[2] == 'y'   ? 5                            \
                                                                  : __DATE__[2] == 'l'   ? 7                            \
                                                                  : __DATE__[2] == 'g'   ? 8                            \
                                                                  : __DATE__[2] == 'p'   ? 9                            \
                                                                  : __DATE__[2] == 't'   ? 10                           \
                                                                  : __DATE__[2] == 'v'   ? 11                           \
                                                                                         : 12) // 12
#define OS_DAY ((__DATE__[4] == ' ' ? 0 : __DATE__[4] - '0') * 10 + (__DATE__[5] - '0'))     // 31
#define OS_HOUR ((__TIME__[0] - '0') * 10 + (__TIME__[1] - '0'))                             // 22
#define OS_MINUTE ((__TIME__[3] - '0') * 10 + (__TIME__[4] - '0'))                           // 45
#define OS_SECOND ((__TIME__[6] - '0') * 10 + (__TIME__[7] - '0'))

long getEsp32TimeEpoch(void);
String RWBoardParameter(const char *method, const char *myParameter, const char *myvalue, PreferEnum preferNamespace);
String getNamespaceName(PreferEnum _preferNamespace);

bool feedExternalWatchdog(int watchdogPin);
bool externalHardwareWatchdog();
String GetESP32ShortId();
void monitorMemorysimple(bool DetailInformation);

String initBoardVersion(PreferEnum _preferNamespace);

bool updateJsonKeyValue(const char* filePath, const char* key, const char* newValue) ;
String getApName(String prefix);

#endif
