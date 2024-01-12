#include "Arduino.h"
#include <Update.h>

#include <iostream>
#include <sstream>
#include "esp_wifi.h"
#include <WiFiClient.h>

#include "mySystem.h"
#include "myWifi.h"


#define READ_OTA_BUFFER (1024)

#define MAX_CONNECT_ATTEMPTS 10
#define CONNECT_DELAY_MS 500

WiFiClient myWiFiClient;
PubSubClient myWiFiMQTTClient;

String subscribedTopics[MQTT_NUMBERSOFTOPIC];
int numbersoftopic = 0;                             // current position in subsribedTopics
String MQTTReceivedMessage;

String g_time;

long contentLengthwifi = 0;
bool isValidContentTypewifi = false;

IPAddress local_AP_IP (192, 168, 4, 123);            // 指定AP的IP地址，这个可以写死，方便在没有连网情况下登录后台
IPAddress AP_gateway  (192, 168, 4, 9  );
IPAddress AP_subnet   (255, 255, 255, 0);

char myWiFiAPSTAvalue=0;
bool WifiMQTTConnectstatus;

float lat = 29.56481;
float lon = 106.45042;
AsyncUDP udp;

// 如下三个函数源码来自：https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
//当WiFi连接成功
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  DEBUG_PRINTLN("Connected to AP successfully!");
}

//当WiFi得到IP
void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  DEBUG_PRINTLN("WiFi connected");
  DEBUG_PRINTLN("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());

  udp.broadcastTo(WiFi.localIP().toString().c_str(), 2255);  
}

//当WiFi连接断线后事件
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  DEBUG_PRINTLN("Disconnected from WiFi access point");
  DEBUG_PRINT("WiFi lost connection. Reason: ");
  DEBUG_PRINTLN(info.wifi_sta_disconnected.reason);
  // DEBUG_PRINTLN("Trying to Reconnect");                   //这里需要重连，但是这里的ssid和password需要用参数导入
  // WiFi.begin(ssid, password);
}

void WiFiAPSTART(WiFiEvent_t event, WiFiEventInfo_t info)
{
  DEBUG_PRINT("WiFi AP Start. Reason: ");
  
}

// 源码来自：https://wokwi.com/projects/332120679373603412
void wifi_event(WiFiEvent_t event)
{
  Serial.printf("[WiFi-event] event: %d -> ", event);

  switch (event)
  {
  case SYSTEM_EVENT_WIFI_READY:
    DEBUG_PRINTLN("WiFi interface ready");
    break;
  case SYSTEM_EVENT_SCAN_DONE:
    DEBUG_PRINTLN("Completed scan for access points");
    break;
  case SYSTEM_EVENT_STA_START:
    DEBUG_PRINTLN("WiFi client started");
    break;
  case SYSTEM_EVENT_STA_STOP:
    DEBUG_PRINTLN("WiFi clients stopped");
    break;
  case SYSTEM_EVENT_STA_CONNECTED:
    DEBUG_PRINTLN("Connected to access point");
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    DEBUG_PRINTLN("Disconnected from WiFi access point");
    break;
  case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
    DEBUG_PRINTLN("Authentication mode of access point has changed");
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    DEBUG_PRINT("Obtained IP address: ");
    DEBUG_PRINTLN(WiFi.localIP());
    udp.broadcastTo(WiFi.localIP().toString().c_str(), 2255);
    break;
  case SYSTEM_EVENT_STA_LOST_IP:
    DEBUG_PRINTLN("Lost IP address and IP address is reset to 0");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
    DEBUG_PRINTLN("WiFi Protected Setup (WPS): succeeded in enrollee mode");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_FAILED:
    DEBUG_PRINTLN("WiFi Protected Setup (WPS): failed in enrollee mode");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
    DEBUG_PRINTLN("WiFi Protected Setup (WPS): timeout in enrollee mode");
    break;
  case SYSTEM_EVENT_STA_WPS_ER_PIN:
    DEBUG_PRINTLN("WiFi Protected Setup (WPS): pin code in enrollee mode");
    break;
  case SYSTEM_EVENT_AP_START:
    DEBUG_PRINTLN("WiFi access point started");
    break;
  case SYSTEM_EVENT_AP_STOP:
    DEBUG_PRINTLN("WiFi access point  stopped");
    break;
  case SYSTEM_EVENT_AP_STACONNECTED:
    DEBUG_PRINTLN("Client connected");
    break;
  case SYSTEM_EVENT_AP_STADISCONNECTED:
    DEBUG_PRINTLN("Client disconnected");
    break;
  case SYSTEM_EVENT_AP_STAIPASSIGNED:
    DEBUG_PRINTLN("Assigned IP address to client");
    break;
  case SYSTEM_EVENT_AP_PROBEREQRECVED:
    DEBUG_PRINTLN("Received probe request");
    break;
  case SYSTEM_EVENT_GOT_IP6:
    DEBUG_PRINTLN("IPv6 is preferred");
    break;
  case SYSTEM_EVENT_ETH_START:
    DEBUG_PRINTLN("Ethernet started");
    break;
  case SYSTEM_EVENT_ETH_STOP:
    DEBUG_PRINTLN("Ethernet stopped");
    break;
  case SYSTEM_EVENT_ETH_CONNECTED:
    DEBUG_PRINTLN("Ethernet connected");
    break;
  case SYSTEM_EVENT_ETH_DISCONNECTED:
    DEBUG_PRINTLN("Ethernet disconnected");
    break;
  case SYSTEM_EVENT_ETH_GOT_IP:
    DEBUG_PRINTLN("Obtained IP address");
    break;
  default:
    break;
  }
}

