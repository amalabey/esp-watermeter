#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "data_logger.h"
#include "wifi.h"

#define MQTT_BROCKER        CONFIG_ESP_MQTT_BROKER
#define MQTT_USER           CONFIG_ESP_MQTT_USER
#define MQTT_PASSWORD   CONFIG_ESP_MQTT_PASSWORD
#define MQTT_MAX_CONNECTION_ATTEMPTS 10

static bool mqtt_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    printf("Event dispatched from event loop base=%s, event_id=%d \n", base, event_id);
    if(event_id == MQTT_EVENT_CONNECTED)
    {
        printf("MQTT_EVENT_CONNECTED \n");
        mqtt_connected = true;
    }
}

void log_data(const sensor_data_t *data)
{
    connect_wifi();

    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_BROCKER,
        .username = MQTT_USER,
        .password = MQTT_PASSWORD
    };
    mqtt_connected = false;
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);

    // Wait till connected
    printf("Waiting for MQTT Connection..");
    for(int i=0; i<MQTT_MAX_CONNECTION_ATTEMPTS; i++)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if(mqtt_connected)
            break;
        
        printf(".");
    }
    printf("\n");

    char payload[100];
    sprintf(payload, "{\"waterconsumption\":%d}", data->water_consumption);
    
    int msg_id = esp_mqtt_client_publish(client, "home/waterconsumption", payload, 0, 1, 0);
    printf("MQTT message published: %s, id=%d \n", payload, msg_id);
}