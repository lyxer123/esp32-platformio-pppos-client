#ifndef MY_4G_FUNCTION_H
#define MY_4G_FUNCTION_H

#include "Arduino.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "esp_netif_ppp.h"
#include "mqtt_client.h"
#include "esp_modem.h"
#include "esp_modem_netif.h"
#include "esp_log.h"

typedef struct {
    char *topic;
    int topic_len;
    char *data;
    int data_len;
} mqtt_message_t;

extern esp_mqtt_client_handle_t client;
extern modem_dce_t *dce;
extern EventGroupHandle_t event_group;
extern const int CONNECT_BIT;
extern const int STOP_BIT;
extern const int GOT_DATA_BIT;

extern bool isMqttConnected ;
extern bool isMqttSubscribed;

void subscribeToTopic(const char *topic, int qos);
void publishMessage(const char *topic, const char *data);

void initializeModem();
void handleModem();
// void initModemMQTT(void);
void initModemMQTT(const char* client_id, const char* mqtt_url, int mqtt_port, const char* user, const char* password);


static void on_ip_event(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);
static void on_ppp_changed(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
void my4GInformation(modem_dce_t *dce);
String my4gModemOperator(void);


// 声明处理 MQTT 消息的函数
void handle_mqtt_message(const mqtt_message_t *message);

#endif // MY_4G_FUNCTION_H