byte wifi_status()
{
  // show WiFi status
  byte status = WiFi.status();
  static byte previousStatus = WL_DISCONNECTED;
  if (status != previousStatus)
  {
    previousStatus = status;
    DEBUG_PRINT(status);
    switch (status)
    {
    case WL_IDLE_STATUS:
      DEBUG_PRINTLN(" WL_IDLE_STATUS");
      break;
    case WL_NO_SSID_AVAIL:
      DEBUG_PRINTLN(" WL_NO_SSID_AVAIL");
      break;
    case WL_SCAN_COMPLETED:
      DEBUG_PRINTLN(" WL_SCAN_COMPLETED");
      break;
    case WL_CONNECTED:
      DEBUG_PRINTLN(" WL_CONNECTED");
      break;
    case WL_CONNECT_FAILED:
      DEBUG_PRINTLN(" WL_CONNECT_FAILED");
      break;
    case WL_CONNECTION_LOST:
      DEBUG_PRINTLN(" WL_CONNECTION_LOST");
      break;
    case WL_DISCONNECTED:
      DEBUG_PRINTLN(" WL_DISCONNECTED");
      break;
    default:
      DEBUG_PRINTLN(" unknown");
    }
  }
  return status;
}

// Utility to extract header value from headers
String getHeaderValue(String header, String headerName)
{
  return header.substring(strlen(headerName.c_str()));
}

