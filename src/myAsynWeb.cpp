/*
  本文件中的SPIFFS文件系统或LittleFS文件系统中的文件管理、文件上传等可参考如下链接，有源代码
  https://myhomethings.eu/en/esp32-web-updater-and-spiffs-manager/

  这个网页的原型参考是：
  https://github.com/fairecasoimeme/ZiGate-Ethernet
*/

#include <Arduino.h>
#include <string>
#include "FS.h"
#include "LittleFS.h"
#include <SPIFFS.h>      //build-in，没有用
#include <ESPmDNS.h>     //build-in
#include "esp_adc_cal.h" //build-in
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#include <ArduinoJson.h>
#include <ETH.h>
#include "WiFi.h"
#include <Update.h>

// #include "myLog.h"
#include "myAsynWeb.h"
#include "mySystem.h"
// #include "myFileDirectory.h"
// #include "myWifi.h"
//#include "my4GFunction.h"

#include "mySystem.h"

String myVersion;
String mybpWSSID, mybpWPASS;

typedef struct
{
  String filename;
  String ftype;
  String fsize;
} fileinfo;

String MessageLine;
fileinfo Filenames[100]; // 不超过100个文件数量
bool StartupErrors = false;
int start, downloadtime = 1, uploadtime = 1, downloadsize, uploadsize, downloadrate, uploadrate, numfiles;

AsyncWebServer serverWeb(80);

std::string ReadFileToString(const char *filename)
{
  auto file = LittleFS.open(filename, "r");
  size_t filesize = file.size();
  // Read into temporary Arduino String
  String data = file.readString();
  // Don't forget to clean up!
  file.close();
  return std::string(data.c_str(), data.length());
}

void webServerHandleClient()
{
  // serverWeb.handleClient();
}

// web server的头，主要是前面的图片和版本号控制
const char HTTP_HEADER1[] PROGMEM =
    "<head>"
    "<meta charset='UTF-8'>" // 中文编码
    "<script type='text/javascript' src='web/js/jquery-min.js'></script>"
    "<script type='text/javascript' src='web/js/bootstrap.min.js'></script>"
    "<script type='text/javascript' src='web/js/functions.js'></script>"
    "<link href='web/css/bootstrap.min.css' rel='stylesheet' type='text/css' />"
    "<link href='web/css/style.css' rel='stylesheet' type='text/css' />"
    "</head>"
    "<body>"
    "<nav class='navbar navbar-expand-lg navbar-light bg-light rounded'><a class='navbar-brand' href='/'><img src='web/img/logo.png'/> <strong>Charing Gateway </strong>";

// 这里主要是菜单，可以通过注释的方式打开相关菜单和关闭相关菜单
const char HTTP_HEADER2[] PROGMEM =
    "</a>"
    "<button class='navbar-toggler' type='button' data-toggle='collapse' data-target='#navbarNavDropdown' aria-controls='navbarNavDropdown' aria-expanded='false' aria-label='Toggle navigation'>"
    "<span class='navbar-toggler-icon'></span>"
    "</button>"

    // 一级菜单，二级菜单可在随后的代码中进行注释和取消注释

    // 一级菜单Status
    "<div id='navbarNavDropdown' class='collapse navbar-collapse justify-content-md-center'>"
    "<ul class='navbar-nav'>"
    "<li class='nav-item'>"
    "<a class='nav-link' href='/'>Status</a>"
    "</li>"

    // 一级菜单config，对应的二级菜单可在随后的代码中进行注释和取消注释
    "<li class='nav-item dropdown'>"
    "<a class='nav-link dropdown-toggle' href='#' id='navbarDropdown' role='button' data-toggle='dropdown' aria-haspopup='true' aria-expanded='false'>Config</a>"
    "<div class='dropdown-menu' aria-labelledby='navbarDropdown'>"
    //"<a class='dropdown-item' href='/general'>General</a>"
    //"<a class='dropdown-item' href='/general'>General</a>"                  //首页
    //"<a class='dropdown-item' href='/serial'>Modbus</a>"                    //串口设置
    //"<a class='dropdown-item' href='/ethernet'>Ethernet</a>"                //以太网
    //"<a class='dropdown-item' href='/module'>4G</a>"                        //4G模块
    "<a class='dropdown-item' href='/wifi'>WiFi</a>"       // Wifi模块
    "<a class='dropdown-item' href='/IOT'>IOT setting</a>" // Wifi模块
    //"<a class='dropdown-item' href='/mqtt'>MQTT</a>"                        //mqtt
    //"<a class='dropdown-item' href='/meter'>-Smart Meter dl/t645</a>"       //电表
    //"<a class='dropdown-item' href='/huawei'>-Huawei inverter modbus</a>"   //华为逆变器
    "<a class='dropdown-item' href='/fsbrowser'>Config Content</a>" // 配置文件内容查看
    "</div>"
    "</li>"

    //"<li class='nav-item'>"
    // "<a class='nav-link' href='/tools'>Tools</a>"

    // 一级菜单File
    //  "<li class='nav-item dropdown'>"
    //    "<a class='nav-link dropdown-toggle' href='#' id='navbarDropdown1' role='button' data-toggle='dropdown' aria-haspopup='true' aria-expanded='false'>File</a>"
    //    "<div class='dropdown-menu' aria-labelledby='navbarDropdown1'>"
    //      "<a class='dropdown-item' href='/dir'>Directory</a></a>"
    //      "<a class='dropdown-item' href='/upload'>Upload file</a>"
    //      //"<a class='dropdown-item' href='/download'>Download file</a>"
    //      "<a class='dropdown-item' href='/stream'>Stream file</a>"
    //      //"<a class='dropdown-item' href='/delete'>Delete file</a>"
    //      "<a class='dropdown-item' href='/rename'>Rename file</a>"
    //    "</div>"
    //  "</li>"

    // 一级菜单TOOLS
    //  "<li class='nav-item dropdown'>"
    //    "<a class='nav-link dropdown-toggle' href='#' id='navbarDropdown2' role='button' data-toggle='dropdown' aria-haspopup='true' aria-expanded='false'>Tools</a>"
    //      "<div class='dropdown-menu' aria-labelledby='navbarDropdown2'>"
    //        "<a class='dropdown-item' href='/logs'>Console</a>"
    //        "<a class='dropdown-item' href='/serialcommand'>Serial Command</a>"
    //      "</div>"
    //  "</li>"

    // 一级菜单
    //"<li class='nav-item'>"
    //"<a class='nav-link' href='/system'>Status</a>"
    //"<a class='nav-link' href='/logout'>Logout</a>"
    //"<a class='nav-link' href='/help'>Help</a>"
    //"</li>"

    // 一级菜单System
    "<li class='nav-item dropdown'>"
    "<a class='nav-link dropdown-toggle' href='#' id='navbarDropdown3' role='button' data-toggle='dropdown' aria-haspopup='true' aria-expanded='false'>System</a>"
    "<div class='dropdown-menu' aria-labelledby='navbarDropdown3'>"
    "<a class='dropdown-item' href='/system'>Information</a>"
    "<a class='dropdown-item' href='/OTA'>OTA</a>"
   // "<a class='dropdown-item' href='/upload'>Upload file</a>"    //可以upload，但是无法delect和download文件
    "<a class='dropdown-item' href='/reboot'>Reboot</a>"
    "<a class='dropdown-item' href='/logout'>Logout</a>"
    "<a class='dropdown-item' href='/help'>Help</a>"
    "</div>"
    "</li>"

    "</ul></div>"
    "</nav>";

// wifi配置界面的文件系统存储数据
const char HTTP_WIFI[] PROGMEM =
    "<h1>Config WiFi</h1>"
    //"<div class='row justify-content-md-center' >"
    "<div class='row' >"
    "<div class='col-sm-6'><form method='POST' action='saveWifi'>" // 这里指明调用的函数为savewifi页面
    "<div class='form-check'>"

    "<input class='form-check-input' id='wifiEnable' type='checkbox' name='wifiEnable' {{checkedWiFi}}>"
    "<label class='form-check-label' for='wifiEnable'>Enable</label>"
    "</div>"
    "<div class='form-group'>"
    "<label for='ssid'>SSID</label>"
    "<input class='form-control' id='ssid' type='text' name='WIFISSID' value='{{ssid}}'> <a onclick='scanNetwork();' class='btn btn-primary mb-2'>Scan</a><div id='networks'></div>"
    "</div>"

    "<div class='form-group'>"
    "<label for='pass'>Password</label>"
    "<input class='form-control' id='pass' type='password' name='WIFIpassword' value=''>"
    "</div>"

    "<div class='form-group'>"
    "<label for='ip'>@IP</label>"
    "<input class='form-control' id='ip' type='text' name='ipAddress' value='{{ip}}'>"
    "</div>"

    "<div class='form-group'>"
    "<label for='mask'>@Mask</label>"
    "<input class='form-control' id='mask' type='text' name='ipMask' value='{{mask}}'>"
    "</div>"

    "<div class='form-group'>"
    "<label for='gateway'>@Gateway</label>"
    "<input type='text' class='form-control' id='gateway' name='ipGW' value='{{gw}}'>"
    "</div>"

    "<div class='form-group'>"
    "<label for='tcpListenPort'>@tcpListenPort</label>"
    "<input type='text' class='form-control' id='tcpListenPort' name='tcpListenPort' value='{{tcpListenPort}}'>"
    "</div>"

    // "Server Port : <br>{{port}}<br><br>"

    "<button type='submit' class='btn btn-primary mb-2'name='save'>Save</button>"
    "</form>";

// 串口或RS485配置接口的文件系统存储数据，目前是禁止的
const char HTTP_SERIAL[] PROGMEM =
    "<h1>Config Serial</h1>"
    //"<div class='row justify-content-md-center' >"
    "<div class='row' >"
    "<div class='col-sm-6'><form method='POST' action='saveSerial'>"
    "<div class='form-group'>"

    // work mode,这里无法获取变量和参数
    "<table style=\"width:100%\">"
    "<tr>"
    "<td style=\"width:30%;vertical-align: middle\"><label for=\"WorkMode\">RS485 Bus Mode</label></td>"
    "<td style=\"vertical-align: middle\"><label>Master</label><input  type=\"checkbox\" id='myWorkMode' name='myWorkMode' class=\"switch\"><label>Slave</label></td>"
    "</tr>"
    "</table>"

    // SerialBaud
    "<label for='SerialBaud'>Speed</label>"
    "<select class='form-control' id='SerialBaud' name='SerialBaud'>"
    "<option value='600'   {{selected600}}>600</option>"
    "<option value='1200'  {{selected1200}}>1200</option>"
    "<option value='2400'  {{selected2400}}>2400</option>"
    "<option value='4800'  {{selected4800}}>4800</option>"
    "<option value='9600'  {{selected9600}}>9600</option>"
    "<option value='19200' {{selected19200}}>19200</option>"
    "<option value='38400' {{selected38400}}>38400</option>"
    "<option value='57600' {{selected57600}}>57600</option>"
    "<option value='115200' {{selected115200}}>115200</option>"
    "</select>"

    // SerialMode
    "<label for='SerialMode'>Serial Mode</label>"
    "<select class='form-control' id='SerialMode' name='SerialMode'>"
    "<option value=\"0x8000010\" {{selected5N1}}>5N1</option>"
    "<option value=\"0x8000014\" {{selected6N1}}>6N1</option>"
    "<option value=\"0x8000018\" {{selected7N1}}>7N1</option>"
    "<option value=\"0x800001c\" {{selected8N1}}>8N1</option>"
    "<option value=\"0x8000030\" {{selected5N2}}>5N2</option>"
    "<option value=\"0x8000034\" {{selected6N2}}>6N2</option>"
    "<option value=\"0x8000038\" {{selected7N2}}>7N2</option>"
    "<option value=\"0x800003c\" {{selected8N2}}>8N2</option>"
    "<option value=\"0x8000012\" {{selected5E1}}>5E1</option>"
    "<option value=\"0x8000016\" {{selected6E1}}>6E1</option>"
    "<option value=\"0x800001a\" {{selected7E1}}>7E1</option>"
    "<option value=\"0x800001e\" {{selected8E1}}>8E1</option>"
    "<option value=\"0x8000032\" {{selected5E2}}>5E2</option>"
    "<option value=\"0x8000036\" {{selected6E2}}>6E2</option>"
    "<option value=\"0x800003a\" {{selected7E2}}>7E2</option>"
    "<option value=\"0x800003e\" {{selected8E2}}>8E2</option>"
    "<option value=\"0x8000013\" {{selected5O1}}>5O1</option>"
    "<option value=\"0x8000017\" {{selected6O1}}>6O1</option>"
    "<option value=\"0x800001b\" {{selected7O1}}>7O1</option>"
    "<option value=\"0x800001f\" {{selected8O1}}>8O1</option>"
    "<option value=\"0x8000033\" {{selected5O2}}>5O2</option>"
    "<option value=\"0x8000037\" {{selected6O2}}>6O2</option>"
    "<option value=\"0x800003b\" {{selected7O2}}>7O2</option>"
    "<option value=\"0x800003f\" {{selected8O2}}>8O2</option>"
    "</select>"

    "</div>"
    "<br><br>"
    "<button type='submit' class='btn btn-primary mb-2'name='save'>Save</button>"
    "</form>";

