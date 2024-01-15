#include "Arduino.h"
#include <esp_http_client.h>
#include "my4GFunction.h"
#include "sim800.h"
#include "bg96.h"
#include "sim7600.h"

#define BROKER_URL "mqtt://test.mosquitto.org"

const char *mqtt_server = "test.mosquitto.org"; // 替换为您的 MQTT 服务器地址
const int mqtt_port = 1883;                     // MQTT 端口，通常为 1883
esp_mqtt_client_handle_t client;

// static EventGroupHandle_t event_group = NULL;
modem_dce_t *dce = NULL;
EventGroupHandle_t event_group = NULL;
const int CONNECT_BIT = BIT0;
const int STOP_BIT = BIT1;
const int GOT_DATA_BIT = BIT2;

static const char *TAG = "pppos_example";

bool isMqttConnected = false;
bool isMqttSubscribed = true;

// GET 请求函数定义

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
        isMqttConnected = true;
        break;

    case MQTT_EVENT_DISCONNECTED:
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
        // Serial.print("Received data on topic=");
        // Serial.write(event->topic, event->topic_len);
        // Serial.println();
        // Serial.print("Data: ");
        // Serial.write(event->data, event->data_len);
        // Serial.println();

        // 创建 mqtt_message_t 结构并填充数据
        mqtt_message_t message;
        message.topic = strndup(event->topic, event->topic_len);
        message.topic_len = event->topic_len;
        message.data = strndup(event->data, event->data_len);
        message.data_len = event->data_len;

        // 调用处理函数
        handle_mqtt_message(&message);

        // 释放分配的内存
        free(message.topic);
        free(message.data);
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

void handle_mqtt_message(const mqtt_message_t *message) {
    if (message == NULL) return;

    Serial.print("Received MQTT message on topic: ");
    Serial.write((const uint8_t *)message->topic, message->topic_len);
    Serial.println();

    Serial.print("Message data: ");
    Serial.write((const uint8_t *)message->data, message->data_len);
    Serial.println();

    // 在此处添加处理接收到的 MQTT 消息的代码
    // 例如，根据 topic 进行分支处理，解析 data，等等
}

static void on_ppp_changed(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "PPP state changed event %d", event_id);
    if (event_id == NETIF_PPP_ERRORUSER)
    {
        /* User interrupted event from esp-netif */
        esp_netif_t *netif = *(esp_netif_t **)event_data;
        ESP_LOGI(TAG, "User interrupted event from netif:%p", netif);
    }
}

static void on_ip_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
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

void my4GInformation(modem_dce_t *dce)
{
    // 确保 dce 对象不为空
    if (dce == NULL)
    {
        ESP_LOGE(TAG, "DCE object is NULL");
        return;
    }

    // 获取运营商名称
    while (!dce->oper || strlen(dce->oper) < 2)
    {
        delay(1000);
        if (dce && strlen(dce->oper) >= 2)
        {

            esp_modem_dce_get_operator_name(dce);
        }
        else
        {
            ESP_LOGW(TAG, "Operator name: %s", dce ? dce->oper : "Unknown");
        }
    }

    // 打印模块信息
    ESP_LOGI(TAG, "Module: %s", dce->name);
    ESP_LOGI(TAG, "Operator: %s", dce->oper);
    ESP_LOGI(TAG, "IMEI: %s", dce->imei);
    ESP_LOGI(TAG, "IMSI: %s", dce->imsi);

    delay(2000);

    // 获取信号质量
    uint32_t rssi = 0, ber = 0;
    esp_err_t ret = dce->get_signal_quality(dce, &rssi, &ber);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get signal quality: %s", esp_err_to_name(ret));
        // 处理错误或返回
    }
    else
    {
        ESP_LOGI(TAG, "RSSI: %d, BER: %d", rssi, ber);
    }

    delay(2000);
    // 获取电池电压
    uint32_t voltage = 0, bcs = 0, bcl = 0;
    esp_err_t ret_battery = dce->get_battery_status(dce, &bcs, &bcl, &voltage);
    if (ret_battery != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get battery status: %s", esp_err_to_name(ret_battery));
        // 可以选择在此处返回或处理错误
    }
    else
    {
        ESP_LOGI(TAG, "Battery voltage: %d mV", voltage);
    }
}

// 在这里放置您的静态函数和其他代码
void initializeModem()
{
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

    my4GInformation(dce);

#if !defined(CONFIG_EXAMPLE_MODEM_PPP_AUTH_NONE) && (defined(CONFIG_LWIP_PPP_PAP_SUPPORT) || defined(CONFIG_LWIP_PPP_CHAP_SUPPORT))
    esp_netif_ppp_set_auth(esp_netif, auth_type, CONFIG_EXAMPLE_MODEM_PPP_AUTH_USERNAME, CONFIG_EXAMPLE_MODEM_PPP_AUTH_PASSWORD);
#endif
    /* attach the modem to the network interface */
    esp_netif_attach(esp_netif, modem_netif_adapter);
    /* Wait for IP address */
    xEventGroupWaitBits(event_group, CONNECT_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

    // 在此处，PPP 连接应该已经建立
    ESP_LOGI(TAG, "PPP connection established");
}

// String my4gModemOperator(modem_dce_t *dce)
String my4gModemOperator(void)
{

    // 确保 dce 对象不为空
    if (dce == NULL)
    {
        ESP_LOGE(TAG, "DCE object is NULL");
        return "";
    }

    // 获取运营商名称
    while (!dce->oper || strlen(dce->oper) < 2)
    {
        delay(1000);
        if (dce && strlen(dce->oper) >= 2)
        {

            esp_modem_dce_get_operator_name(dce);
        }
        else
        {
            ESP_LOGW(TAG, "Operator name: %s", dce ? dce->oper : "Unknown");
        }
    }

    return dce->oper;
}

void initModemMQTT(void)
{    /* Config MQTT */
    esp_mqtt_client_config_t mqtt_config = {
        .event_handle = mqtt_event_handler,
        .uri = BROKER_URL,
    };
    esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_start(mqtt_client);
}

void handleModem()
{
    // 放置处理 4G 模块的代码
    // 例如，检查连接状态，处理网络事件等
}