/*
  采用http方式获取bin文件，由于wifi速度比较快，测试是没有问题的。
  host: http://www.bldxny.com
  port: 80
  bin:"/bin/myupdate.bin"
*/
void execOTAusingWifi(String host, int port, String bin) // 测试没问题
{
  // myWiFiClient.setInsecure();
  DEBUG_PRINTLN("Connecting to: " + String(host));
  if (myWiFiClient.connect(host.c_str(), port)) // Connect to bldxny.com
  {
    DEBUG_PRINTLN("Fetching Bin: " + String(bin));             // Connection Succeed.Fecthing the bin
    myWiFiClient.print(String("GET ") + bin + " HTTP/1.1\r\n" + // Get the contents of the bin file
                       "Host: " + host + "\r\n" +
                       "Cache-Control: no-cache\r\n" +
                       "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (myWiFiClient.available() == 0)
    {
      if (millis() - timeout > 5000)
      {
        DEBUG_PRINTLN("myWiFiClient Timeout !");
        myWiFiClient.stop();
        return;
      }
    }
    // Once the response is available,
    // check stuff

    /*
       Response Structure
        HTTP/1.1 200 OK
        x-amz-id-2: NVKxnU1aIQMmpGKhSwpCBh8y2JPbak18QLIfE+OiUDOos+7UftZKjtCFqrwsGOZRN5Zee0jpTd0=
        x-amz-request-id: 2D56B47560B764EC
        Date: Wed, 14 Jun 2017 03:33:59 GMT
        Last-Modified: Fri, 02 Jun 2017 14:50:11 GMT
        ETag: "d2afebbaaebc38cd669ce36727152af9"
        Accept-Ranges: bytes
        Content-Type: application/octet-stream
        Content-Length: 357280
        Server: AmazonS3

        {{BIN FILE CONTENTS}}
    */
    while (myWiFiClient.available())
    {
      String line = myWiFiClient.readStringUntil('\n'); // read line till /n
      line.trim();                                      // remove space, to check if the line is end of headers

      // if the the line is empty,this is end of headers break the while and feed the
      // remaining `myWiFiClient` to the Update.writeStream();

      if (!line.length())
      {
        // headers ended
        break; // and get the OTA started
      }

      if (line.startsWith("HTTP/1.1")) // Check if the HTTP Response is 200  else break and Exit Update
      {
        if (line.indexOf("200") < 0)
        {
          DEBUG_PRINTLN("Got a non 200 status code from server. Exiting OTA Update.");
          break;
        }
      }

      if (line.startsWith("Content-Length: ")) // extract headers here Start with content length
      {
        contentLengthwifi = atol((getHeaderValue(line, "Content-Length: ")).c_str());
        DEBUG_PRINTLN("Got " + String(contentLengthwifi) + " bytes from server");
      }

      if (line.startsWith("Content-Type: ")) // Next, the content type
      {
        String contentType = getHeaderValue(line, "Content-Type: ");
        DEBUG_PRINTLN("Got " + contentType + " payload.");
        if (contentType == "application/octet-stream")
        {
          isValidContentTypewifi = true;
        }
      }
    }
  }
  else
  {
    // Connect to S3 failed May be try? Probably a choppy network?
    DEBUG_PRINTLN("Connection to " + String(host) + " failed. Please check your setup");
    // retry??
    // execOTA();
  }

  // Check what is the contentLength and if content type is `application/octet-stream`
  DEBUG_PRINTLN("contentLength : " + String(contentLengthwifi) + ", isValidContentType : " + String(isValidContentTypewifi));

  if (contentLengthwifi && isValidContentTypewifi) // check contentLength and content type
  {
    bool canBegin = Update.begin(contentLengthwifi); // Check if there is enough to OTA Update
    if (canBegin)                                    // If yes, begin
    {
      DEBUG_PRINTLN("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
      // No activity would appear on the Serial monitor So be patient. This may take 2 - 5mins to complete

      uint32_t written = 0;
      uint8_t buffer[READ_OTA_BUFFER];
      while (1)
      {
        // 读数据到数组里面。
        int read_bytes = myWiFiClient.readBytes(buffer, min((uint32_t)READ_OTA_BUFFER, (uint32_t)(contentLengthwifi - written)));
        if (read_bytes == 0)
        {
          break;
        }
        Update.write(buffer, read_bytes);
        written += read_bytes;
        Serial.printf("Write %.02f%% (%ld/%ld)\n", (float)written / (float)contentLengthwifi * 100.0, written, contentLengthwifi);

        if (written == contentLengthwifi)
        {
          break;
        }
      }

      if (written == contentLengthwifi)
      {
        DEBUG_PRINTLN("Written : " + String(written) + " successfully");
      }
      else
      {
        DEBUG_PRINTLN("Written only : " + String(written) + "/" + String(contentLengthwifi) + ". Retry?");
        // retry??
        // execOTA();
      }

      if (Update.end())
      {
        DEBUG_PRINTLN("OTA done!");
        if (Update.isFinished())
        {
          DEBUG_PRINTLN("Update successfully completed. Rebooting.");
          ESP.restart();
        }
        else
        {
          DEBUG_PRINTLN("Update not finished? Something went wrong!");
        }
      }
      else
      {
        DEBUG_PRINTLN("Error Occurred. Error #: " + String(Update.getError()));
      }
    }
    else
    {
      // not enough space to begin OTA Understand the partitions and space availability
      DEBUG_PRINTLN("Not enough space to begin OTA");
      myWiFiClient.flush();
    }
  }
  else
  {
    DEBUG_PRINTLN("There was no content in the response");
    myWiFiClient.flush();
  }
}

/*
 esp32工作在STA模式下，获取自己的相关信息，如IP，RSSI等数据
 参考源码：https://www.upesy.com/blogs/tutorials/how-to-connect-wifi-acces-point-with-esp32
 测试结果：Connected to the WiFi network
         [*] Network information for HUAWEI_**
         [+] BSSID : F0:43:47:32:1F:4D
         [+] Gateway IP : 192.168.43.1
         [+] Subnet Mask : 255.255.255.0
         [+] RSSI : -25 dB
         [+] ESP32 IP : 192.168.43.129
*/

void get_wifinetwork_info()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    // DEBUG_PRINT("[*] Network information for ");
    // DEBUG_PRINTLN(ssid);

    DEBUG_PRINTLN("[+] BSSID : " + WiFi.BSSIDstr());
    DEBUG_PRINT("[+] Gateway IP : ");
    DEBUG_PRINTLN(WiFi.gatewayIP());
    DEBUG_PRINT("[+] Subnet Mask : ");
    DEBUG_PRINTLN(WiFi.subnetMask());
    DEBUG_PRINTLN((String) "[+] RSSI : " + WiFi.RSSI() + " dB");
    DEBUG_PRINT("[+] ESP32 IP : ");
    DEBUG_PRINTLN(WiFi.localIP());

    udp.broadcastTo(WiFi.localIP().toString().c_str(), 2255);
  }
}