// 智能电表的文件系统存储数据
const char HTTP_METER[] PROGMEM =
    "<h1>Config Smart Meter</h1>"
    //"<div class='row justify-content-md-center' >"
    "<div class='row' >"
    "<div class='col-sm-6'><form method='POST' action='saveMeter'>"
    "<div class='form-group'>"

    // work mode,这里无法获取变量和参数
    "<table style=\"width:100%\">"
    "<tr>"
    "<td style=\"width:10%;vertical-align: middle\"><input  type=\"checkbox\" id='myvoltage1' name='myvoltage1' ></td>"
    "<td ><label for=\"myvoltage1\">Voltage</label></td>"
    "</tr>"

    "<tr>"
    "<td style=\"width:10%;vertical-align: middle\"><input  type=\"checkbox\" id='myvoltage2' name='myvoltage2' ></td>"
    "<td ><label for=\"myvoltage2\">Current</label></td>"
    "</tr>"

    "<tr>"
    "<td style=\"width:10%;vertical-align: middle\"><input  type=\"checkbox\" id='myvoltage3' name='myvoltage3' ></td>"
    "<td ><label for=\"myvoltage3\">Power</label></td>"
    "</tr>"

    "<tr>"
    "<td style=\"width:10%;vertical-align: middle\"><input  type=\"checkbox\" id='myvoltage4' name='myvoltage4' ></td>"
    "<td ><label for=\"myvoltage4\">Power factor</label></td>"
    "</tr>"

    "</table>"
    "</div>"
    "<br><br>"
    "<button type='submit' class='btn btn-primary mb-2'name='save'>Save</button>"
    "</form>";

// 帮助界面的文件系统存储数据，已经测试，不需要修改。
const char HTTP_HELP[] PROGMEM =
    "<h1>Help !</h1>"
    "<h3>Shop & description</h3>"
    "You can go to this url :</br>"
    "<a href=\"https://www.bldxny.com\" target='_blank'>Shop </a></br>"
    //"<a href=\"https://zigate.fr/documentation/descriptif-de-la-zigate-ethernet/\" target='_blank'>Description</a></br>"
    "<h3>Firmware Source & Issues</h3>"
    "Please go here :</br>"
    "<a href=\"https://github.com/bldxny/NetGate\" target='_blank'>Sources</a>";

// MQTT界面的文件系统存储数据，目前需要修改，用于适配物美iot平台，2023-8-2
const char HTTP_MQTT[] PROGMEM =
    "<h1>Config MQTT</h1>"

    // "<div class='row justify-content-md-center' >"
    // "<div class='col-sm-6'><form method='POST' action='saveMQTT'>"
    // "<div class='form-group'>"

    //"<div class='row justify-content-md-center'>"
    "<div class='row' >"
    "<div class='col-sm-6'> <form method='POST' action='saveMQTTCredentials'>"
    "<div class='card'>"
    "<div class='card-header'>MQTT Credentials</div>"
    "<div class='card-body'>"
    "<div id='MQTTCredentialsConfig'>"

    "<table style=\"width:100%\">"
    //"<tr>"
    //"<th>Parameter</th>"
    //"<th>Content</th>"
    //"</tr>"
    "<tr> "
    "<td>MQTT_Host:</td> "
    "<td><input maxlength=\"20\" name=\"MQTTHost1\"></td>"
    "</tr>"

    "<tr>"
    "<td>MQTT_Port:</td>"
    "<td><input maxlength=\"20\" name=\"MQTTPort1\"></td>"
    "</tr>"

    "<tr>"
    "<td>MQTT_User:</td>"
    "<td><input maxlength=\"20\" name=\"MQTTUser1\"></td>"
    "</tr>"
    "<tr>"
    "<td>MQTT_Password:</td>"
    "<td><input maxlength=\"20\" name=\"MQTTPassword1\"></td>"
    "</tr>"

    "</table>"

    "</div>"
    "</div>"
    "</div>"
    "</div>"
    "</div>"

    //"<div class='row justify-content-md-center'>"
    "<div class='row' >"
    "<div class='col-sm-6'> <form method='POST' action='saveMQTTsubscribe'>"
    "<div class='card'>"
    "<div class='card-header'>MQTT subscribe Points</div>"
    "<div class='card-body'>"
    "<div id='MQTTSubPointConfig'>"
    "<table>"

    "<tr>"
    "<th></th>"
    "<th>Topic</th>"
    "<th>Value</th>"
    "</tr>"

    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqsubcheck1\" name=\"mqsubcheck1\" value=\"mqsubcheck1\">  </td>"
    "<td><input maxlength=\"10\" name=\"mqsubTopic1\"></td> "
    "<td><input maxlength=\"10\" name=\"mqsubValue1\"></td> "
    "</tr>"
    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqsubcheck2\" name=\"mqsubcheck2\" value=\"mqsubcheck2\">  </td>"
    "<td><input maxlength=\"10\" name=\"mqsubTopic2\"></td> "
    "<td><input maxlength=\"10\" name=\"mqsubValue2\"></td> "
    "</tr>"

    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqsubcheck3\" name=\"mqsubcheck3\" value=\"mqsubcheck3\">  </td>"
    "<td><input maxlength=\"10\" name=\"mqsubTopic3\"></td> "
    "<td><input maxlength=\"10\" name=\"mqsubValue3\"></td> "
    "</tr>"

    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqcheck4\" name=\"mqcheck4\" value=\"mqcheck4\">  </td>"
    "<td><input maxlength=\"10\" name=\"mqTopic4\"></td> "
    "<td><input maxlength=\"10\" name=\"mqValue4\"></td> "
    "</tr>"

    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqsubcheck5\" name=\"mqsubcheck5\" value=\"mqsubcheck5\">  </td>"
    "<td><input maxlength=\"10\" name=\"mqsubTopic5\"></td> "
    "<td><input maxlength=\"10\" name=\"mqsubValue5\"></td> "

    "</tr>"
    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqsubcheck6\" name=\"mqsubcheck6\" value=\"mqsubcheck6\">  </td>"
    "<td><input maxlength=\"10\" name=\"mqsubTopic6\"></td> "
    "<td><input maxlength=\"10\" name=\"mqsubValue6\"></td> "
    "</tr>"

    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqsubcheck7\" name=\"mqsubcheck7\" value=\"mqsubcheck7\">  </td>"
    "<td><input maxlength=\"10\" name=\"mqsubTopic7\"></td> "
    "<td><input maxlength=\"10\" name=\"mqsubValue7\"></td> "
    "</tr>"

    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqsubcheck8\" name=\"mqsubcheck8\" value=\"mqsubcheck8\">  </td>"
    "<td><input maxlength=\"10\" name=\"mqsubTopic8\"></td> "
    "<td><input maxlength=\"10\" name=\"mqsubValue8\"></td> "
    "</tr>"
    "</table>"
    "</div>"
    "</div>"
    "</div>"
    "</div>"
    "</div>"

    //"<div class='row justify-content-md-center'>"
    "<div class='row' >"
    "<div class='col-sm-6'> <form method='POST' action='saveMQTTPublishs'>"
    "<div class='card'>"
    "<div class='card-header'>MQTT Publish Point</div>"
    "<div class='card-body'>"
    "<div id='MQTTPubPointConfig'>"
    "<table>"
    "<tr>"
    "<th></th>"
    "<th>Topic</th>"
    "<th>Value</th>"
    "<th>Time interval</th>"
    "</tr>"
    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqpubcheck1\" name=\"mqpubcheck1\" value=\"mqpubcheck1\">  </td>"
    "<td ><input style=\"width:150px\" name=\"mqpubTopic1\"></td> "
    "<td ><input style=\"width:150px\" name=\"mqpubValue1\"></td> "
    "<td ><input style=\"width:100px\" name=\"mqpubInter1\"></td>"
    "</tr>"
    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqpubcheck2\" name=\"mqpubcheck2\" value=\"mqpubcheck2\">  </td>"
    "<td ><input style=\"width:150px\" name=\"mqpubTopic2\"></td> "
    "<td ><input style=\"width:150px\" name=\"mqpubValue2\"></td> "
    "<td ><input style=\"width:100px\" name=\"mqpubInter2\"></td>"
    "</tr>"
    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqpubcheck3\" name=\"mqpubcheck3\" value=\"mqpubcheck3\">  </td>"
    "<td ><input style=\"width:150px\" name=\"mqpubTopic3\"></td> "
    "<td ><input style=\"width:150px\" name=\"mqpubValue3\"></td> "
    "<td ><input style=\"width:100px\" name=\"mqpubInter3\"></td>"
    "</tr>"
    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqpubcheck4\" name=\"mqpubcheck4\" value=\"mqpubcheck4\">  </td>"
    "<td ><input style=\"width:150px\" name=\"mqpubTopic4\"></td> "
    "<td ><input style=\"width:150px\" name=\"mqpubValue4\"></td> "
    "<td ><input style=\"width:100px\" name=\"mqpubInter4\"></td>"
    "</tr>"
    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqpubcheck5\" name=\"mqpubcheck5\" value=\"mqpubcheck5\">  </td>"
    "<td ><input style=\"width:150px\" name=\"mqpubTopic5\"></td> "
    "<td ><input style=\"width:150px\" name=\"mqpubValue5\"></td> "
    "<td ><input style=\"width:100px\" name=\"mqpubInter5\"></td>"
    "</tr>"
    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqpubcheck6\" name=\"mqpubcheck6\" value=\"mqpubcheck6\">  </td>"
    "<td ><input style=\"width:150px\" name=\"mqpubTopic6\"></td> "
    "<td ><input style=\"width:150px\" name=\"mqpubValue6\"></td> "
    "<td ><input style=\"width:100px\" name=\"mqpubInter6\"></td>"
    "</tr>"
    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqpubcheck7\" name=\"mqpubcheck7\" value=\"mqpubcheck7\">  </td>"
    "<td ><input style=\"width:150px\" name=\"mqpubTopic7\"></td> "
    "<td ><input style=\"width:150px\" name=\"mqpubValue7\"></td> "
    "<td ><input style=\"width:100px\" name=\"mqpubInter7\"></td>"
    "</tr>"
    "<tr> "
    "<td> <input type=\"checkbox\" id=\"mqpubcheck8\" name=\"mqpubcheck8\" value=\"mqpubcheck8\">  </td>"
    "<td ><input style=\"width:150px\" name=\"mqpubTopic8\"></td> "
    "<td ><input style=\"width:150px\" name=\"mqpubValue8\"></td> "
    "<td ><input style=\"width:100px\" name=\"mqpubInter8\"></td>"
    "</tr>"
    "</table>"

    "</div>"
    "</div>"
    "</div>"
    "</div>"
    "</div>"

    "</div>";

// 有线以太网的文件系统存储数据
const char HTTP_ETHERNET[] PROGMEM =
    "<h1>Config Ethernet</h1>"
    //"<div class='row justify-content-md-center' >"
    "<div class='row' >"
    "<div class='col-sm-6'><form method='POST' action='saveEther'>"
    "<div class='form-check'>"

    "<input class='form-check-input' id='dhcp' type='checkbox' name='dhcp' {{modeEther}}>"
    "<label class='form-check-label' for='dhcp'>DHCP</label>"
    "</div>"
    "<div class='form-group'>"
    "<label for='ip'>@IP</label>"
    "<input class='form-control' id='ip' type='text' name='ipAddress' value='{{ipEther}}'>"
    "</div>"
    "<div class='form-group'>"
    "<label for='mask'>@Mask</label>"
    "<input class='form-control' id='mask' type='text' name='ipMask' value='{{maskEther}}'>"
    "</div>"
    "<div class='form-group'>"
    "<label for='gateway'>@Gateway</label>"
    "<input type='text' class='form-control' id='gateway' name='ipGW' value='{{GWEther}}'>"
    "</div>"
    "Server Port : <br>{{port}}<br><br>"
    "<button type='submit' class='btn btn-primary mb-2'name='save'>Save</button>"
    "</form>";

// 无线4G模块的文件系统存储数据
const char HTTP_MODULE[] PROGMEM =
    "<h1>Config 4G Module</h1>"
    //"<div class='row justify-content-md-center' >"
    "<div class='row' >"
    "<div class='col-sm-6'><form method='POST' action='saveModule'>"
    "<div class='form-check'>"

    "<input class='form-check-input' id='dhcp' type='checkbox' name='dhcp' {{modeEther}}>"
    "<label class='form-check-label' for='dhcp'>DHCP</label>"
    "</div>"
    "<div class='form-group'>"
    "<label for='ip'>@IP</label>"
    "<input class='form-control' id='ip' type='text' name='ipAddress' value='{{ipEther}}'>"
    "</div>"
    "<div class='form-group'>"
    "<label for='mask'>@Mask</label>"
    "<input class='form-control' id='mask' type='text' name='ipMask' value='{{maskEther}}'>"
    "</div>"
    "<div class='form-group'>"
    "<label for='gateway'>@Gateway</label>"
    "<input type='text' class='form-control' id='gateway' name='ipGW' value='{{GWEther}}'>"
    "</div>"
    "Server Port : <br>{{port}}<br><br>"
    "<button type='submit' class='btn btn-primary mb-2'name='save'>Save</button>"
    "</form>";

