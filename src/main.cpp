#include <Arduino.h>
/* PPPoS Client Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "esp_netif_ppp.h"
#include "mqtt_client.h"
#include "esp_modem.h"
#include "esp_modem_netif.h"
#include "esp_log.h"
#include "sim800.h"
#include "bg96.h"
#include "sim7600.h"

#include <esp_http_client.h>

#define BROKER_URL "mqtt://test.mosquitto.org"

const char *mqtt_server = "test.mosquitto.org"; // 替换为您的 MQTT 服务器地址
const int mqtt_port = 1883;                     // MQTT 端口，通常为 1883
esp_mqtt_client_handle_t client;

static const char *TAG = "pppos_example";
static EventGroupHandle_t event_group = NULL;
static const int CONNECT_BIT = BIT0;
static const int STOP_BIT = BIT1;
static const int GOT_DATA_BIT = BIT2;
modem_dce_t *dce = NULL;

// 状态标志
bool isMqttConnected = false;
bool isMqttSubscribed = true;

// 订阅 MQTT topic 的函数
void subscribeToTopic(const char *topic, int qos)
{
    esp_mqtt_client_subscribe(client, topic, qos);
}

// 发布 MQTT 消息的函数
void publishMessage(const char *topic, const char *data)
{
    esp_mqtt_client_publish(client, topic, data, 0, 1, 0);
}

#if CONFIG_EXAMPLE_SEND_MSG
/**
 * @brief This example will also show how to send short message using the infrastructure provided by esp modem library.
 * @note Not all modem support SMG.
 *
 */
static esp_err_t example_default_handle(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    else if (strstr(line, MODEM_RESULT_CODE_ERROR))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    return err;
}