/*
 在AP模式下，获取连接到esp32 AP上的各个接入信息，显示其IP等数据
 源码参考：https://www.upesy.com/blogs/tutorials/how-create-a-wifi-acces-point-with-esp32
 测试结果如下：
    [+] Device 0 | MAC : 44:E2:42:08:D7:11 | IP 192.168.4.2
    [+] Device 1 | MAC : 00:0A:F7:42:42:42 | IP 192.168.4.3
    其中esp_wifi_ap_get_sta_list需要包含头文件#include "esp_wifi.h"
*/
void display_connected_devices()
{
  wifi_sta_list_t wifi_sta_list;
  tcpip_adapter_sta_list_t adapter_sta_list;
  esp_wifi_ap_get_sta_list(&wifi_sta_list);
  tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);

  if (adapter_sta_list.num > 0)
    DEBUG_PRINTLN("-----------");
  for (uint8_t i = 0; i < adapter_sta_list.num; i++)
  {
    tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
    DEBUG_PRINT((String) "[+] Device " + i + " | MAC : ");
    Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X", station.mac[0], station.mac[1], station.mac[2], station.mac[3], station.mac[4], station.mac[5]);
    // DEBUG_PRINTLN((String) " | IP " + ip4addr_ntoa(&(station.ip)));
  }
}

/* This routine is specifically geared for ESP32 perculiarities */
/**
 * @brief 本程序是用ssid和password以STA模式设置wifi。
 * @param void
 * @return long
 */