// 通用部分的HTTP数据
const char HTTP_GENERAL[] PROGMEM =
    "<h1>General</h1>"
    //"<div class='row justify-content-md-center' >"
    "<div class='row ' >"
    "<div class='col-sm-6'><form method='POST' action='saveGeneral'>"
    "<div class='form-check'>"
    "<input class='form-check-input' id='disableWeb' type='checkbox' name='disableWeb' {{disableWeb}}>"
    "<label class='form-check-label' for='disableWeb'>Disable web server when Board is connected</label>"
    "<br>"

    "<input class='form-check-input' id='enableHeartBeat' type='checkbox' name='enableHeartBeat' {{enableHeartBeat}}>"
    "<label class='form-check-label' for='enableHeartBeat'>Enable HeartBeat (send ping to TCP when no trafic)</label>"
    "<br>"

    "<label for='refreshLogs'>Refresh console log</label>"
    "<input class='form-control' id='refreshLogs' type='text' name='refreshLogs' value='{{refreshLogs}}'>"
    "<br>"

    // 这里是modbus的数据
    // "<label for='modbusnumber'>Modbus Points</label>"
    // "<input class='form-control' id='modbusnumber' type='text' name='modbusnumber' value='{{modbusnumber}}'>"
    // "<br>"
    "</div>"

    "<button type='submit' class='btn btn-primary mb-2'name='save'>Save</button>"
    "</form></div>"
    "</div>";

// status菜单栏，当点击Status后，下面的显示内容
const char HTTP_ROOT[] PROGMEM =
    "<h1>Status</h1>"

    "<div class='row'>"
    "<div class='col-sm-6'>"
    "<div class='card'>"
    "<div class='card-header'>Blockchain</div>"
    "<div class='card-body'>"
    "<div id='blockchainsConfig'>"
    "<strong>ChipID : </strong>{{ChipID}}"
    //"<br><strong>Entropy : </strong>{{Entropy}}"
    "<br><strong>Pubkey: </strong>{{Publickey}}"
    "<br><strong>Address: </strong>{{Address}}"
    "</div>"
    "</div>"
    "</div>"
    "</div>"
    "</div>"

    "<div class='row'>"
    "<div class='col-sm-6'>"
    "<div class='card'>"
    "<div class='card-header'></div>"
    "<div class='card-body'>"
    "<div id='4G & Blockchain'>"
    "<strong>RSSI : </strong>{{RSSI}}"
    "<br><strong>NetworkStatus : </strong>{{NetworkStatus}}"
    "<br><strong>OnBlockchainData : </strong>{{OnBlockchainData}}"
    "<br><strong>Send Platform : </strong>{{BlockchainResult}}"
    // "<br><strong>@Mask : </strong>{{maskEther}}"
    // "<br><strong>@GW : </strong>{{GWEther}}"
    "</div>"
    "</div>"
    "</div>"
    "</div>"
    "</div>"

    "<div class='row'>"
    "<div class='col-sm-6'>"
    "<div class='card'>"
    "<div class='card-header'>Wifi</div>"
    "<div class='card-body'>"
    "<div id='wifiConfig'>"
    "<strong>Enable : </strong>{{enableWifi}}"
    "<br><strong>WIFI SSID : </strong>{{ssidWifi}}"
    "<br><strong>WIFI IP : </strong>{{ipWifi}}"
    "<br><strong>WIFI MAC : </strong>{{macWifi}}"
    "<br><strong>WIFI RSSI : </strong>{{RSSIWifi}}"
    "</div>"
    "</div>"
    "</div>"
    "</div>"
    "</div>";

void handleNotFound(AsyncWebServerRequest *request)
{
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += request->url();
  message += F("\nMethod: ");
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += request->args();
  message += F("\n");

  for (uint8_t i = 0; i < request->args(); i++)
  {
    message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
  }

  request->send(404, F("text/plain"), message);
}

// 打开help网页的时候展示的页面内容
void handleHelp(AsyncWebServerRequest *request)
{
  String result;
  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += FPSTR(HTTP_HELP);

  result += F("</html>");

  request->send(200, "text/html", result);
}

// 打开mqtt后页面展示的内容
void handlemqtt(AsyncWebServerRequest *request)
{
  String result;
  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += FPSTR(HTTP_MQTT);
  result += F("</html>");

  request->send(200, "text/html", result);
}

void handleSavemqtt(AsyncWebServerRequest *request)
{
}

void handlehuawei(AsyncWebServerRequest *request)
{
}

void handleSavehuawei(AsyncWebServerRequest *request)
{
}

// 智能电表
void handlemeter(AsyncWebServerRequest *request)
{
  String result;
  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += FPSTR(HTTP_METER);

  result += F("</html>");
  request->send(200, "text/html", result);
}

void handleSavemeter(AsyncWebServerRequest *request)
{
}

// 这个测试后发现没问题，这个函数可兼顾测试出html格式和string格式
// Utility function to create indentation
String createIndent(uint8_t depth)
{
  String indent = "";
  for (uint8_t i = 0; i < depth; i++)
  {
    indent += "&nbsp;&nbsp;&nbsp;&nbsp;"; // 4 spaces for HTML
  }
  return indent;
}

void listDir2(fs::FS &fs, const char *dirname, uint8_t levels, bool ishtml, uint8_t depth)
{
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  String indent = createIndent(depth);

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      if (ishtml)
      {
        bpinfo.webDirTable += "<tr align='left'><td>" + indent + String(file.name()) + "/</td><td>[DIR]</td><td></td></tr>\n";
      }
      else
      {
        bpinfo.webDirTable += indent + "DIR: " + String(file.name()) + "\n";
      }
      if (levels)
      {
        listDir2(fs, file.path(), levels - 1, ishtml, depth + 1); // Increment depth for indentation
      }
    }
    else
    {
      if (ishtml)
      {
        bpinfo.webDirTable += "<tr align='left'><td>" + indent + String(file.name()) + "</td><td>" + String(file.size()) + "</td>";
        bpinfo.webDirTable += "<td><button class='directory_buttons' onclick=\"directory_button_handler(\'" + String(file.name()) + "\', 'download')\">Download</button></td>";
        bpinfo.webDirTable += "<td><button class='directory_buttons' onclick=\"directory_button_handler(\'" + String(file.name()) + "\', 'delete')\">Delete</button></td></tr>\n";
      }
      else
      {
        bpinfo.webDirTable += indent + "FILE: " + String(file.name()) + " SIZE: " + String(file.size()) + "\n";
      }
    }
    file = root.openNextFile();
  }
}

// 展示文件夹数据信息
void handlemydir(AsyncWebServerRequest *request)
{
  // 需要注意清零
  bpinfo.webDirTable = "";
  bpinfo.webDirTable += "<table align='center'>\n<tr>\n<th align='left'>Name</th><th align='left'>Size</th><th></th><th></th>\n</tr>\n";

  listDir2(LittleFS, "/", 2, true, 2);

  bpinfo.webDirTable += "</table> \n";

  String result;
  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += bpinfo.webDirTable;

  result += F("</html>");
  request->send(200, "text/html", result);
}

void handleGeneral(AsyncWebServerRequest *request)
{
  String result;
  result += F("<html>");

  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += FPSTR(HTTP_GENERAL);
  result += F("</html>");

  // if (bpinfo.disableWeb)
  // {
  //   result.replace("{{disableWeb}}", "checked");
  // }
  // else
  // {
  //   result.replace("{{disableWeb}}", "");
  // }

  // if (bpinfo.enableHeartBeat)
  // {
  //   result.replace("{{enableHeartBeat}}", "checked");
  // }
  // else
  // {
  //   result.replace("{{enableHeartBeat}}", "");
  // }

  // result.replace("{{refreshLogs}}", (String)bpinfo.refreshLogs);

  request->send(200, "text/html", result);
}

