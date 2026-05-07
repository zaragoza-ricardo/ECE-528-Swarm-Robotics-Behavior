/* ESPNOW Initator

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

   https://github.com/espressif/esp-idf/tree/c0087486/examples/wifi/espnow

*/

/*
    ESP-NOW configuration for the initiator node.
 
    This implementation uses ESP-NOW to transmit control commands to responder
    robots and verifies delivery using the send callback function.
 
    Additionally, UART pins 16 (RX) and 17 (TX) are used to receive command strings
    from a second ESP32 interfaced with the Xbox Bluetooth controller. These commands 
    are parsed by the initiator and then transmitted to one or both robots using ESP-NOW.
    To support this communication, the ESP-NOW initiator must be configured with the 
    MAC address of each responder so that messages can be directed to specific robots
    or sent to all registered peers.

*/




#include <stdlib.h>
//#include <time.h>
#include <string.h>
//#include <assert.h>
#include "freertos/FreeRTOS.h"
//#include "freertos/semphr.h"
//#include "freertos/timers.h"
#include "nvs_flash.h"
//#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
//#include "esp_crc.h"
#include "espnow_init.h"
#include <esp_err.h>


#include "driver/uart.h"
#include "esp_log.h"


#define CONFIG_ESPNOW_CHANNEL 1


#define CONFIG_ESPNOW_PMK "pmk1234567890123"
#define CONFIG_ESPNOW_LMK "lmk1234567890123"

#define UART_PORT      UART_NUM_1
#define UART_TX_PIN    17       
#define UART_RX_PIN    16
#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE  256

static const char *TAG = "espnow_init";
static const char *TAG_UART = "uart_rx";


static uint8_t robot1_mac[ESP_NOW_ETH_ALEN] = { 0xC0, 0xCD, 0xD6, 0xCA, 0x11, 0x58 }; //Mac address for unicast communication with Robot 1, Peer registration
static uint8_t robot2_mac[ESP_NOW_ETH_ALEN] = { 0x1C, 0xC3, 0xAB, 0xC3, 0x92, 0x50 }; //Mac address for unicast communication with Robot 2, Peer registration




// Confiure UART for PIN 17TX and 16RX
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

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */


static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    ESP_LOGI(TAG, "Send cb to " MACSTR ", status: %d", MAC2STR(mac_addr), status);
}


static esp_err_t example_espnow_init(void)
{



    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    //ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );   //uncommen this line for responder
#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
    ESP_ERROR_CHECK( esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW) );
    ESP_ERROR_CHECK( esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL) );
#endif
    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    esp_now_peer_info_t peer = {0};
    peer.channel = CONFIG_ESPNOW_CHANNEL;
    peer.ifidx = ESPNOW_WIFI_IF;
    peer.encrypt = false;
    memcpy(peer.peer_addr, robot1_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));

    // add robot 2
    memset(&peer, 0, sizeof(peer));
    peer.channel = CONFIG_ESPNOW_CHANNEL;
    peer.ifidx = ESPNOW_WIFI_IF;
    peer.encrypt = false;
    memcpy(peer.peer_addr, robot2_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));

    return ESP_OK;
}


void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    uart_init_simple();  // Initialize UART
    
    // Initialize Wi-Fi and ESP-NOW
    example_wifi_init();
    example_espnow_init();

    char line[UART_BUF_SIZE];
    int line_pos = 0;

while (1)
{
    
    char mode[16];
    uint8_t ch;
    
    //Read UART port
    int n = uart_read_bytes(UART_PORT, &ch, 1, pdMS_TO_TICKS(20));

    if (n > 0)
    {
        if (ch == '\r')
        {
            continue;
        }

        if (ch == '\n')
        {
            line[line_pos] = '\0';

            if (line_pos > 0)
            {
                int robot_id = 0;

    // When scanf() successuflly read 2 values.
    if (sscanf(line, "%15[^,],%d,", mode, &robot_id) == 2)
    {   
        //debug print on the ESP32
        ESP_LOGI(TAG, "Parsed mode=%s robot_id=%d line=%s", mode, robot_id, line);

        if (robot_id == 0)
        {
            esp_err_t err1 = esp_now_send(robot1_mac, (uint8_t *)line, strlen(line) + 1);
            esp_err_t err2 = esp_now_send(robot2_mac, (uint8_t *)line, strlen(line) + 1);

        if (err1 != ESP_OK)
        {
            ESP_LOGE(TAG, "ESP-NOW send to robot 1 failed: %s", esp_err_to_name(err1));
        }
        if (err2 != ESP_OK)
        {
            ESP_LOGE(TAG, "ESP-NOW send to robot 2 failed: %s", esp_err_to_name(err2));
        }
        }
    else if (robot_id == 1)
    {
        esp_err_t err = esp_now_send(robot1_mac, (uint8_t *)line, strlen(line) + 1);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "ESP-NOW send to robot 1 failed: %s", esp_err_to_name(err));
        }
    }
    else if (robot_id == 2)
    {
        esp_err_t err = esp_now_send(robot2_mac, (uint8_t *)line, strlen(line) + 1);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "ESP-NOW send to robot 2 failed: %s", esp_err_to_name(err));
        }
    }
    else
    {
        ESP_LOGW(TAG, "Unknown robot_id: %d, msg=%s", robot_id, line);
    }
}
                else
                {
                    ESP_LOGW(TAG, "Could not parse robot_id from: %s", line);
                }
            }

            line_pos = 0;
        }
        else
        {
            if (line_pos < UART_BUF_SIZE - 1)
            {
                line[line_pos++] = (char)ch;
            }
            else
            {
                line_pos = 0;
            }
        }
    }
}
}