bool setup_wifi_sta(const char *myssid, const char *mypassword)
{
  char wificounter;
  if (WiFi.status() == WL_CONNECTED)
  {
    return true;
  }

  DEBUG_PRINTLN();
  DEBUG_PRINT("Connecting to ");
  DEBUG_PRINTLN(myssid);

  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.persistent(false);
    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_STA);

    WiFi.begin(myssid, mypassword);
  }

  wificounter = 0;
  while (WiFi.status() != WL_CONNECTED && wificounter < 10)
  {
    for (int i = 0; i < 500; i++)
    {
      delay(1);
    }
    DEBUG_PRINT(".");
    wificounter++;
  }

  if (wificounter >= 10)
  {
    DEBUG_PRINTLN("Restarting ...");
    // ESP.restart();                        //targetting 8266 & Esp32 - you may need to replace this
    return false;
  }

  delay(10);
  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("WiFi connected.");
  DEBUG_PRINTLN("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP()); 

  return true;
}

char setup_wifi_sta_ap(const char *myssid, const char *mypassword, const char *myAPssid, const char *myAPpassword) {
  char wificounter;

  // 尝试以STA方式连接
  if (try_connect_sta(myssid, mypassword)) {
    myWiFiAPSTAvalue = 1;
    return 1;
  } else {
    // 连接失败，设置AP模式
    if (configure_ap_mode(myAPssid, myAPpassword)) {
      myWiFiAPSTAvalue = 2;
      return 2;
    } else {
      // 如果AP模式也失败，则根据需求做进一步处理
      // 这里可以根据实际情况返回错误码或进行错误处理
      return -1;
    }
  }
}

bool try_connect_sta(const char *ssid, const char *password) {
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);

  WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid, password);

  for (char wificounter = 0; wificounter < MAX_CONNECT_ATTEMPTS; ++wificounter) {
    delay(CONNECT_DELAY_MS);  // 简化的延迟，避免不必要的内循环
    if (WiFi.status() == WL_CONNECTED) {
      DEBUG_PRINTLN("WiFi connected. IP address:");
      DEBUG_PRINTLN(WiFi.localIP());
      udp.broadcastTo(WiFi.localIP().toString().c_str(), 2255);
      return true;
    }
    DEBUG_PRINT(".");
  }

  return false; // 连接失败
}

bool configure_ap_mode(const char *APssid, const char *APpassword) 
{
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_MODE_APSTA);

  if (!WiFi.softAPConfig(local_AP_IP, AP_gateway, AP_subnet)) {
    DEBUG_PRINTLN("Setting soft-AP configuration failed!");
    return false;
  }

  if (!WiFi.softAP(APssid, APpassword)) {
    DEBUG_PRINTLN("Setting soft-AP failed!");
    return false;
  }

  DEBUG_PRINT("ESP32 IP as soft AP: ");
  DEBUG_PRINTLN(WiFi.softAPIP());
  return true;
}


/*
  首先用写入的ssid和pwd以STA方式联网，如果发现联网不成功就转成AP模式，提供AP的SSID和pwd方式供以web server方式连入修改STA热点数据
  经过测试，1.如果STA模式下SSID和PWD不对，可直接切换到AP模式；提供热点，并可使用web server。
           2.如果STA模式下SSID和PWD对，可连入wifi路由器，并可使用web server，也可以wifi_scan。
  返回1：代表以sta方式连入了wifi路由器；返回2，代表以AP方式启动softAP热点。
*/
// char setup_wifi_sta_ap(const char *myssid, const char *mypassword, const char *myAPssid, const char *myAPpassword)
// {
//   char wificounter;

//   // 以STA方式设置如下参数
//   DEBUG_PRINTLN("get WiFi.status.");
//   if (WiFi.status() == WL_CONNECTED)                      // 如果WiFi网络已经连接上 
//   {
//     DEBUG_PRINTLN("WiFi connected.IP address:");
//     DEBUG_PRINTLN(WiFi.localIP());
//     udp.broadcastTo(WiFi.localIP().toString().c_str(), 2255);
//     myWiFiAPSTAvalue=1;
//     return 1;
//   }
//   else
//   {
//     WiFi.persistent(false);                                   // 先用STA连网络
//     WiFi.mode(WIFI_OFF);
    
//     //加入几个WiFi事件
//     WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
//     WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
//     WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