// 只是看看相关数据
void handleSystem(AsyncWebServerRequest *request)
{
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  String result;
  result += F("<html>");

  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  // 需要将数据放到flash中，
  result += "<h1>System Information</h1>";
  // result += "<div class='row justify-content-md-center' >";
  result += "<div class='row' >";
  result += "<div class='col-sm-6'>";

  result += "<h4>data transfer</h4>";
  result += "<table class='center'>";
  result += "<tr><th>Last Upload</th><th>Last Download/Stream</th><th>Units</th></tr>";
  result += "<tr><td>" + ConvBinUnits(uploadsize, 1) + "</td><td>" + ConvBinUnits(downloadsize, 1) + "</td><td>File Size</td></tr> ";
  result += "<tr><td>" + ConvBinUnits((float)uploadsize / uploadtime * 1024.0, 1) + "/Sec</td>";
  result += "<td>" + ConvBinUnits((float)downloadsize / downloadtime * 1024.0, 1) + "/Sec</td><td>Transfer Rate</td></tr>";
  result += "</table>";
  result += "<h4>Filing System</h4>";
  result += "<table class='center'>";
  result += "<tr><th>Total Space</th><th>Used Space</th><th>Free Space</th><th>Number of Files</th></tr>";
  result += "<tr><td>" + ConvBinUnits(LittleFS.totalBytes(), 1) + "</td>";
  result += "<td>" + ConvBinUnits(LittleFS.usedBytes(), 1) + "</td>";
  result += "<td>" + ConvBinUnits(LittleFS.totalBytes() - LittleFS.usedBytes(), 1) + "</td>";
  result += "<td>" + (numfiles == 0 ? "Pending Dir or Empty" : String(numfiles)) + "</td></tr>";
  result += "</table>";
  result += "<h4>CPU Information</h4>";
  result += "<table class='center'>";
  result += "<tr><th>Parameter</th><th>Value</th></tr>";
  result += "<tr><td>Number of Cores</td><td>" + String(chip_info.cores) + "</td></tr>";
  result += "<tr><td>Chip revision</td><td>" + String(chip_info.revision) + "</td></tr>";
  result += "<tr><td>Internal or External Flash Memory</td><td>" + String(((chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "Embedded" : "External")) + "</td></tr>";
  result += "<tr><td>Flash Memory Size</td><td>" + String((spi_flash_get_chip_size() / (1024 * 1024))) + " MB</td></tr>";
  result += "<tr><td>Current Free RAM</td><td>" + ConvBinUnits(ESP.getFreeHeap(), 1) + "</td></tr>";
  result += "</table>";

  // result += "<h4>Network Information</h4>";    //挪到status里面
  // result += "<table class='center'>";
  // result += "<tr><th>Parameter</th><th>Value</th></tr>";
  // result += "<tr><td>LAN IP Address</td><td>" + String(WiFi.localIP().toString()) + "</td></tr>";
  // result += "<tr><td>Network Adapter MAC Address</td><td>" + String(WiFi.BSSIDstr()) + "</td></tr>";
  // result += "<tr><td>WiFi SSID</td><td>" + String(WiFi.SSID()) + "</td></tr>";
  // result += "<tr><td>WiFi RSSI</td><td>" + String(WiFi.RSSI()) + " dB</td></tr>";
  // result += "<tr><td>WiFi Channel</td><td>" + String(WiFi.channel()) + "</td></tr>";
  // result += "<tr><td>WiFi Encryption Type</td><td>" + String(EncryptionType(WiFi.encryptionType(0))) + "</td></tr>";
  // result += "</table> ";

  result += F("</html>");

  request->send(200, "text/html", result);
}

bool readJsonConfigFromFS(const char *path, DynamicJsonDocument &jsonConfig)
{
  // if (LittleFS.begin()) {
  // if (LittleFS.exists(path)) {
  File configFile = LittleFS.open(path, "r");
  if (configFile)
  {
    DeserializationError error = deserializeJson(jsonConfig, configFile);
    configFile.close();
    // LittleFS.end();

    if (!error)
    {
      return true;
    }
  }
  // }
  // }
  return false;
}

void handleWifi(AsyncWebServerRequest *request)
{
  String result;
  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += FPSTR(HTTP_WIFI);
  result += F("</html>");

  // Read and parse config from /config/config.json
  DynamicJsonDocument jsonConfig(1024); // Adjust the size as needed

  if (readJsonConfigFromFS("/config/config.json", jsonConfig))
  {
    // Fill in the HTML template with config values
    if (jsonConfig.containsKey("enableWiFi"))
    {
      if (jsonConfig["enableWiFi"].as<int>() == 1)
      {
        result.replace("{{checkedWiFi}}", "Checked");
      }
      else
      {
        result.replace("{{checkedWiFi}}", "");
      }
    }

    if (jsonConfig.containsKey("ssid"))
    {
      result.replace("{{ssid}}", jsonConfig["ssid"].as<String>());
    }

    if (jsonConfig.containsKey("pass"))
    {
      result.replace("{{pass}}", jsonConfig["pass"].as<String>());
    }

    if (jsonConfig.containsKey("ip"))
    {
      result.replace("{{ip}}", jsonConfig["ip"].as<String>());
    }

    if (jsonConfig.containsKey("mask"))
    {
      result.replace("{{mask}}", jsonConfig["mask"].as<String>());
    }

    if (jsonConfig.containsKey("gw"))
    {
      result.replace("{{gw}}", jsonConfig["gw"].as<String>());
    }

    if (jsonConfig.containsKey("tcpListenPort"))
    {
      result.replace("{{tcpListenPort}}", jsonConfig["tcpListenPort"].as<String>());
    }

    request->send(200, "text/html", result);
  }
  else
  {
    // Failed to read config
    request->send(500, "text/html", "Failed to read config.");
  }
}

// 串口页面展示
void handleSerial(AsyncWebServerRequest *request)
{
  String result;
  // F宏和FPSTR宏
  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += FPSTR(HTTP_SERIAL);
  result += F("</html>");
  // DEBUG_PRINTLN(result);
  DEBUG_PRINTLN("-----------------------------");
  // DEBUG_PRINTLN(bpinfo.serialSpeed);

  // if (bpinfo.serialSpeed == 600)
  // {
  //   result.replace("{{selected600}}", "Selected");
  // }
  // else if (bpinfo.serialSpeed == 1200)
  // {
  //   result.replace("{{selected1200}}", "Selected");
  // }
  // else if (bpinfo.serialSpeed == 2400)
  // {
  //   result.replace("{{selected2400}}", "Selected");
  // }
  // else if (bpinfo.serialSpeed == 4800)
  // {
  //   result.replace("{{selected4800}}", "Selected");
  // }
  // else if (bpinfo.serialSpeed == 9600)
  // {
  //   result.replace("{{selected9600}}", "Selected");
  // }
  // else if (bpinfo.serialSpeed == 19200)
  // {
  //   result.replace("{{selected19200}}", "Selected");
  // }
  // else if (bpinfo.serialSpeed == 38400)
  // {
  //   result.replace("{{selected38400}}", "Selected");
  // }
  // else if (bpinfo.serialSpeed == 57600)
  // {
  //   result.replace("{{selected57600}}", "Selected");
  // }
  // else if (bpinfo.serialSpeed == 115200)
  // {
  //   result.replace("{{selected115200}}", "Selected");
  // }
  // else
  // {
  //   result.replace("{{selected115200}}", "Selected");
  // }

  // if (bpinfo.SerialWorkMode == "master")
  // {
  //   result.replace("{{selected600}}", "on");
  // }
  // else if (bpinfo.SerialWorkMode == "slave")
  // {
  //   result.replace("{{selected1200}}", "off");
  // }
  // else
  // {
  //   result.replace("{{selected2400}}", "off");
  // }

  // if (bpinfo.SerialMode == "0x8000010")
  // {
  //   result.replace("{{selected5N1}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x8000014")
  // {
  //   result.replace("{{selected6N1}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x8000018")
  // {
  //   result.replace("{{selected7N1}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x800001c")
  // {
  //   result.replace("{{selected8N1}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x8000030")
  // {
  //   result.replace("{{selected5N2}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x8000034")
  // {
  //   result.replace("{{selected6N2}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x8000038")
  // {
  //   result.replace("{{selected7N2}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x800003c")
  // {
  //   result.replace("{{selected8N2}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x8000012")
  // {
  //   result.replace("{{selected5E1}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x8000016")
  // {
  //   result.replace("{{selected6E1}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x800001a")
  // {
  //   result.replace("{{selected7E1}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x800001e")
  // {
  //   result.replace("{{selected8E1}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x8000032")
  // {
  //   result.replace("{{selected5E2}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x8000036")
  // {
  //   result.replace("{{selected6E2}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x800003a")
  // {
  //   result.replace("{{selected7E2}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x800003e")
  // {
  //   result.replace("{{selected8E2}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x8000013")
  // {
  //   result.replace("{{selected5O1}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x8000017")
  // {
  //   result.replace("{{selected6O1}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x800001b")
  // {
  //   result.replace("{{selected7O1}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x800001f")
  // {
  //   result.replace("{{selected8O1}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x8000033")
  // {
  //   result.replace("{{selected5O2}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x8000037")
  // {
  //   result.replace("{{selected6O2}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x800003b")
  // {
  //   result.replace("{{selected7O2}}", "Selected");
  // }
  // else if (bpinfo.SerialMode == "0x800003f")
  // {
  //   result.replace("{{selected8O2}}", "Selected");
  // }
  // else
  // {
  //   result.replace("{{selected8N1}}", "Selected");
  // }

  // DEBUG_PRINTLN("###############################");
  // DEBUG_PRINTLN(result);

  request->send(200, "text/html", result);
}

// 有线网络设置
void handleEther(AsyncWebServerRequest *request)
{
  String result;
  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += FPSTR(HTTP_ETHERNET);
  result += F("</html>");

  // if (bpinfo.dhcp)
  // {
  //   result.replace("{{modeEther}}", "Checked");
  // }
  // else
  // {
  //   result.replace("{{modeEther}}", "");
  // }
  // result.replace("{{ipEther}}", bpinfo.ipAddress);
  // result.replace("{{maskEther}}", bpinfo.ipMask);
  // result.replace("{{GWEther}}", bpinfo.ipGW);
  // result.replace("{{port}}", String(bpinfo.tcpListenPort));

  request->send(200, "text/html", result);
}

// 有线网络设置保存
void handleSaveEther(AsyncWebServerRequest *request)
{
  if (!request->hasArg("ipAddress"))
  {
    request->send(500, "text/plain", "BAD ARGS");
    return;
  }

  String StringConfig;
  String dhcp;
  if (request->arg("dhcp") == "on")
  {
    dhcp = "1";
  }
  else
  {
    dhcp = "0";
  }
  String ipAddress = request->arg("ipAddress");
  String ipMask = request->arg("ipMask");
  String ipGW = request->arg("ipGW");

  const char *path = "/config/configEther.json";

  StringConfig = "{\"dhcp\":" + dhcp + ",\"ip\":\"" + ipAddress + "\",\"mask\":\"" + ipMask + "\",\"gw\":\"" + ipGW + "\"}";
  DEBUG_PRINTLN(StringConfig);
  StaticJsonDocument<512> jsonBuffer;
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, StringConfig);

  File configFile = LittleFS.open(path, FILE_WRITE);
  if (!configFile)
  {
    DEBUG_PRINTLN(F("failed open"));
  }
  else
  {
    serializeJson(doc, configFile);
  }
  request->send(200, "text/html", "Save config OK ! <br><form method='GET' action='reboot'><input type='submit' name='reboot' value='Reboot'></form>");
}

// 2g或4g模块的设置
void handlemodule(AsyncWebServerRequest *request)
{
  String result;

  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += FPSTR(HTTP_MODULE);
  result += F("</html>");

  // if (bpinfo.dhcp)
  // {
  //   result.replace("{{modeEther}}", "Checked");
  // }
  // else
  // {
  //   result.replace("{{modeEther}}", "");
  // }
  // result.replace("{{ipEther}}", bpinfo.ipAddress);
  // result.replace("{{maskEther}}", bpinfo.ipMask);
  // result.replace("{{GWEther}}", bpinfo.ipGW);
  // result.replace("{{port}}", String(bpinfo.tcpListenPort));

  request->send(200, "text/html", result);
}

// 2g或4g模块保存设置
void handleSavemodule(AsyncWebServerRequest *request) // 目前有错误
{
  DEBUG_PRINTLN("handle Save module ");
}

uint32_t readADC_Cal(int ADC_Raw)
{
  esp_adc_cal_characteristics_t adc_chars;

  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  return (esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}

// status状态页面展示
void handleRoot(AsyncWebServerRequest *request)
{
  String result;
  result += F("<html>");
  // result += FPSTR(HTTP_HEADER);

  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += FPSTR(HTTP_ROOT);

  result += F("</html>");
  // result += "<div class='row justify-content-md-center' >";
  result += "<div class='row' >";
  result += "<div class='col-sm-6'>";

  // result.replace("{{ChipID}}", bpinfo.bpBOARDID);
  // result.replace("{{ENTROPY}}", bpinfo.bpENTROPY);
  // result.replace("{{Address}}", RWBoardParameter("READ", "HDADDR", ""));
  // result.replace("{{Publickey}}", RWBoardParameter("READ", "PUBKEY", ""));

  // result.replace("{{RSSI}}", String(bpinfo.NetworkRSSI));
  // result.replace("{{OnBlockchainData}}", String(bpinfo.OnBlockchainData));
  // result.replace("{{BlockchainResult}}", String(bpinfo.BlockchainResult));
  // result.replace("{{NetworkStatus}}",  String(getNetworkregisterStatus()));

  if (String(WiFi.localIP().toString()) != "") // 通过判断wifi是否获得IP来判断
  {
    result.replace("{{enableWifi}}", "<img src='/web/img/ok.png'>");
  }
  else
  {
    result.replace("{{enableWifi}}", "<img src='/web/img/nok.png'>");
  }
  result.replace("{{ssidWifi}}", String(WiFi.SSID()));
  result.replace("{{ipWifi}}", String(WiFi.localIP().toString()));
  result.replace("{{macWifi}}", String(WiFi.BSSIDstr()));
  result.replace("{{RSSIWifi}}", String(WiFi.RSSI()));

  // if (bpinfo.dhcp)
  // {
  //   result.replace("{{modeEther}}", "DHCP");
  //   result.replace("{{ipEther}}", ETH.localIP().toString());
  //   result.replace("{{maskEther}}", ETH.subnetMask().toString());
  //   result.replace("{{GWEther}}", ETH.gatewayIP().toString());
  // }
  // else
  // {
  //   result.replace("{{modeEther}}", "STATIC");
  //   result.replace("{{ipEther}}", bpinfo.ipAddress);
  //   result.replace("{{maskEther}}", bpinfo.ipMask);
  //   result.replace("{{GWEther}}", bpinfo.ipGW);
  // }
  // if (bpinfo.connectedEther)
  // {
  //   result.replace("{{connectedEther}}", "<img src='/web/img/ok.png'>");
  // }
  // else
  // {
  //   result.replace("{{connectedEther}}", "<img src='/web/img/nok.png'>");
  // }

  request->send(200, "text/html", result);
}

void handleSaveGeneral(AsyncWebServerRequest *request)
{
  String StringConfig;
  String disableWeb;
  String enableHeartBeat;
  String refreshLogs;

  DEBUG_PRINTLN("handle Save General");

  if (request->arg("disableWeb") == "on")
  {
    disableWeb = "1";
  }
  else
  {
    disableWeb = "0";
  }

  if (request->arg("enableHeartBeat") == "on")
  {
    enableHeartBeat = "1";
  }
  else
  {
    enableHeartBeat = "0";
  }

  if (request->arg("refreshLogs").toDouble() < 1000)
  {
    refreshLogs = "1000";
  }
  else
  {
    refreshLogs = request->arg("refreshLogs");
  }

  const char *path = "/config/configGeneral.json";

  StringConfig = "{\"disableWeb\":" + disableWeb + ",\"enableHeartBeat\":" + enableHeartBeat + ",\"refreshLogs\":" + refreshLogs + "}";
  StaticJsonDocument<512> jsonBuffer;
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, StringConfig);

  File configFile = LittleFS.open(path, FILE_WRITE);
  if (!configFile)
  {
    DEBUG_PRINTLN(F("failed open"));
  }
  else
  {
    serializeJson(doc, configFile);
  }
  request->send(200, "text/html", "Save config OK ! <br><form method='GET' action='reboot'><input type='submit' name='reboot' value='Reboot'></form>");
}

// OPENAI帮着优化
const int WIFI_CONNECT_MAX_RETRIES = 10;
const int WIFI_CONNECT_TIMEOUT_MS = 10000;
// 这个功能目前还有点问题，具体就是点击save后，页面半天不出来，同时会导致littlefs崩溃
void handleSaveWifi(AsyncWebServerRequest *request)
{
  String enableWiFi;

  if (!request->hasArg("WIFISSID"))
  {
    request->send(400, "text/plain", "Missing SSID");
    return;
  }

  if (request->arg("wifiEnable") == "on")
  {
    enableWiFi = "1";
  }
  else
  {
    enableWiFi = "0";
  }

  String ssid = request->arg("WIFISSID");
  String pass = request->arg("WIFIpassword");
  String ipAddress = request->arg("ipAddress");
  String ipMask = request->arg("ipMask");
  String ipGW = request->arg("ipGW");
  String tcpListenPort = request->arg("tcpListenPort");

  DEBUG_PRINTLN();
  DEBUG_PRINT("SSID: ");
  DEBUG_PRINTLN(ssid.c_str());
  DEBUG_PRINT("PASS: ");
  DEBUG_PRINTLN(pass.c_str());

  // bpinfo.bpWSSID = RWBoardParameter("WRITE", "WSSID", ssid.c_str(),General);
  // bpinfo.bpWPASS = RWBoardParameter("WRITE", "WPASS", pass.c_str(),General);

  mybpWSSID = RWBoardParameter("WRITE", "WSSID", ssid.c_str(), General);
  mybpWPASS = RWBoardParameter("WRITE", "WPASS", pass.c_str(), General);

  const char *path = "/config/config.json";

  String StringConfig = "{\"enableWiFi\":" + enableWiFi + ",\"ssid\":\"" + ssid + "\",\"pass\":\"" + pass + "\",\"ip\":\"" + ipAddress + "\",\"mask\":\"" + ipMask + "\",\"gw\":\"" + ipGW + "\",\"tcpListenPort\":\"" + tcpListenPort + "\"}";
  DEBUG_PRINTLN(StringConfig);

  // if (!LittleFS.begin()) {
  //   request->send(500, "text/plain", "Failed to mount file system.");
  //   return;  //这里的LittleFS在项目setup就begin了，再begin会出错，并return，后面的代码就不执行了
  // }
  File configFile = LittleFS.open(path, "w");
  if (!configFile)
  {
    LittleFS.end();
    request->send(500, "text/plain", "Failed to open config file.");
    return;
  }
  else
  {
    configFile.print(StringConfig);
    configFile.close();
  }
  // LittleFS.end();
  Serial.println("****----****");

  // Connect to the WiFi network with the new credentials
  // WiFi.disconnect();                                            // 断开已有的WiFi连接
  WiFi.begin(ssid.c_str(), pass.c_str());

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < WIFI_CONNECT_MAX_RETRIES)
  {
    // delay(WIFI_CONNECT_TIMEOUT_MS / WIFI_CONNECT_MAX_RETRIES);
    vTaskDelay((WIFI_CONNECT_TIMEOUT_MS / WIFI_CONNECT_MAX_RETRIES) / portTICK_PERIOD_MS); // 时间延迟和任务切换
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    String ShowipAddress = "Settings saved. New IP address: " + WiFi.localIP().toString();
    request->send(200, "text/html", ShowipAddress);
  }
  else
  {
    request->send(500, "text/html", "Failed to connect to WiFi.");
  }
}

