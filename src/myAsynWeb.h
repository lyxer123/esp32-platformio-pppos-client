#ifndef __MYASYNWEB_H_
#define __MYASYNWEB_H_

#include "Arduino.h"
#include "mySystem.h"
#include <ESPmDNS.h>           // Built-in

#include <AsyncTCP.h>          // https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h> // https://github.com/me-no-dev/ESPAsyncWebServer

extern String myVersion;

void initWebServer();
void webServerHandleClient();


void listDir2(fs::FS &fs, const char *dirname, uint8_t levels, bool ishtml, uint8_t depth );
String createIndent(uint8_t depth);

//mqtt的处理
void handlemqtt(AsyncWebServerRequest *request);
void handleSavemqtt(AsyncWebServerRequest *request);

//智能电表的处理
void handlemeter(AsyncWebServerRequest *request);
void handleSavemeter(AsyncWebServerRequest *request);

void handleSystem(AsyncWebServerRequest *request);

void handleLogOut(AsyncWebServerRequest *request);

void Dir(AsyncWebServerRequest * request) ;
void Directory() ;

void UploadFileSelect(AsyncWebServerRequest *request);
void Format(AsyncWebServerRequest *request);

void handleFileUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) ;

void File_Stream() ;
void File_Delete() ;

void Handle_File_Delete(AsyncWebServerRequest *request,String filename);
void File_Rename(AsyncWebServerRequest *request);
void Handle_File_Rename(AsyncWebServerRequest *request, String filename, int Args) ;
void Select_File_For_Function(AsyncWebServerRequest *request,String title, String function);


String processor(const String& var) ;

void notFound(AsyncWebServerRequest *request) ;
void Handle_File_Download() ;
String getContentType(String filenametype) ;

void SelectInput(String Heading, String Command, String Arg_name) ;
int GetFileSize(String filename) ;
void Home() ;


String repeatString(const String &str, const unsigned int times);
String server_ui_size(size_t bytes);
//void Display_New_Page(AsyncWebServerRequest *request);

void Page_Not_Found(AsyncWebServerRequest *request);

void Display_System_Info() ;
String ConvBinUnits(int bytes, int resolution) ;
String EncryptionType(wifi_auth_mode_t encryptionType);
bool StartMDNSservice(const char* Name) ;


String server_directory(bool ishtml,uint8_t levels);
String server_ui_size(const size_t bytes) ;


String readConfigFromFS() ;
void saveConfigToNVS(const DynamicJsonDocument& jsonConfig);
bool saveConfigToFS(const DynamicJsonDocument& jsonConfig);
String readConfigFromNVS();

bool readJsonConfigFromFS(const char* path,DynamicJsonDocument& jsonConfig) ;


#endif 