//     //WiFi.onEvent(WiFiAPSTART, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_START);

//     DEBUG_PRINTLN();
//     DEBUG_PRINT("Connecting to ");
//     DEBUG_PRINTLN(myssid);
//     WiFi.mode(WIFI_MODE_STA);                                 
//     WiFi.begin(myssid, mypassword);
//     wificounter = 0;
//     while (WiFi.status() != WL_CONNECTED && wificounter < 10) // 尽可能连接网络
//     {
//       for (int i = 0; i < 500; i++)
//       {
//         delay(1);
//       }
//       DEBUG_PRINT(".");
//       wificounter++;
//     }

//     if (wificounter >= 10)
//     {
//       DEBUG_PRINTLN("Setting to AP and STA mode ...");

//       WiFi.mode(WIFI_OFF);                                    // 实在联系不上网络，切换到AP模式
//       delay(100);

//       WiFi.mode(WIFI_MODE_APSTA);

//       DEBUG_PRINT("Setting soft-AP configuration ... ");
//       DEBUG_PRINTLN(WiFi.softAPConfig(local_AP_IP, AP_gateway, AP_subnet) ? "Ready" : "Failed!");

//       DEBUG_PRINT("Setting soft-AP ... ");
//       DEBUG_PRINTLN(WiFi.softAP(myAPssid, myAPpassword) ? "Ready" : "Failed!");

//       WiFi.softAP(myAPssid, myAPpassword);
//       DEBUG_PRINT("ESP32 IP as soft AP: ");
//       DEBUG_PRINTLN(WiFi.softAPIP());

//       myWiFiAPSTAvalue=2;
//       return 2;
//     }
//     else
//     {
//       DEBUG_PRINTLN("WiFi connected.IP address:");
//       DEBUG_PRINTLN(WiFi.localIP());
//       udp.broadcastTo(WiFi.localIP().toString().c_str(), 2255);
//       myWiFiAPSTAvalue=1;
//       return 1;
//     }
//   }
// }

//复位WiFi
void resetWiFi()
{
  // Set WiFi to station mode
  // and disconnect from an AP if it was previously connected
  // ESP_LOGD(TAG, "Resetting Wifi");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
}

//修改WiFi显示名称
void setWiFiHostname(String hostname)
{
  WiFi.setHostname(hostname.c_str());
}

// 这个初始化必须放在modbus config后面
bool WiFiMQTTinit(String mqttHost, uint16_t mqttPort)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    myWiFiMQTTClient.setClient(myWiFiClient);
    myWiFiMQTTClient.setServer(mqttHost.c_str(), mqttPort);
    myWiFiMQTTClient.setCallback(WifiCallback);
    myWiFiMQTTClient.setBufferSize(1024);
    myWiFiMQTTClient.setKeepAlive(10);
    DEBUG_PRINTLN("WiFi MQTT init success.");
    return true;
  }
  else
  {
    DEBUG_PRINTLN("WiFi MQTT init false.wifi not WL_CONNECTED");
    return false;
  }
}

bool getWifiMQTTConnectstatus(void)
{
  return WifiMQTTConnectstatus;
}