// 串口数据保存
void handleSaveSerial(AsyncWebServerRequest *request)
{
  String StringConfig;

  //   int paramsNr = request->params();
  //     DEBUG_PRINTLN(paramsNr);

  //   for(int i=0;i<paramsNr;i++){

  //      AsyncWebParameter* p = request->getParam(i);

  //      DEBUG_PRINT("Param name: ");
  //      DEBUG_PRINTLN(p->name());

  //      DEBUG_PRINT("Param value: ");
  //      DEBUG_PRINTLN(p->value());

  //      DEBUG_PRINTLN("------");
  // }

  String serialSpeed = request->arg("SerialBaud");    // 网页参数及数据提取
  String serialWorkMode = request->arg("myWorkMode"); // 网页参数及数据提取
  String SerialDataMode = request->arg("SerialMode"); // 网页参数及数据提取

  // DEBUG_PRINT("WorkMode:");
  // DEBUG_PRINTLN(serialWorkMode);

  // DEBUG_PRINT("serial Speed:");
  // DEBUG_PRINTLN(serialSpeed);

  // DEBUG_PRINT("SerialMode:");
  // DEBUG_PRINTLN(SerialDataMode);

  if (serialWorkMode == "on")
    serialWorkMode = "Slave";
  else
    serialWorkMode = "Master";

  const char *path = "/config/configSerial.json";

  // arduinojson不支持16进制，所以通过“”包含起来。
  StringConfig = "{\"Baud\":\"" + serialSpeed + "\",\"WorkMode\":\"" + serialWorkMode + "\",\"SerialMode\":\"" + SerialDataMode + "\"}";
  DEBUG_PRINTLN(StringConfig);
  StaticJsonDocument<512> jsonBuffer;
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, StringConfig);

  File configFile = LittleFS.open(path, FILE_WRITE);
  if (!configFile)
  {
    DEBUG_PRINTLN(F("failed open"));
  }
  else
  {
    serializeJson(doc, configFile);
  }

  request->send(200, "text/html", "Save config OK ! <br><form method='GET' action='reboot'><input type='submit' name='reboot' value='Reboot'></form>");
}

void handleLogs(AsyncWebServerRequest *request)
{
  String result;

  result += F("<html>");
  // result += FPSTR(HTTP_HEADER);

  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += F("<h1>Console</h1>");
  // result += F("<div class='row justify-content-md-center'>");
  result += F("<div class='row'>");
  result += F("<div class='col-sm-6'>");
  result += F("<button type='button' onclick='cmd(\"ClearConsole\");document.getElementById(\"console\").value=\"\";' class='btn btn-primary'>Clear Console</button> ");
  result += F("<button type='button' onclick='cmd(\"GetVersion\");' class='btn btn-primary'>Get Version</button> ");
  result += F("<button type='button' onclick='cmd(\"ErasePDM\");' class='btn btn-primary'>Erase PDM</button> ");
  result += F("</div></div>");
  // result += F("<div class='row justify-content-md-center' >");
  result += F("<div class='row' >");
  result += F("<div class='col-sm-6'>");

  result += F("Raw datas : <textarea id='console' rows='16' cols='100'>");

  result += F("</textarea></div></div>");
  // result += F("</div>");
  result += F("</body>");
  result += F("<script language='javascript'>");
  result += F("logRefresh({{refreshLogs}});");
  result += F("</script>");
  result += F("</html>");

  // result.replace("{{refreshLogs}}", (String)bpinfo.refreshLogs);

  request->send(200, F("text/html"), result);
}

void handleTools(AsyncWebServerRequest *request)
{
  String result;

  result += F("<html>");
  // result += FPSTR(HTTP_HEADER);

  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += F("<h1>Tools</h1>");
  result += F("<div class='btn-group-vertical'>");
  result += F("<a href='/logs' class='btn btn-primary mb-2'>Console</button>");
  result += F("<a href='/fsbrowser' class='btn btn-primary mb-2'>FSbrowser</button>");
  // result += F("<a href='/update' class='btn btn-primary mb-2'>Update</button>");
  result += F("<a href='/reboot' class='btn btn-primary mb-2'>Reboot</button>");
  result += F("</div></body></html>");

  request->send(200, F("text/html"), result);
}

// 重启设备
void handleReboot(AsyncWebServerRequest *request)
{
  String result;

  result += F("<html>");
  // result += FPSTR(HTTP_HEADER);

  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += F("<h1>Reboot ...</h1>");
  result = result + F("</body></html>");
  AsyncWebServerResponse *response = request->beginResponse(303);
  response->addHeader(F("Location"), F("/"));
  request->send(response);

  // 设备重启
  ESP.restart();
}

// 这个页面有问题
void handleUpdate(AsyncWebServerRequest *request)
{
  String result;
  result += F("<html>");
  // result += FPSTR(HTTP_HEADER);
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += F("<h1>Update ...</h1>");
  result += F("<div class='btn-group-vertical'>");
  result += F("<a href='/setchipid' class='btn btn-primary mb-2'>setChipId</button>");
  result += F("<a href='/setmodeprod' class='btn btn-primary mb-2'>setModeProd</button>");
  result += F("</div>");

  result = result + F("</body></html>");

  request->send(200, F("text/html"), result);
}

// 文件浏览器，这里手机版本的chrome和windows版本的edge浏览器均可正常显示，而windows版本的chrome显示乱
void handleFSbrowser(AsyncWebServerRequest *request)
{
  String result;
  String tmpString = "";
  String rtnString = "";
  uint8_t levels;

  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += F("<h1>File Browser</h1>");
  result += F("<nav id='navbar-custom' class='navbar navbar-default navbar-fixed-left'>");
  result += F("      <div class='navbar-header'>");
  result += F("        <!--<a class='navbar-brand' href='#'>Brand</a>-->");
  result += F("      </div>");
  result += F("<ul class='nav navbar-nav'>");

  String str = "";
  // File root = LittleFS.open("/config");
  File root = LittleFS.open("/config");
  if (!root)
  {
    DEBUG_PRINTLN("- failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    DEBUG_PRINTLN(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    String tmp = file.name();
    // DEBUG_PRINTLN(tmp);
    // tmp = tmp.substring(0);                                                      //删除这一句:lyx
    result += F("<li><a href='#' onClick=\"readfile('"); // 读取文件内容
    result += tmp;
    result += F("');\">");
    result += tmp;
    result += F(" ( ");
    result += file.size();
    result += F(" Byte)</a></li>");
    file = root.openNextFile();
  }
  result += F("</ul></nav>");

  result += F("<div class='container-fluid' >");
  result += F("  <div class='app-main-content'>");
  result += F("<form method='POST' action='saveFile'>"); // 保存文件内容
  result += F("<div class='form-group'>");
  result += F(" <label for='file'>File : <span id='title'></span></label>");
  result += F("<input type='hidden' name='filename' id='filename' value=''>");
  result += F(" <textarea class='form-control' id='file' name='file' rows='10'>");
  result += F("</textarea>");
  result += F("</div>");
  result += F("<button type='submit' class='btn btn-primary mb-2'>Save</button>");
  result += F("</Form>");
  result += F("</div>");
  result += F("</div>");
  result += F("</body></html>");

  // DEBUG_PRINTLN(result);

  request->send(200, F("text/html"), result);
}

// 读取文件内容
void handleReadfile(AsyncWebServerRequest *request)
{
  String result;
  uint8_t i = 0;
  String filename = "/config/" + request->arg(i);
  File file = LittleFS.open(filename, "r");

  if (!file)
  {
    return;
  }

  while (file.available())
  {
    result += (char)file.read();
  }
  file.close();

  // DEBUG_PRINTLN(result);
  request->send(200, F("text/html"), result);
}

// 测试通过
void handleSavefile(AsyncWebServerRequest *request)
{
  // DEBUG_PRINTLN("handle Save file");
  if (request->method() != HTTP_POST)
  {
    request->send(405, F("text/plain"), F("Method Not Allowed"));
  }
  else
  {
    uint8_t i = 0;
    String filename = "/config/" + request->arg(i);
    DEBUG_PRINTLN(filename);

    String content = request->arg(1);
    File file = LittleFS.open(filename, "w");
    if (!file)
    {
      DEBUG_PRINT(F("Failed to open file for reading\r\n"));
      return;
    }

    int bytesWritten = file.print(content);

    if (bytesWritten > 0)
    {
      DEBUG_PRINTLN(F("File was written"));
      // DEBUG_PRINTLN(bytesWritten);
    }
    else
    {
      DEBUG_PRINTLN(F("File write failed"));
    }

    file.close();
    AsyncWebServerResponse *response = request->beginResponse(303); // 303？
    response->addHeader(F("Location"), F("/fsbrowser"));
    request->send(response);
  }

  request->send(200, "text/html", "Save config OK ! <br><form method='GET' action='reboot'><input type='submit' name='reboot' value='Reboot'></form>");
}

void handleLogBuffer(AsyncWebServerRequest *request)
{
  // String result;
  // result = logPrint();
  // request->send(200, F("text/html"), result);
}

// WIFI扫描,
// 测试:1.在STA模式下很好工作；
//      2.如果是工作在纯AP模式下，本函数中scanNetworks返回为-2，代表执行错误。
//      3.如果是AP+STA模式下，可是扫描到热点数据。
void handleScanNetwork(AsyncWebServerRequest *request)
{
  String result = "";
  // DEBUG_PRINTLN("** Scan Networks **");

  // WIFI_MODE_APSTA模式下也能实现扫描，
  WiFi.mode(WIFI_MODE_APSTA);

  int n = WiFi.scanNetworks();
  DEBUG_PRINT("scanNetworks n=");
  DEBUG_PRINTLN(n);
  if (n <= 0)
  {
    result = " <label for='ssid'>SSID</label>";
    result += "<input class='form-control' id='ssid' type='text' name='WIFISSID' value='{{ssid}}'> <a onclick='scanNetwork();' class='btn btn-primary mb-2'>Scan</a><div id='networks'></div>";
  }
  else
  {
    result = "<select name='WIFISSID' onChange='updateSSID(this.value);'>";
    result += "<OPTION value=''>--Choose SSID--</OPTION>";
    for (int i = 0; i < n; ++i)
    {
      result += "<OPTION value='";
      result += WiFi.SSID(i);
      result += "'>";
      result += WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ")";
      result += "</OPTION>";
    }
    result += "</select>";
  }

  // WiFi.mode(WIFI_MODE_APSTA);
  request->send(200, F("text/html"), result);
}

// 清除控制台
void handleClearConsole(AsyncWebServerRequest *request)
{
  // logClear();

  // request->send(200, F("text/html"), "");
}

void handleGetVersion(AsyncWebServerRequest *request)
{
  // //\01\02\10\10\02\10\02\10\10\03
  // char output_sprintf[2];
  // uint8_t cmd[10];
  // cmd[0] = 0x01;
  // cmd[1] = 0x02;
  // cmd[2] = 0x10;
  // cmd[3] = 0x10;
  // cmd[4] = 0x02;
  // cmd[5] = 0x10;
  // cmd[6] = 0x02;
  // cmd[7] = 0x10;
  // cmd[8] = 0x10;
  // cmd[9] = 0x03;

  // Serial2.write(cmd, 10);
  // Serial2.flush();

  // String tmpTime;
  // String buff = "";

  // // timeLog = millis();
  // // tmpTime = String(timeLog,DEC);
  // logPush('[');

  // for (int j = 0; j < 50; j++)
  // // for (int j =0;j<tmpTime.length();j++)
  // {
  //   logPush(tmpTime[j]);
  // }
  // logPush(']');
  // logPush('-');
  // logPush('>');

  // for (int i = 0; i < 10; i++)
  // {
  //   sprintf(output_sprintf, "%02x", cmd[i]);
  //   logPush(' ');
  //   logPush(output_sprintf[0]);
  //   logPush(output_sprintf[1]);
  // }
  // logPush('\n');
  // request->send(200, F("text/html"), "");
}

// 有问题，2023-2-5
void Dir(AsyncWebServerRequest *request)
{
  String Fname1, Fname2;
  String result;
  int index = 0;
  Directory(); // Get a list of the current files on the FS

  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += "<h3>Filing System Content</h3><br>";
  if (numfiles > 0)
  {
    result += "<table class='center'>";
    result += "<tr><th>Type</th><th>File Name</th><th>File Size</th><th class='sp'></th><th>Type</th><th>File Name</th><th>File Size</th></tr>";
    while (index < numfiles)
    {
      Fname1 = Filenames[index].filename;
      Fname2 = Filenames[index + 1].filename;
      result += "<tr>";
      result += "<td style = 'width:5%'>" + Filenames[index].ftype + "</td><td style = 'width:25%'>" + Fname1 + "</td><td style = 'width:10%'>" + Filenames[index].fsize + "</td>";
      result += "<td class='sp'></td>";
      if (index < numfiles - 1)
      {
        result += "<td style = 'width:5%'>" + Filenames[index + 1].ftype + "</td><td style = 'width:25%'>" + Fname2 + "</td><td style = 'width:10%'>" + Filenames[index + 1].fsize + "</td>";
      }
      result += "</tr>";
      index = index + 2;
    }
    result += "</table>";
    result += "<p style='background-color:yellow;'><b>" + MessageLine + "</b></p>";
    MessageLine = "";
  }
  else
  {
    result += "<h2>No Files Found</h2>";
  }

  result += F("</body></html>");

  request->send(200, "text/html", result);
}

// 有问题，2023-2-5
void Directory()
{
  numfiles = 0; // Reset number of FS files counter
  File root = LittleFS.open("/");
  if (root)
  {
    root.rewindDirectory();
    File file = root.openNextFile();
    while (file)
    { // Now get all the filenames, file types and sizes
      Filenames[numfiles].filename = (String(file.name()).startsWith("/") ? String(file.name()).substring(1) : file.name());
      Filenames[numfiles].ftype = (file.isDirectory() ? "Dir" : "File");
      Filenames[numfiles].fsize = ConvBinUnits(file.size(), 1);
      file = root.openNextFile();
      numfiles++;
    }
    root.close();
  }
}

// 这里需要修改，实现上传文件到flash中，实现OTA功能。
void UploadFileSelect(AsyncWebServerRequest *request)
{
  String result;

  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += F("<h1>File Upload</h1>");
  // result += F("<div class='row justify-content-md-center'>");

  result += F("<div class='row'>");
  result += F("<div class='col-sm-6'>");

  result += "<p>Select a File to [UPLOAD] to this device</p>";
  result += "<form method = 'POST' action = '/handleupload' enctype='multipart/form-data'>";
  result += "<input type='file' name='filename'><br><br>";
  result += "<input type='submit' value='Upload'>";
  result += "</form>";
  result += F("<html>");

  request->send(200, "text/html", result);
}

void OTAFileSelect(AsyncWebServerRequest *request)
{
  String result;

  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += F("<h1>OTA this Device</h1>");
  // result += F("<div class='row justify-content-md-center'>");

  result += F("<div class='row'>");
  result += F("<div class='col-sm-6'>");

  result += "<p>Select a File to [OTA] to this device</p>";
  result += "<form method = 'POST' action = '/update' enctype='multipart/form-data'>";
  result += "<input type='file' name='filename'><br><br>";
  result += "<input type='submit' value='Upload'>";
  result += "</form>";
  result += F("<html>");

  request->send(200, "text/html", result);
}

void Format(AsyncWebServerRequest *request)
{
  String result;

  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += "<h3>***  Format Filing System on this device ***</h3>";
  result += "<form action='/handleformat'>";
  result += "<input type='radio' id='YES' name='format' value = 'YES'><label for='YES'>YES</label><br><br>";
  result += "<input type='radio' id='NO'  name='format' value = 'NO' checked><label for='NO'>NO</label><br><br>";
  result += "<input type='submit' value='Format?'>";
  result += "</form>";

  result += F("</body></html>");

  request->send(200, "text/html", result);
}

void handleFileUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    String file = filename;
    if (!filename.startsWith("/"))
      file = "/" + filename;
    request->_tempFile = LittleFS.open(file, "w");
    if (!request->_tempFile)
      DEBUG_PRINTLN("Error creating file for upload...");
    start = millis();
  }
  if (request->_tempFile)
  {
    if (len)
    {
      request->_tempFile.write(data, len); // Chunked data
      DEBUG_PRINTLN("Transferred : " + String(len) + " Bytes");
    }
    if (final)
    {
      uploadsize = request->_tempFile.size();
      request->_tempFile.close();
      uploadtime = millis() - start;
      request->redirect("/dir");
    }
  }
}

