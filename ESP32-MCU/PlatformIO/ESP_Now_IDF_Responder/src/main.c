/* ESPNOW Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

   https://github.com/espressif/esp-idf/tree/c0087486/examples/wifi/espnow
*/

/*

 ESP-NOW responder node. This ESP32 receives command messages from the ESP-NOW initiator
 and forwards them through UART to the robot's local microcontroller.
 In this project, the responder acts as the wireless communication bridge between the 
 initiator ESP32 and the robot controller. In responder mode, the ESP32 uses a receive callback function
 to process incoming ESP-NOW messages from the initiator.

*/
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "espnow_init.h"
#include <esp_err.h>

#include "driver/uart.h" 
#include "esp_log.h"


#define CONFIG_ESPNOW_CHANNEL 1


#define CONFIG_ESPNOW_PMK "pmk1234567890123"


#define UART_PORT      UART_NUM_1
#define UART_TX_PIN    17
#define UART_RX_PIN    16
#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE  256

// Configure UART on pins 17 (TX) and 16 (RX)
static void uart_init_simple(void)
{
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

// Log tags used for ESP-NOW and UART debugging
static const char *TAG = "espnow_responder";
static const char *TAG_UART = "espnow_UART";



/* WiFi should start before using ESPNOW */
static void example_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());      //initialize network interface layer
    ESP_ERROR_CHECK(esp_event_loop_create_default());   // create deaul event loop
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); //create a deafult wifi config struct
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );              // initialize wifi driver
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) ); //sotres WIFI settings in RAM instead of flash
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );     //sets WIFI mode, station mode
    ESP_ERROR_CHECK( esp_wifi_start());                         //start the WIFI subsystem
    ESP_ERROR_CHECK( esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));       //choose channel 1 for radio

//enables Wi-Fi Long Range mode if the config flag is turned on.
#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}

// ESP-NOW receive callback
// Receives wireless messages from the initiator and forwards them over UART
static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    if (recv_info == NULL || recv_info->src_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    char msg[UART_BUF_SIZE];
    int copy_len = len;

    if (copy_len >= sizeof(msg)) {
        copy_len = sizeof(msg) - 1;
    }

    memcpy(msg, data, copy_len);
    msg[copy_len] = '\0';

    ESP_LOGI(TAG, "Received from " MACSTR ": %s",
             MAC2STR(recv_info->src_addr), msg);

    uart_write_bytes(UART_PORT, msg, copy_len);
    uart_write_bytes(UART_PORT, "\n", 1);
}


// Initialize ESP-NOW and register the receive callback
static esp_err_t example_espnow_init(void)
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(example_espnow_recv_cb));
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK));
    return ESP_OK;
}

// Main application entry point
void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    
    uart_init_simple();
    example_wifi_init();
    example_espnow_init();

    while (1) 
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}