// mqtt client断网，重连，并重新订阅topic,这里通过将列表数据加入订阅
bool WifiMQTTConnect(String MQTT_CLIENT_ID, String MQTT_USERNAME, String MQTT_PASSWORD)
{
  char tryoutnumber=0;;

  while (!myWiFiMQTTClient.connected()) // Loop until we're reconnected
  {
    //DEBUG_PRINTLN("$$$$$!myWiFiMQTTClient.connected()$$$$");
    // Attempt to connect
    if(MQTT_USERNAME!="")
    {
      if (myWiFiMQTTClient.connect(MQTT_CLIENT_ID.c_str(), MQTT_USERNAME.c_str(), MQTT_PASSWORD.c_str()))  //这里不好连接
      {
        DEBUG_PRINTLN("MQTT connected1");

        //myWiFiMQTTClient.subscribe("test_subscribe");       // 加入一个测试订阅，后期需要删除，刘永相

        for (int i = 0; i < numbersoftopic; i++)
        {
          myWiFiMQTTClient.subscribe(subscribedTopics[i].c_str());
          DEBUG_PRINT("subscribe topic[");
          DEBUG_PRINT(i);
          DEBUG_PRINTLN("]:" + subscribedTopics[i]);       // 查看是否订阅mqtt数据
        }
        WifiMQTTConnectstatus=true;
        return true;
      }
      else
      {
        DEBUG_PRINT("MQTT not connected1, rc="+String(myWiFiMQTTClient.state()));
        DEBUG_PRINTLN(" try again in 5 seconds");

        delay(5000);
        //myWiFiMQTTClient.disconnect();
        if(tryoutnumber++==5)
        {
          WifiMQTTConnectstatus=false;
          return false;
        }
      }
    }
    else
    {
      if (myWiFiMQTTClient.connect(MQTT_CLIENT_ID.c_str()))      //这里不好连接
      {
        DEBUG_PRINTLN("MQTT connected2");

        //myWiFiMQTTClient.subscribe("test_subscribe");       // 加入一个测试订阅，后期需要删除，刘永相

        for (int i = 0; i < numbersoftopic; i++)
        {
          myWiFiMQTTClient.subscribe(subscribedTopics[i].c_str());
          DEBUG_PRINT("subscribe topic[");
          DEBUG_PRINT(i);
          DEBUG_PRINTLN("]:" + subscribedTopics[i]);       // 查看是否订阅mqtt数据
        }
        WifiMQTTConnectstatus=true;
        return true;
      }
      else
      {
        DEBUG_PRINT("MQTT not connected2, rc="+String(myWiFiMQTTClient.state()));
        DEBUG_PRINTLN(" try again in 5 seconds");

        delay(5000);
        //myWiFiMQTTClient.disconnect();
        if(tryoutnumber++==5)
        {
          WifiMQTTConnectstatus=false;
          return false;
        }          
      }
    }
  }
}

// 通过WiFi发布信息到平台
void publishWiFiMQTTMessage(String pInfoTopic, String output)
{
  DEBUG_PRINTLN("publish WiFi MQTT Message");
  myWiFiMQTTClient.publish(pInfoTopic.c_str(), output.c_str());
}

// 修改自：https://github.com/tobiasfaust/SolaxModbusGateway/
void Publish_Bool(const char *subtopic, bool b, bool fulltopic)
{
  String s;
  if (b)
  {
    s = "1";
  }
  else
  {
    s = "0";
  };
  Publish_String(subtopic, s, fulltopic);
}