void File_Stream()
{
  SelectInput("Select a File to Stream", "handlestream", "filename");
}

void File_Delete()
{
  SelectInput("Select a File to Delete", "handledelete", "filename");
}

// Delete the file
void Handle_File_Delete(AsyncWebServerRequest *request, String filename)
{
  String result;

  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  if (!filename.startsWith("/"))
    filename = "/" + filename;
  File dataFile = LittleFS.open(filename, "r"); // Now read FS to see if file exists
  if (dataFile)
  { // It does so delete it
    LittleFS.remove(filename);
    result += "<h3>File '" + filename.substring(1) + "' has been deleted</h3>";
    result += "<a href='/dir'>[Enter]</a><br><br>";
  }
  else
  {
    result += "<h3>File [ " + filename + " ] does not exist</h3>";
    result += "<a href='/dir'>[Enter]</a><br><br>";
  }

  result += F("</body></html>");

  request->send(200, "text/html", result);
}

void File_Rename(AsyncWebServerRequest *request)
{
  String result;

  // 下面函数有问题
  Directory();

  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += "<h3>Select a File to [RENAME] on this device</h3>";
  result += "<FORM action='/renamehandler'>";
  result += "<table class='center'>";
  result += "<tr><th>File name</th><th>New Filename</th><th>Select</th></tr>";
  int index = 0;
  while (index < numfiles)
  {
    result += "<tr><td><input type='text' name='oldfile' style='color:blue;' value = '" + Filenames[index].filename + "' readonly></td>";
    result += "<td><input type='text' name='newfile'></td><td><input type='radio' name='choice'></tr>";
    index++;
  }
  result += "</table><br>";
  result += "<input type='submit' value='Enter'>";
  result += "</form>";

  result += F("</body></html>");

  request->send(200, "text/html", result);
}

void Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args)
{ // Rename the file

  String result;
  String newfilename;
  // int Args = request->args();

  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  for (int i = 0; i < Args; i++)
  {
    if (request->arg(i) != "" && request->arg(i + 1) == "on")
    {
      filename = request->arg(i - 1);
      newfilename = request->arg(i);
    }
  }
  if (!filename.startsWith("/"))
    filename = "/" + filename;
  if (!newfilename.startsWith("/"))
    newfilename = "/" + newfilename;
  File CurrentFile = LittleFS.open(filename, "r"); // Now read FS to see if file exists
  if (CurrentFile && filename != "/" && newfilename != "/" && (filename != newfilename))
  { // It does so rename it, ignore if no entry made, or Newfile name exists already
    if (LittleFS.rename(filename, newfilename))
    {
      filename = filename.substring(1);
      newfilename = newfilename.substring(1);
      result += "<h3>File '" + filename + "' has been renamed to '" + newfilename + "'</h3>";
      result += "<a href='/dir'>[Enter]</a><br><br>";
    }
  }
  else
  {
    if (filename == "/" && newfilename == "/")
      result += "<h3>File was not renamed</h3>";
    else
      result += "<h3>New filename exists, cannot rename</h3>";
    result += "<a href='/rename'>[Enter]</a><br><br>";
  }
  CurrentFile.close();

  result += F("</body></html>");
  request->send(200, "text/html", result);
}

String processor(const String &var)
{
  if (var == "HELLO_FROM_TEMPLATE")
    return F("Hello world!");
  return String();
}

//  Not found handler is also the handler for 'delete', 'download' and 'stream' functions
void notFound(AsyncWebServerRequest *request)
{ // Process selected file types
  String filename;
  if (request->url().startsWith("/downloadhandler") ||
      request->url().startsWith("/streamhandler") ||
      request->url().startsWith("/deletehandler") ||
      request->url().startsWith("/renamehandler"))
  {
    // Now get the filename and handle the request for 'delete' or 'download' or 'stream' functions
    if (!request->url().startsWith("/renamehandler"))
      filename = request->url().substring(request->url().indexOf("~/") + 1);
    start = millis();
    if (request->url().startsWith("/downloadhandler"))
    {
      DEBUG_PRINTLN("Download handler started...");
      MessageLine = "";
      File file = LittleFS.open(filename, "r");
      String contentType = getContentType("download");
      AsyncWebServerResponse *response = request->beginResponse(contentType, file.size(), [file](uint8_t *buffer, size_t maxLen, size_t total) mutable -> size_t
                                                                { return file.read(buffer, maxLen); });
      response->addHeader("Server", "ESP Async Web Server");
      request->send(response);
      downloadtime = millis() - start;
      downloadsize = GetFileSize(filename);
      request->redirect("/dir");
    }
    if (request->url().startsWith("/streamhandler"))
    {
      DEBUG_PRINTLN("Stream handler started...");
      String ContentType = getContentType(filename);
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, filename, ContentType);
      request->send(response);
      downloadsize = GetFileSize(filename);
      downloadtime = millis() - start;
      request->redirect("/dir");
    }
    if (request->url().startsWith("/deletehandler"))
    {
      Handle_File_Delete(request, filename); // Build webpage ready for display
    }
    if (request->url().startsWith("/renamehandler"))
    {
      Handle_File_Rename(request, filename, request->args()); // Build webpage ready for display
    }
  }
  else
  {
    Page_Not_Found(request);
  }
}

// 将webpage修改为result
void Handle_File_Download()
{
  String filename = "";
  String result;
  int index = 0;
  Directory(); // Get a list of files on the FS

  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += "<h3>Select a File to Download</h3>";
  result += "<table>";
  result += "<tr><th>File Name</th><th>File Size</th></tr>";
  while (index < numfiles)
  {
    result += "<tr><td><a href='" + Filenames[index].filename + "'></a><td>" + Filenames[index].fsize + "</td></tr>";
    index++;
  }
  result += "</table>";
  result += "<p>" + MessageLine + "</p>";

  result += F("</body></html>");
}

String getContentType(String filenametype)
{ // Tell the browser what file type is being sent
  if (filenametype == "download")
  {
    return "application/octet-stream";
  }
  else if (filenametype.endsWith(".txt"))
  {
    return "text/plainn";
  }
  else if (filenametype.endsWith(".json"))
  {
    return "text/plainn";
  }
  else if (filenametype.endsWith(".htm"))
  {
    return "text/html";
  }
  else if (filenametype.endsWith(".html"))
  {
    return "text/html";
  }
  else if (filenametype.endsWith(".css"))
  {
    return "text/css";
  }
  else if (filenametype.endsWith(".js"))
  {
    return "application/javascript";
  }
  else if (filenametype.endsWith(".png"))
  {
    return "image/png";
  }
  else if (filenametype.endsWith(".gif"))
  {
    return "image/gif";
  }
  else if (filenametype.endsWith(".jpg"))
  {
    return "image/jpeg";
  }
  else if (filenametype.endsWith(".ico"))
  {
    return "image/x-icon";
  }
  else if (filenametype.endsWith(".xml"))
  {
    return "text/xml";
  }
  else if (filenametype.endsWith(".pdf"))
  {
    return "application/x-pdf";
  }
  else if (filenametype.endsWith(".zip"))
  {
    return "application/x-zip";
  }
  else if (filenametype.endsWith(".gz"))
  {
    return "application/x-gzip";
  }
  return "text/plain";
}

void Select_File_For_Function(AsyncWebServerRequest *request, String title, String function)
{
  String Fname1, Fname2;
  String result;

  int index = 0;
  Directory(); // Get a list of files on the FS

  // result = HTML_Header();

  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += "<h3>Select a File to " + title + " from this device</h3>";
  result += "<table class='center'>";
  result += "<tr><th>File Name</th><th>File Size</th><th class='sp'></th><th>File Name</th><th>File Size</th></tr>";
  while (index < numfiles)
  {
    Fname1 = Filenames[index].filename;
    Fname2 = Filenames[index + 1].filename;
    if (Fname1.startsWith("/"))
      Fname1 = Fname1.substring(1);
    if (Fname2.startsWith("/"))
      Fname1 = Fname2.substring(1);
    result += "<tr>";
    result += "<td style='width:25%'><button><a href='" + function + "~/" + Fname1 + "'>" + Fname1 + "</a></button></td><td style = 'width:10%'>" + Filenames[index].fsize + "</td>";
    result += "<td class='sp'></td>";
    if (index < numfiles - 1)
    {
      result += "<td style='width:25%'><button><a href='" + function + "~/" + Fname2 + "'>" + Fname2 + "</a></button></td><td style = 'width:10%'>" + Filenames[index + 1].fsize + "</td>";
    }
    result += "</tr>";
    index = index + 2;
  }
  result += "</table>";

  result += F("</body></html>");

  request->send(200, "text/html", result);
}