static esp_err_t example_handle_cmgs(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    else if (strstr(line, MODEM_RESULT_CODE_ERROR))
    {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    else if (!strncmp(line, "+CMGS", strlen("+CMGS")))
    {
        err = ESP_OK;
    }
    return err;
}

#define MODEM_SMS_MAX_LENGTH (128)
#define MODEM_COMMAND_TIMEOUT_SMS_MS (120000)
#define MODEM_PROMPT_TIMEOUT_MS (100)

static esp_err_t example_send_message_text(modem_dce_t *dce, const char *phone_num, const char *text)
{
    modem_dte_t *dte = dce->dte;
    dce->handle_line = example_default_handle;
    /* Set text mode */
    if (dte->send_cmd(dte, "AT+CMGF=1\r", MODEM_COMMAND_TIMEOUT_DEFAULT) != ESP_OK)
    {
        ESP_LOGE(TAG, "send command failed");
        goto err;
    }
    if (dce->state != MODEM_STATE_SUCCESS)
    {
        ESP_LOGE(TAG, "set message format failed");
        goto err;
    }
    ESP_LOGD(TAG, "set message format ok");
    /* Specify character set */
    dce->handle_line = example_default_handle;
    if (dte->send_cmd(dte, "AT+CSCS=\"GSM\"\r", MODEM_COMMAND_TIMEOUT_DEFAULT) != ESP_OK)
    {
        ESP_LOGE(TAG, "send command failed");
        goto err;
    }
    if (dce->state != MODEM_STATE_SUCCESS)
    {
        ESP_LOGE(TAG, "set character set failed");
        goto err;
    }
    ESP_LOGD(TAG, "set character set ok");
    /* send message */
    char command[MODEM_SMS_MAX_LENGTH] = {0};
    int length = snprintf(command, MODEM_SMS_MAX_LENGTH, "AT+CMGS=\"%s\"\r", phone_num);
    /* set phone number and wait for "> " */
    dte->send_wait(dte, command, length, "\r\n> ", MODEM_PROMPT_TIMEOUT_MS);
    /* end with CTRL+Z */
    snprintf(command, MODEM_SMS_MAX_LENGTH, "%s\x1A", text);
    dce->handle_line = example_handle_cmgs;
    if (dte->send_cmd(dte, command, MODEM_COMMAND_TIMEOUT_SMS_MS) != ESP_OK)
    {
        ESP_LOGE(TAG, "send command failed");
        goto err;
    }
    if (dce->state != MODEM_STATE_SUCCESS)
    {
        ESP_LOGE(TAG, "send message failed");
        goto err;
    }
    ESP_LOGD(TAG, "send message ok");
    return ESP_OK;
err:
    return ESP_FAIL;
}
#endif

// 配置 HTTP 客户端
esp_http_client_config_t http_config = {
    .url = "http://httpbin.org/anything?client=ESP32_PPPoS", // 替换为您要访问的 URL
};

// HTTP 客户端配置
esp_http_client_config_t http_config1 = {
    .url = "http://example.com/post", // 替换为您的目标 URL
    .method = HTTP_METHOD_POST,       // 设置为 POST 方法
};

// HTTP 事件处理函数
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer; // 缓冲区存储响应内容
    static int output_len;      // 接收到的数据长度

    switch (evt->event_id)

    {
    case HTTP_EVENT_ERROR:
        Serial.println("HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        Serial.println("HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        Serial.println("HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        Serial.printf("HTTP_EVENT_ON_HEADER, key=%s, value=%s\n", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // 如果响应不是分块的，直接打印数据
            Serial.printf("HTTP_EVENT_ON_DATA, len=%d\n", evt->data_len);
            if (output_buffer == NULL)
            {
                output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
                output_len = 0;
                if (output_buffer == NULL)
                {
                    Serial.println("Failed to allocate memory for output buffer");
                    return ESP_FAIL;
                }
            }
            memcpy(output_buffer + output_len, evt->data, evt->data_len);
            output_len += evt->data_len;
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        Serial.println("HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL)
        {
            // 输出接收到的完整数据
            Serial.print("Received data:");
            Serial.println(output_buffer);
            free(output_buffer);
            output_buffer = NULL;
            output_len = 0;
        }
        break;
    case HTTP_EVENT_DISCONNECTED:
        Serial.println("HTTP_EVENT_DISCONNECTED");
        if (output_buffer != NULL)
        {
            free(output_buffer);
            output_buffer = NULL;
            output_len = 0;
        }
        break;
    }
    return ESP_OK;
}

static void modem_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case ESP_MODEM_EVENT_PPP_START:
        ESP_LOGI(TAG, "Modem PPP Started");
        break;
    case ESP_MODEM_EVENT_PPP_STOP:
        ESP_LOGI(TAG, "Modem PPP Stopped");
        xEventGroupSetBits(event_group, STOP_BIT);
        break;
    case ESP_MODEM_EVENT_UNKNOWN:
        ESP_LOGW(TAG, "Unknow line received: %s", (char *)event_data);
        break;
    default:
        break;
    }
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    client = event->client;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        Serial.println("MQTT_EVENT_CONNECTED");
        // 连接成功后订阅 topics
        // subscribeToTopic("/topic/qos0", 0);
        // subscribeToTopic("/topic/qos1", 1);
        // subscribeToTopic("/another/topic", 0);
        // break;

        // Serial.println("MQTT Connected");
        isMqttConnected = true;
        break;

    case MQTT_EVENT_DISCONNECTED:
        // Serial.println("MQTT_EVENT_DISCONNECTED");
        // break;

        Serial.println("MQTT Disconnected");
        isMqttConnected = false;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        Serial.print("Subscribed to topic with msg_id=");
        Serial.println(event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        Serial.print("Unsubscribed from topic with msg_id=");
        Serial.println(event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        Serial.print("Message published with msg_id=");
        Serial.println(event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        Serial.print("Received data on topic=");
        Serial.write(event->topic, event->topic_len);
        Serial.println();
        Serial.print("Data: ");
        Serial.write(event->data, event->data_len);
        Serial.println();
        break;

    case MQTT_EVENT_ERROR:
        Serial.println("MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            Serial.print("Last error code reported from esp-tls: ");
            Serial.println(event->error_handle->esp_tls_last_esp_err);
            Serial.print("Last tls stack error number: ");
            Serial.println(event->error_handle->esp_tls_stack_err);
            Serial.print("Last captured errno : ");
            Serial.println(event->error_handle->esp_transport_sock_errno);
            Serial.println("Translated to esp_err_t: ");
            // Serial.println(esp_transport_sock_errno_trans(event->error_handle->esp_transport_sock_errno));
        }
        break;

    default:
        Serial.print("Unhandled MQTT event id=");
        Serial.println(event->event_id);
        break;
    }
    return ESP_OK;
}

static void on_ppp_changed(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "PPP state changed event %d", event_id);
    if (event_id == NETIF_PPP_ERRORUSER)
    {
        /* User interrupted event from esp-netif */
        esp_netif_t *netif = *(esp_netif_t **)event_data;
        ESP_LOGI(TAG, "User interrupted event from netif:%p", netif);
    }
}

static void on_ip_event(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "IP event! %d", event_id);
    if (event_id == IP_EVENT_PPP_GOT_IP)
    {
        esp_netif_dns_info_t dns_info;

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        esp_netif_t *netif = event->esp_netif;

        ESP_LOGI(TAG, "Modem Connect to PPP Server");
        ESP_LOGI(TAG, "~~~~~~~~~~~~~~");
        ESP_LOGI(TAG, "IP          : " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Netmask     : " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "Gateway     : " IPSTR, IP2STR(&event->ip_info.gw));
        esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
        ESP_LOGI(TAG, "Name Server1: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        esp_netif_get_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);
        ESP_LOGI(TAG, "Name Server2: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
        ESP_LOGI(TAG, "~~~~~~~~~~~~~~");
        xEventGroupSetBits(event_group, CONNECT_BIT);

        ESP_LOGI(TAG, "GOT ip event!!!");
    }
    else if (event_id == IP_EVENT_PPP_LOST_IP)
    {
        ESP_LOGI(TAG, "Modem Disconnect from PPP Server");
    }
    else if (event_id == IP_EVENT_GOT_IP6)
    {
        ESP_LOGI(TAG, "GOT IPv6 event!");

        ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
        ESP_LOGI(TAG, "Got IPv6 address " IPV6STR, IPV62STR(event->ip6_info.ip));
    }
}

void setup(void)
{
    Serial.begin(115200);
    pinMode(16, OUTPUT);
    digitalWrite(16, HIGH);
    delay(1000);

#if CONFIG_LWIP_PPP_PAP_SUPPORT
    esp_netif_auth_type_t auth_type = NETIF_PPP_AUTHTYPE_PAP;
#elif CONFIG_LWIP_PPP_CHAP_SUPPORT
    esp_netif_auth_type_t auth_type = NETIF_PPP_AUTHTYPE_CHAP;
#elif !defined(CONFIG_EXAMPLE_MODEM_PPP_AUTH_NONE)
#error "Unsupported AUTH Negotiation"
#endif
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &on_ip_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID, &on_ppp_changed, NULL));

    event_group = xEventGroupCreate();

    /* create dte object */
    esp_modem_dte_config_t config = ESP_MODEM_DTE_DEFAULT_CONFIG();
    /* setup UART specific configuration based on kconfig options */
    config.tx_io_num = CONFIG_EXAMPLE_MODEM_UART_TX_PIN;
    config.rx_io_num = CONFIG_EXAMPLE_MODEM_UART_RX_PIN;
    config.rts_io_num = CONFIG_EXAMPLE_MODEM_UART_RTS_PIN;
    config.cts_io_num = CONFIG_EXAMPLE_MODEM_UART_CTS_PIN;
    config.rx_buffer_size = CONFIG_EXAMPLE_MODEM_UART_RX_BUFFER_SIZE;
    config.tx_buffer_size = CONFIG_EXAMPLE_MODEM_UART_TX_BUFFER_SIZE;
    config.pattern_queue_size = CONFIG_EXAMPLE_MODEM_UART_PATTERN_QUEUE_SIZE;
    config.event_queue_size = CONFIG_EXAMPLE_MODEM_UART_EVENT_QUEUE_SIZE;
    config.event_task_stack_size = CONFIG_EXAMPLE_MODEM_UART_EVENT_TASK_STACK_SIZE;
    config.event_task_priority = CONFIG_EXAMPLE_MODEM_UART_EVENT_TASK_PRIORITY;
    config.line_buffer_size = CONFIG_EXAMPLE_MODEM_UART_RX_BUFFER_SIZE / 2;

    modem_dte_t *dte = esp_modem_dte_init(&config);
    /* Register event handler */
    ESP_ERROR_CHECK(esp_modem_set_event_handler(dte, modem_event_handler, ESP_EVENT_ANY_ID, NULL));

    // Init netif object
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_PPP();
    esp_netif_t *esp_netif = esp_netif_new(&cfg);
    assert(esp_netif);

    void *modem_netif_adapter = esp_modem_netif_setup(dte);
    esp_modem_netif_set_default_handlers(modem_netif_adapter, esp_netif);

    while (!dce)
    {
        /* create dce object */
#if CONFIG_EXAMPLE_MODEM_DEVICE_SIM800
        dce = sim800_init(dte);
#elif CONFIG_EXAMPLE_MODEM_DEVICE_BG96
        dce = bg96_init(dte);
#elif CONFIG_EXAMPLE_MODEM_DEVICE_SIM7600
        dce = sim7600_init(dte);
#else
#error "Unsupported DCE"
#endif
    }

    assert(dce != NULL);
    ESP_ERROR_CHECK(dce->set_flow_ctrl(dce, MODEM_FLOW_CONTROL_NONE));
    ESP_ERROR_CHECK(dce->store_profile(dce));
    while (!dce->oper || strlen(dce->oper) < 2)
    {
        delay(1000);
        if (dce && !dce->oper < 2)
            esp_modem_dce_get_operator_name(dce);
        else
            ESP_LOGW(TAG, "Operator name: %s", dce ? dce->oper : "");
    }

    /* Print Module ID, Operator, IMEI, IMSI */
    ESP_LOGI(TAG, "Module: %s", dce->name);
    ESP_LOGI(TAG, "Operator: %s", dce->oper);
    ESP_LOGI(TAG, "IMEI: %s", dce->imei);
    ESP_LOGI(TAG, "IMSI: %s", dce->imsi);
    /* Get signal quality */
    uint32_t rssi = 0, ber = 0;
    ESP_ERROR_CHECK(dce->get_signal_quality(dce, &rssi, &ber));
    ESP_LOGI(TAG, "rssi: %d, ber: %d", rssi, ber);
    /* Get battery voltage */
    uint32_t voltage = 0, bcs = 0, bcl = 0;
    ESP_ERROR_CHECK(dce->get_battery_status(dce, &bcs, &bcl, &voltage));
    ESP_LOGI(TAG, "Battery voltage: %d mV", voltage);
    /* setup PPPoS network parameters */
#if !defined(CONFIG_EXAMPLE_MODEM_PPP_AUTH_NONE) && (defined(CONFIG_LWIP_PPP_PAP_SUPPORT) || defined(CONFIG_LWIP_PPP_CHAP_SUPPORT))
    esp_netif_ppp_set_auth(esp_netif, auth_type, CONFIG_EXAMPLE_MODEM_PPP_AUTH_USERNAME, CONFIG_EXAMPLE_MODEM_PPP_AUTH_PASSWORD);
#endif
    /* attach the modem to the network interface */
    esp_netif_attach(esp_netif, modem_netif_adapter);
    /* Wait for IP address */
    xEventGroupWaitBits(event_group, CONNECT_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

    // 初始化 HTTP 客户端
    // http_config.event_handler = _http_event_handler;

    /* Config MQTT */
    esp_mqtt_client_config_t mqtt_config = {
        .event_handle = mqtt_event_handler,
        .uri = BROKER_URL,
    };
    esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_start(mqtt_client);
    // xEventGroupWaitBits(event_group, GOT_DATA_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

    // http_config.event_handler = _http_event_handler;

    // --------------------------------- > DEINIT PPPOS <------------------------------- //

    // esp_mqtt_client_destroy(mqtt_client);

    // /* Exit PPP mode */
    // ESP_ERROR_CHECK(esp_modem_stop_ppp(dte));

    // xEventGroupWaitBits(event_group, STOP_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

    // /* Power down module */
    // ESP_ERROR_CHECK(dce->power_down(dce));
    // ESP_LOGI(TAG, "Power down");
    // ESP_ERROR_CHECK(dce->deinit(dce));

    // ESP_LOGI(TAG, "Restart after 60 seconds");
    // vTaskDelay(pdMS_TO_TICKS(60000));
    // }

    /* Unregister events, destroy the netif adapter and destroy its esp-netif instance */
    // esp_modem_netif_clear_default_handlers(modem_netif_adapter);
    // esp_modem_netif_teardown(modem_netif_adapter);
    // esp_netif_destroy(esp_netif);

    // ESP_ERROR_CHECK(dte->deinit(dte));
}

void loop()
{
    if (isMqttConnected && isMqttSubscribed)
    {
        // MQTT 连接后的逻辑（如发布或订阅）
        subscribeToTopic("/topic/qos0", 0);
        subscribeToTopic("/topic/qos1", 1);
        subscribeToTopic("/another/topic", 0);
        isMqttSubscribed=false;
    }

    if (!isMqttConnected )
    {
        isMqttSubscribed=true;
    }
        

    // Serial.print("void loop ");
    http_config.event_handler = _http_event_handler;
    esp_http_client_handle_t client = esp_http_client_init(&http_config);

    // 发送 HTTP GET 请求
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        Serial.print("HTTP GET Status = ");
        Serial.println(esp_http_client_get_status_code(client));
    }
    else
    {
        Serial.print("HTTP GET request failed: ");
        Serial.println(esp_err_to_name(err));
    }

    // 清理
    esp_http_client_cleanup(client);

    delay(5000);
}
