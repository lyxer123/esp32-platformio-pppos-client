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
void initModemMQTT(void);

static void on_ip_event(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);
static void on_ppp_changed(void *arg, esp_event_base_t event_base,int32_t event_id, void *event_data);
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
void my4GInformation(modem_dce_t *dce);
String my4gModemOperator(void);

#endif // MY_4G_FUNCTION_H