void SelectInput(String Heading, String Command, String Arg_name)
{
  String result;

  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += "<h3>" + Heading + "</h3>";
  result += "<form  action='/" + Command + "'>";
  result += "Filename: <input type='text' name='" + Arg_name + "'><br><br>";
  result += "<input type='submit' value='Enter'>";
  result += "</form>";

  result += F("</body></html>");
}

int GetFileSize(String filename)
{
  int filesize;
  File CheckFile = LittleFS.open(filename, "r");
  filesize = CheckFile.size();
  CheckFile.close();
  return filesize;
}

void Home()
{
  String result;

  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += "<h1>Home Page</h1>";
  result += "<h2>ESP Asychronous WebServer Example</h2>";
  result += "<img src = 'icon' alt='icon'>";
  result += "<h3>File Management - Directory, Upload, Download, Stream and Delete File Examples</h3>";
  result += F("</body></html>");
}

void handleIOT(AsyncWebServerRequest *request)
{
  String html;

  html += F("<html>");
  html += FPSTR(HTTP_HEADER1);
  html += "V" + myVersion;
  html += FPSTR(HTTP_HEADER2);

  // Read the configuration from the file
  // File configFile = LittleFS.open("/config/IOTconfig.json", "r");
  // if (configFile)
  // {
  //   // Parse the JSON data
  //   size_t size = configFile.size();
  //   std::unique_ptr<char[]> buf(new char[size]);
  //   configFile.readBytes(buf.get(), size);
  //   DynamicJsonDocument jsonConfig(1024);
  //   DeserializationError error = deserializeJson(jsonConfig, buf.get());
  //   configFile.close();

  //   if (!error)
  //   {
  //     // Fill the form fields with the configuration data
  //     String productId = jsonConfig["productId"].as<String>();
  //     String deviceNum = jsonConfig["deviceNum"].as<String>();
  //     String authCode = jsonConfig["authCode"].as<String>();
  //     String mqttUserName = jsonConfig["mqttUserName"].as<String>();
  //     String mqttPwd = jsonConfig["mqttPwd"].as<String>();
  //     String mqttSecret = jsonConfig["mqttSecret"].as<String>();
  //     String userId = jsonConfig["userId"].as<String>();

  //     //result +="<h1>Config MQTT</h1>";
  //     result +="<div class='row' >";
  //     result +="<div class='col-sm-6'> <form method='POST' action='saveMQTTCredentials'>";
  //     result +="<div class='card'>";
  //     result +="<div class='card-header'>MQTT Credentials</div>";
  //     result +="<div class='card-body'>";
  //     result +="<div id='MQTTCredentialsConfig'>";
  //     result +="<table style=\"width:100%\">";

  //     result += "<form id='configForm' action='/IOTSave' method='post'>";
  //     result +="<tr><td><label for='productId'>Product ID:</label></td><td><input type='text' id='productId' name='productId' value='" + productId + "' required></td></tr>";
  //     result +="<tr><td><label for='deviceNum'>Device Number:</label></td><td><input type='text' id='deviceNum' name='deviceNum' value='" + deviceNum + "' required></td></tr>";
  //     result +="<tr><td><label for='authCode'>Authorization Code:</label></td><td><input type='text' id='authCode' name='authCode' value='" + authCode + "' required></td></tr>";
  //     result +="<tr><td><label for='mqttUserName'>MQTT Username:</label></td><td><input type='text' id='mqttUserName' name='mqttUserName' value='" + mqttUserName + "' required></td></tr>";
  //     result +="<tr><td><label for='mqttPwd'>MQTT Password:</label></td><td><input type='text' id='mqttPwd' name='mqttPwd' value='" + mqttPwd + "' required></td></tr>";
  //     result +="<tr><td><label for='mqttSecret'>MQTT Secret:</label></td><td><input type='text' id='mqttSecret' name='mqttSecret' value='" + mqttSecret + "' required></td></tr>";
  //     result +="<tr><td><label for='userId'>User ID:</label></td><td><input type='text' id='userId' name='userId' value='" + userId + "' required></td></tr>";

  //     // result += "<label for='productId'>Product ID:</label><input type='text' id='productId' name='productId' value='" + productId + "' required><br><br>";
  //     // result += "<label for='deviceNum'>Device Number:</label><input type='text' id='deviceNum' name='deviceNum' value='" + deviceNum + "' required><br><br>";
  //     // result += "<label for='authCode'>Authorization Code:</label><input type='text' id='authCode' name='authCode' value='" + authCode + "' required><br><br>";
  //     // result += "<label for='mqttUserName'>MQTT Username:</label><input type='text' id='mqttUserName' name='mqttUserName' value='" + mqttUserName + "' required><br><br>";
  //     // result += "<label for='mqttPwd'>MQTT Password:</label><input type='text' id='mqttPwd' name='mqttPwd' value='" + mqttPwd + "' required><br><br>";
  //     // result += "<label for='mqttSecret'>MQTT Secret:</label><input type='text' id='mqttSecret' name='mqttSecret' value='" + mqttSecret + "' required><br><br>";
  //     // result += "<label for='userId'>User ID:</label><input type='text' id='userId' name='userId' value='" + userId + "' required><br><br>";
  //     result += "<button type='submit'>Save Configuration</button>";
  //     result += "</form>";
  //     result += "</table>";
  //     result +=    "</div>";
  //     result +="</div>";
  //     result +="</div>";
  //     result +="</div>";
  //     result +="</div>";
  //   }
  //   else
  //   {
  //     result += "<p>Error reading configuration data</p>";
  //   }
  // }
  // else
  // {
  //   result += "<p>Configuration file not found</p>";
  // }

  // Read the configuration from the file
  File configFile = LittleFS.open("/config/IOTconfig.json", "r");
  if (configFile)
  {
    // Parse the JSON data
    size_t size = configFile.size();
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);
    DynamicJsonDocument jsonConfig(1024);
    DeserializationError error = deserializeJson(jsonConfig, buf.get());
    configFile.close();

    if (!error)
    {
      // Create a table for displaying configuration data

      html += "<h1>Config IOT Parameter</h1>";
      html += "<div class='row' >";
      html += "<div class='col-sm-6'>";
      html += "<div class='card'>";
      html += "<div class='card-header'>MQTT Credentials</div>";
      html += "<div class='card-body'>";
      html += "<div id='MQTTCredentialsConfig'>";

      html += "<form id='configForm' action='/IOTSave' method='post'><table>";
      html += "<tr><td><label for='productId'>Product ID:</label></td><td><input type='text' id='productId' name='productId' value='" + jsonConfig["productId"].as<String>() + "'></td></tr>";
      html += "<tr><td><label for='deviceNum'>Device Number:</label></td><td><input type='text' id='deviceNum' name='deviceNum' value='" + jsonConfig["deviceNum"].as<String>() + "'></td></tr>";
      html += "<tr><td><label for='authCode'>Authorization Code:</label></td><td><input type='text' id='authCode' name='authCode' value='" + jsonConfig["authCode"].as<String>() + "'></td></tr>";
      html += "<tr><td><label for='mqttUserName'>MQTT Username:</label></td><td><input type='text' id='mqttUserName' name='mqttUserName' value='" + jsonConfig["mqttUserName"].as<String>() + "'></td></tr>";
      html += "<tr><td><label for='mqttPwd'>MQTT Password:</label></td><td><input type='text' id='mqttPwd' name='mqttPwd' value='" + jsonConfig["mqttPwd"].as<String>() + "'></td></tr>";
      html += "<tr><td><label for='mqttSecret'>MQTT Secret:</label></td><td><input type='text' id='mqttSecret' name='mqttSecret' value='" + jsonConfig["mqttSecret"].as<String>() + "'></td></tr>";
      html += "<tr><td><label for='userId'>User ID:</label></td><td><input type='text' id='userId' name='userId' value='" + jsonConfig["userId"].as<String>() + "'></td></tr>";
      html += "</table><br><button type='submit'>Save Configuration</button></form>";

      html += "</div>";
      html += "</div>";
      html += "</div>";
      html += "</div>";
      html += "</div>";
    }
    else
    {
      html += "<p>Error reading configuration data</p>";
    }
  }
  else
  {
    html += "<p>Configuration file not found</p>";
  }

  html += "</body></html>";

  request->send(200, "text/html", html);
}

void handleLogOut(AsyncWebServerRequest *request)
{
  String result;
  result += F("<html>");

  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  // 需要将数据放到flash中，
  result += "<h1>Log Out</h1>";
  result += "<p>You have now been logged out...</p>";
  result += "<p>NOTE: On most browsers you must close all windows for this to take effect...</p>";

  result += F("</html>");

  request->send(200, "text/html", result);
}

void Page_Not_Found(AsyncWebServerRequest *request) // 这个页面ok
{
  String result;

  result += F("<html>");
  result += FPSTR(HTTP_HEADER1);
  result += "V" + myVersion;
  result += FPSTR(HTTP_HEADER2);

  result += "<div class='notfound'>";
  result += "<h1>Sorry</h1>";
  result += "<p>Error 404 - Page Not Found</p>";
  result += "</div><div class='left'>";
  result += "<p>The page you were looking for was not found, it may have been moved or is currently unavailable.</p>";
  result += "<p>Please check the address is spelt correctly and try again.</p>";
  result += "<p>Or click <b><a href='/'>[Here]</a></b> for the home page.</p></div>";

  result += F("</body></html>");

  request->send(200, "text/html", result);
}

String ConvBinUnits(int bytes, int resolution)
{
  if (bytes < 1024)
  {
    return String(bytes) + " B";
  }
  else if (bytes < 1024 * 1024)
  {
    return String((bytes / 1024.0), resolution) + " KB";
  }
  else if (bytes < (1024 * 1024 * 1024))
  {
    return String((bytes / 1024.0 / 1024.0), resolution) + " MB";
  }
  else
    return "";
}

String EncryptionType(wifi_auth_mode_t encryptionType)
{
  switch (encryptionType)
  {
  case (WIFI_AUTH_OPEN):
    return "OPEN";
  case (WIFI_AUTH_WEP):
    return "WEP";
  case (WIFI_AUTH_WPA_PSK):
    return "WPA PSK";
  case (WIFI_AUTH_WPA2_PSK):
    return "WPA2 PSK";
  case (WIFI_AUTH_WPA_WPA2_PSK):
    return "WPA WPA2 PSK";
  case (WIFI_AUTH_WPA2_ENTERPRISE):
    return "WPA2 ENTERPRISE";
  case (WIFI_AUTH_MAX):
    return "WPA2 MAX";
  default:
    return "";
  }
}

// 源代码参考：https://github.com/G6EJD/G6EJD-ESP-File-Server/blob/main/ESPAsynch_Server_v1.ino
bool StartMDNSservice(const char *Name)
{
  esp_err_t err = mdns_init(); // Initialise mDNS service
  if (err)
  {
    printf("MDNS Init failed: %d\n", err); // Return if error detected
    return false;
  }
  mdns_hostname_set(Name); // Set hostname
  return true;
}

// 本源代码来自：https://github.com/har-in-air/ESP32_ASYNC_WEB_SERVER_SPIFFS_OTA/blob/master/src/async_server.cpp
// used by server.on functions to discern whether a user has the correct httpapitoken OR is authenticated by username and password
bool server_authenticate(AsyncWebServerRequest *request)
{
  bool isAuthenticated = false;

  if (request->authenticate("user", "pass")) // 这里是后台网页的登录界面用户名和密码
  {
    // if (request->authenticate(config.httpuser.c_str(), config.httppassword.c_str())) {
    DEBUG_PRINTLN("is authenticated via username and password");
    isAuthenticated = true;
  }
  return isAuthenticated;
}

// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String server_ui_size(const size_t bytes)
{
  if (bytes < 1024)
    return String(bytes) + " B";
  else if (bytes < (1024 * 1024))
    return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024))
    return String(bytes / 1024.0 / 1024.0) + " MB";
  else
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

// 本源代码来自：https://github.com/har-in-air/ESP32_ASYNC_WEB_SERVER_SPIFFS_OTA/blob/master/src/async_server.cpp
// list all of the files, if ishtml=true, return html rather than simple text
// 准备参考listDir函数来修改本函数，实现网页显示

// Function to list files in a directory with hierarchical view
String server_directory(bool ishtml, uint8_t levels, const String &path = "/", int depth = 0)
{
  String returnText = "";
  File root = LittleFS.open(path);
  if (!root || !root.isDirectory())
  {
    DEBUG_PRINTLN("- failed to open directory or not a directory");
    return returnText;
  }

  File file = root.openNextFile();

  if (ishtml && depth == 0)
  {
    returnText += "<table align='center'><tr><th>Name</th><th>Size</th><th>Actions</th></tr>";
  }

  while (file)
  {
    String indention = repeatString("-", depth); // Use '>' to represent directory depth in HTML

    if (file.isDirectory())
    {
      if (ishtml)
      {
        returnText += "<tr><td>" + indention + " " + String(file.name()) + "/</td><td>[DIR]</td><td></td></tr>";
      }
      else
      {
        returnText += indention + " DIR: " + String(file.name()) + "\n";
      }
      if (levels)
      {
        returnText += server_directory(ishtml, levels - 1, String(file.name()) + "/", depth + 1);
      }
    }
    else
    {
      if (ishtml)
      {
        returnText += "<tr><td>" + indention + " " + String(file.name()) + "</td><td>" + String(file.size()) + "</td>";
        returnText += "<td><button onclick=\"alert('Download ' + this.parentElement.parentElement.children[0].innerText)\">Download</button>";
        returnText += "<button onclick=\"alert('Delete ' + this.parentElement.parentElement.children[0].innerText)\">Delete</button></td></tr>";
      }
      else
      {
        returnText += indention + " File: " + String(file.name()) + " Size: " + String(file.size()) + "\n";
      }
    }
    file = root.openNextFile();
  }

  if (ishtml && depth == 0)
  {
    returnText += "</table>";
  }
  root.close();
  return returnText;
}
// Utility function to repeat a string n times
String repeatString(const String &str, const unsigned int times)
{
  String result;
  for (unsigned int i = 0; i < times; i++)
  {
    result += str;
  }
  return result;
}

