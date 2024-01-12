
#ifndef __MYWIFI_H_
#define __MYWIFI_H_

#include "Arduino.h"
#include <stddef.h>

#include <stdint.h>
#include <string.h>
#include <string>

#include <WiFi.h>
#include <AsyncUDP.h>                                                       //udp广播相关数据

#include <PubSubClient.h>                                                   // 版本2.8.0
#include "mySystem.h"

// #include "bingMapsGeocoding.h"
// #include "WifiLocation.h"
#define MQTT_NUMBERSOFTOPIC 10


extern WiFiClient myWiFiClient;
extern PubSubClient myWiFiMQTTClient;
extern String  subscribedTopics[MQTT_NUMBERSOFTOPIC];
//extern int numbersoftopic = 0;               // current position in subsribedTopics
extern String MQTTReceivedMessage;

//采用wifi模式升级固件，测试成功。
String  getHeaderValue(String header, String headerName) ;
void    execOTAusingWifi(String host, int port, String bin) ;

void display_connected_devices();
void get_wifinetwork_info();
void wifi_event(WiFiEvent_t event) ;
bool setup_wifi_sta(const char *myssid, const char *mypassword);
char setup_wifi_sta_ap(const char *myssid, const char *mypassword, const char *myAPssid, const char *myAPpassword);
bool try_connect_sta(const char *ssid, const char *password);
bool configure_ap_mode(const char *APssid, const char *APpassword) ;

void resetWiFi() ;

// void printMsg(String msg);
void publishWiFiMQTTMessage(String pInfoTopic,String output);

bool WiFiMQTTinit(String mqttHost, uint16_t mqttPort);

bool WifiMQTTConnect(String MQTT_CLIENT_ID, String MQTT_USERNAME,  String MQTT_PASSWORD) ;
bool AddSubscribeTopic(String Topic);
bool unSubscribeTopic(String Topic) ;
void WifiCallback(char *topic, byte *payload, unsigned int length);
void Publish_String(const char* subtopic, String value, bool fulltopic);
bool getWifiMQTTConnectstatus(void);

byte wifi_status();
char getWiFiAPSTAValue(void);

void WifiMqqtLoop(void);


void wifiBroadcastIP(const String myAPName);

//利用wifi实现定位功能
// location_t getWifiLocate(void);
#endif 