// 修改自：https://github.com/tobiasfaust/SolaxModbusGateway/
void Publish_Int(const char *subtopic, int number, bool fulltopic)
{
  char buffer[20] = {0};
  memset(buffer, 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%d", number);
  Publish_String(subtopic, (String)buffer, fulltopic);
}

/* 
  修改自：https://github.com/tobiasfaust/SolaxModbusGateway/
*/
void Publish_Float(const char *subtopic, float number, bool fulltopic)
{
  char buffer[10] = {0};
  memset(&buffer[0], 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%.2f", number);
  Publish_String(subtopic, (String)buffer, fulltopic);
}

/*
  参考：https://github.com/tobiasfaust/SolaxModbusGateway/blob/master/src/mqtt.cpp
*/
void Publish_String(const char *subtopic, String value, bool fulltopic)
{
  char topic[50] = {0};
  memset(topic, 0, sizeof(topic));
  if (fulltopic)
  {
    snprintf(topic, sizeof(topic), "%s", subtopic);
  }
  else
  {
    snprintf(topic, sizeof(topic), "%s", subtopic);
    //DEBUG_PRINTf("Publish %s: %s \n", subtopic, value.c_str());
    //snprintf(topic, sizeof(topic), "%s/%s/%s", this->mqtt_basepath.c_str(), this->mqtt_root.c_str(), subtopic);
  }
  if (myWiFiMQTTClient.connected())
  {
    myWiFiMQTTClient.publish((const char *)topic, value.c_str(), true);
    // if (Config->GetDebugLevel() >=3) {DEBUG_PRINT(F("Publish ")); DEBUG_PRINT(FPSTR(topic)); DEBUG_PRINT(F(": ")); DEBUG_PRINTLN(value);}
    // } else { if (Config->GetDebugLevel() >=2) {DEBUG_PRINTLN(F("Request for MQTT Publish, but not connected to Broker")); }
  }
}

// 修改自：https://github.com/tobiasfaust/SolaxModbusGateway/
void Publish_IP()
{
  char buffer[15] = {0};
  memset(&buffer[0], 0, sizeof(buffer));
  snprintf(buffer, sizeof(buffer), "%s", WiFi.localIP().toString().c_str());
  Publish_String("IP", buffer, false);
}

// 增加订阅Topic，并在列表中增加
bool AddSubscribeTopic(String Topic)
{
  DEBUG_PRINTLN("Add Subscribe Topic:");
  myWiFiMQTTClient.subscribe(Topic.c_str()); // 新增订阅

  bool alreadySubscribed = false; // 保存订阅到数组中并用于reconnect使用
  for (int i = 0; i < MQTT_NUMBERSOFTOPIC; i++)
  {
    if (subscribedTopics[i].equals(Topic))
    {
      alreadySubscribed = true;
    }
  }
  if (!alreadySubscribed)
  {
    if (numbersoftopic == MQTT_NUMBERSOFTOPIC) // 数组满了
    {
      DEBUG_PRINT("Topic will not be resubscribed on reconnect! Maximum is ");
      DEBUG_PRINT(MQTT_NUMBERSOFTOPIC);
      return false;
    }
    else
    {
      subscribedTopics[numbersoftopic] = Topic;
      numbersoftopic++;
      return true;
    }
  }
}

// 将Topic取消，并在列表中删除
bool unSubscribeTopic(String Topic)
{
  myWiFiMQTTClient.unsubscribe(Topic.c_str()); // 删除订阅

  int position = -1;                            // 在列表中删除
  for (int i = 0; i < MQTT_NUMBERSOFTOPIC; i++) // 在现有列表中找出来
  {
    if (subscribedTopics[i].equals(Topic))
    {
      position = i;
    }
  }

  if (position >= 0) // if Topic is in list, overwrite it with the last Topic in list
  {
    numbersoftopic--;
    subscribedTopics[position] = subscribedTopics[numbersoftopic];
    subscribedTopics[numbersoftopic] = ""; // can be overwritten because it was copied to subscribedTopics[position].
    return true;
  }
}

/* Mqtt回调 */
void WifiCallback(char *topic, byte *payload, unsigned int length)
{
  //{"Topic":"/Coil/sub/1/3","Value":[1,2,3]}

  //重新组装成json格式，方便后面处理。
  MQTTReceivedMessage = "{\"Topic\":\"";
  MQTTReceivedMessage.concat(topic);
  MQTTReceivedMessage.concat("\",\"Value\":");
  for (int i = 0; i < length; i++)
  {
    MQTTReceivedMessage.concat((char)payload[i]);
  }
  MQTTReceivedMessage.concat("");
  MQTTReceivedMessage.concat("}");


  //将收到的数据压入LiFo和FiFo里面
  DEBUG_PRINT("MQTT Received Message:");
  DEBUG_PRINTLN(MQTTReceivedMessage);
}

void WifiMqqtLoop(void)
{
  myWiFiMQTTClient.loop();
}

char getWiFiAPSTAValue(void)
{
  //STA==1;AP==2
  return myWiFiAPSTAvalue;
}

void wifiBroadcastIP(const String myAPName)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    String nameIP=myAPName+" IP:"+WiFi.localIP().toString();
    udp.broadcastTo(nameIP.c_str(), 2255);                            //广播到2255端口
  }
}