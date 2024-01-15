#include <Arduino.h>
#include "my4GFunction.h"
#include <esp_http_client.h>

// 状态标志
// bool isMqttConnected = false;
// bool isMqttSubscribed = true;

void setup(void)
{
    Serial.begin(115200);
    pinMode(16, OUTPUT);
    digitalWrite(16, LOW);
    delay(1000);
    digitalWrite(16, HIGH);
    delay(5000);

    initializeModem();

    if (dce != NULL)
    {
        my4GInformation(dce);
    }

    Serial.print("---");
    Serial.println(my4gModemOperator());

    initModemMQTT();

    // xEventGroupWaitBits(event_group, GOT_DATA_BIT, pdTRUE, pdTRUE, portMAX_DELAY);    //阻塞模式，可能导致别的程序代码使用不了

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
        Serial.println("subscribe To Topic");
        subscribeToTopic("/topic/qos0", 0);
        subscribeToTopic("/topic/qos1", 1);
        subscribeToTopic("/another/topic", 0);
        isMqttSubscribed = false;
    }

    if (!isMqttConnected)
    {
        isMqttSubscribed = true;
    }

    delay(5000);
}