String readConfigFromFS()
{
  String config;
  File configFile = LittleFS.open("/config/IOTconfig.json", "r");
  if (configFile)
  {
    while (configFile.available())
    {
      config += (char)configFile.read();
    }
    configFile.close();
  }
  return config;
}

void saveConfigToNVS(const DynamicJsonDocument &jsonConfig)
{
  // Create a preferences instance
  Preferences prefs;
  prefs.begin("IOTconfig", false); // Namespace "config"

  const char *productId = jsonConfig["productId"];
  const char *deviceNum = jsonConfig["deviceNum"];
  const char *authCode = jsonConfig["authCode"];
  const char *mqttUserName = jsonConfig["mqttUserName"];
  const char *mqttPwd = jsonConfig["mqttPwd"];
  const char *mqttSecret = jsonConfig["mqttSecret"];
  const char *userId = jsonConfig["userId"];

  prefs.putString("productId", productId);
  prefs.putString("deviceNum", deviceNum);
  prefs.putString("authCode", authCode);
  prefs.putString("mqttUserName", mqttUserName);
  prefs.putString("mqttPwd", mqttPwd);
  prefs.putString("mqttSecret", mqttSecret);
  prefs.putString("userId", userId);

  prefs.end();
}

bool saveConfigToFS(const DynamicJsonDocument &jsonConfig)
{
  String jsonString;
  serializeJson(jsonConfig, jsonString);

  File configFile = LittleFS.open("/config/IOTconfig.json", "w");
  if (!configFile)
  {
    return false;
  }

  if (configFile.print(jsonString))
  {
    configFile.close();
    return true;
  }
  else
  {
    configFile.close();
    return false;
  }
}

String readConfigFromNVS()
{
  // Create a preferences instance with the "IOTconfig" namespace
  Preferences prefs;
  prefs.begin("IOTconfig", false); // Namespace "IOTconfig"

  // Read each value from NVS
  String productId = prefs.getString("productId", "");
  String deviceNum = prefs.getString("deviceNum", "");
  String authCode = prefs.getString("authCode", "");
  String mqttUserName = prefs.getString("mqttUserName", "");
  String mqttPwd = prefs.getString("mqttPwd", "");
  String mqttSecret = prefs.getString("mqttSecret", "");
  String userId = prefs.getString("userId", "");

  // End preferences
  prefs.end();

  // Create a JSON object to hold the configuration data
  DynamicJsonDocument jsonConfig(1024);
  jsonConfig["productId"] = productId;
  jsonConfig["deviceNum"] = deviceNum;
  jsonConfig["authCode"] = authCode;
  jsonConfig["mqttUserName"] = mqttUserName;
  jsonConfig["mqttPwd"] = mqttPwd;
  jsonConfig["mqttSecret"] = mqttSecret;
  jsonConfig["userId"] = userId;

  // Serialize the JSON object to a string
  String jsonString;
  serializeJson(jsonConfig, jsonString);

  return jsonString;
}

void initWebServer()
{
  // myVersion=RWBoardParameter("READ", "MYVERSION", "",General);

  // 本地域名，通过路由器可解析，http://block.local/，注意，必须打上http://才可以访问
  if (!StartMDNSservice("block"))
  {
    DEBUG_PRINTLN("Error starting mDNS Service...");
    delay(10);
    return;
  }

  // 如下的LittleFS是对的
  serverWeb.serveStatic("/web/js/jquery-min.js", LittleFS, "/web/js/jquery-min.js");
  serverWeb.serveStatic("/web/js/functions.js", LittleFS, "/web/js/functions.js");
  serverWeb.serveStatic("/web/js/bootstrap.min.js", LittleFS, "/web/js/bootstrap.min.js");
  serverWeb.serveStatic("/web/js/bootstrap.min.js.map", LittleFS, "/web/js/bootstrap.min.js.map");
  serverWeb.serveStatic("/web/css/bootstrap.min.css", LittleFS, "/web/css/bootstrap.min.css");
  serverWeb.serveStatic("/web/css/style.css", LittleFS, "/web/css/style.css");
  serverWeb.serveStatic("/web/img/logo.png", LittleFS, "/web/img/logo.png");
  serverWeb.serveStatic("/web/img/wait.gif", LittleFS, "/web/img/wait.gif");
  serverWeb.serveStatic("/web/img/nok.png", LittleFS, "/web/img/nok.png");
  serverWeb.serveStatic("/web/img/ok.png", LittleFS, "/web/img/ok.png");
  serverWeb.serveStatic("/web/img/", LittleFS, "/web/img/");

  // 登录http://block.local后看到的页面，也可以登录http://192.168.X.Y后看到的页面
  serverWeb.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleRoot(request); });

  // config/general菜单
  serverWeb.on("/general", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleGeneral(request); });
  serverWeb.on("/saveGeneral", HTTP_POST, [](AsyncWebServerRequest *request)
               { handleSaveGeneral(request); });

  // config/modbus菜单
  serverWeb.on("/serial", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleSerial(request); });
  serverWeb.on("/saveSerial", HTTP_POST, [](AsyncWebServerRequest *request)
               { handleSaveSerial(request); });

  // config/ethernet菜单
  serverWeb.on("/ethernet", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleEther(request); });
  serverWeb.on("/saveEther", HTTP_POST, [](AsyncWebServerRequest *request)
               { handleSaveEther(request); });

  // config/module 菜单，用于2g或4g模块
  serverWeb.on("/module", HTTP_GET, [](AsyncWebServerRequest *request)
               { handlemodule(request); });
  serverWeb.on("/savemodule", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleSavemodule(request); });

  // config/wifi菜单
  serverWeb.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleWifi(request); });
  serverWeb.on("/scanNetwork", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleScanNetwork(request); });
  serverWeb.on("/saveWifi", HTTP_POST, [](AsyncWebServerRequest *request) // 以post方式存数据
               { handleSaveWifi(request); });

  // config/mqtt菜单
  serverWeb.on("/mqtt", HTTP_GET, [](AsyncWebServerRequest *request)
               { handlemqtt(request); });
  serverWeb.on("/savemqtt", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleSavemqtt(request); });

  // config/smart meter菜单
  serverWeb.on("/meter", HTTP_GET, [](AsyncWebServerRequest *request)
               { handlemeter(request); });
  serverWeb.on("/savemeter", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleSavemeter(request); });

  // config/huawei菜单
  serverWeb.on("/huawei", HTTP_GET, [](AsyncWebServerRequest *request)
               { handlehuawei(request); });
  serverWeb.on("/savehuawei", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleSavehuawei(request); });

  // config/config content菜单
  serverWeb.on("/fsbrowser", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleFSbrowser(request); });
  serverWeb.on("/saveFile", HTTP_POST, [](AsyncWebServerRequest *request)
               { handleSavefile(request); });

  serverWeb.on("/tools", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleTools(request); });

  // File/directory菜单
  serverWeb.on("/dir", HTTP_GET, [](AsyncWebServerRequest *request)
               { handlemydir(request); });

  // File//upload菜单
  serverWeb.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request)
               { UploadFileSelect(request); });

  // File/stream菜单
  serverWeb.on("/stream", HTTP_GET, [](AsyncWebServerRequest *request)
               { Select_File_For_Function(request, "[STREAM]", "streamhandler"); });

  // File/rename菜单
  serverWeb.on("/rename", HTTP_GET, [](AsyncWebServerRequest *request)
               { File_Rename(request); });

  // Tools/console菜单
  serverWeb.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleLogs(request); });

  // System/Reboot菜单
  serverWeb.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleReboot(request); });

  // System/OTA菜单
  serverWeb.on("/OTA", HTTP_GET, [](AsyncWebServerRequest *request)
               { OTAFileSelect(request); });

  serverWeb.on(
      "/update", HTTP_POST, [](AsyncWebServerRequest *request)
      {
    // OTA更新结束
    // 通过判断更新是否成功，发送不同的响应给客户端
    bool updateSuccess = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", updateSuccess ? "Update Success! Rebooting..." : "Update Failed!");
    response->addHeader("Connection", "close");
    request->send(response);
    if(updateSuccess) {
      // 如果更新成功，则延迟重启
      delay(1000);
      ESP.restart();
    } },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        // 接收上传的.bin文件并进行OTA更新
        if (!index)
        {
          // 如果是上传的第一个数据包，就开始更新过程
          Serial.printf("Update Start: %s\n", filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN))
          { // UPDATE_SIZE_UNKNOWN允许接收任何大小的文件
            Update.printError(Serial);
          }
        }

        // 写入上传的数据
        if (!Update.hasError())
        {
          if (Update.write(data, len) != len)
          {
            Update.printError(Serial);
          }
        }

        if (final)
        {
          // 如果是最后一个数据包
          if (Update.end(true))
          { // true表示当上传完成后应用更新
            Serial.printf("Update Success: %uB\n", index + len);
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });

  // system/help菜单
  serverWeb.on("/help", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleHelp(request); });

  // system/logout菜单
  serverWeb.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleLogOut(request); });

  serverWeb.on("/readFile", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleReadfile(request); });

  serverWeb.on("/getLogBuffer", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleLogBuffer(request); });

  serverWeb.on("/cmdClearConsole", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleClearConsole(request); });

  serverWeb.on("/cmdGetVersion", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleGetVersion(request); });

  serverWeb.on("/download", HTTP_GET, [](AsyncWebServerRequest *request)
               { Select_File_For_Function(request, "[DOWNLOAD]", "downloadhandler"); }); // Build webpage ready for display

  // Set handler for '/handleupload'
  serverWeb.on(
      "/handleupload", HTTP_POST, [](AsyncWebServerRequest *request) {}, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
      { handleFileUpload(request, filename, index, data, len, final); });

  // Set handler for '/handleformat'
  serverWeb.on("/handleformat", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    if (request->getParam("format")->value() == "YES") {
      DEBUG_PRINT("Starting to Format Filing System...");
      LittleFS.end();
      bool formatted = LittleFS.format();
      if (formatted) {
        DEBUG_PRINTLN(" Successful Filing System Format...");
      }
      else         {
        DEBUG_PRINTLN(" Formatting Failed...");
      }
    }
    request->redirect("/dir"); });

  serverWeb.on("/IOT", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleIOT(request); });

  serverWeb.on("/IOTSave", HTTP_POST, [](AsyncWebServerRequest *request)
               {
    if (request->hasArg("productId") && request->hasArg("deviceNum") && request->hasArg("authCode") &&
        request->hasArg("mqttUserName") && request->hasArg("mqttPwd") && request->hasArg("mqttSecret") &&
        request->hasArg("userId")) {

      String productId = request->arg("productId");
      String deviceNum = request->arg("deviceNum");
      String authCode = request->arg("authCode");
      String mqttUserName = request->arg("mqttUserName");
      String mqttPwd = request->arg("mqttPwd");
      String mqttSecret = request->arg("mqttSecret");
      String userId = request->arg("userId");

      DynamicJsonDocument jsonConfig(1024);
      jsonConfig["productId"] = productId;
      jsonConfig["deviceNum"] = deviceNum;
      jsonConfig["authCode"] = authCode;
      jsonConfig["mqttUserName"] = mqttUserName;
      jsonConfig["mqttPwd"] = mqttPwd;
      jsonConfig["mqttSecret"] = mqttSecret;
      jsonConfig["userId"] = userId;

      if (saveConfigToFS(jsonConfig)) {
        saveConfigToNVS(jsonConfig);

        request->send(200, "text/html", "Configuration saved successfully.");
      } else {
        request->send(500, "text/html", "Failed to save configuration.");
      }
    } else {
      request->send(400, "text/html", "Bad Request");
    } });

  serverWeb.on("/IOTData", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    String jsonConfig = readConfigFromFS();
    if (jsonConfig.isEmpty()) {
      jsonConfig = readConfigFromNVS();
    }
    request->send(200, "application/json", jsonConfig); });

  serverWeb.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request)
               { Select_File_For_Function(request, "[DELETE]", "deletehandler"); });

  serverWeb.on("/format", HTTP_GET, [](AsyncWebServerRequest *request)
               { Format(request); });

  // System/information菜单
  serverWeb.on("/system", HTTP_GET, [](AsyncWebServerRequest *request)
               { handleSystem(request); });

  serverWeb.on("/icon", HTTP_GET, [](AsyncWebServerRequest *request)
               { request->send(LittleFS, "/icon.gif", "image/gif"); });

  serverWeb.onNotFound(notFound);

  // serverWeb.onNotFound(handleNotFound);
  serverWeb.begin();
}