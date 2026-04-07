#include "wlan.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_spi_flash.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "settings.h"
#include "esp_netif.h"
#include <string.h>
#include <strings.h>

#define EXAMPLE_ESP_WIFI_SSID      "WIFISSID"
#define EXAMPLE_ESP_WIFI_PASS      "WIFIPASSWD"
#define EXAMPLE_ESP_MAXIMUM_RETRY  10
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT2

static const char *TAG = "wifi station";

//
char *ssid = "";
char *pwd = "";


char wlan_status_string[256] = "Keine Konfigurationsdaten" ;
int wlan_status=0; //0=> not connected; 1=> connected
char strStatusFehler[] = "Not connected";
char strStatusOk[] = "Connected AP";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
// static esp_ip4_addr_t intIPAdresse;

/* The event group allows multiple bits for each event, but we only care about one event 
 * - are we connected to the AP with an IP? */
// const int WIFI_CONNECTED_BIT = BIT0;

#define TAG "WLAN"

static int s_retry_num = 0;

//This event handler deals with WLAN events and starts the tcp_client_task
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
            wlan_status=0; //Not connect
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
        // snprintf(wlan_status_string, sizeof(wlan_status_string), "connect to the AP fail");
        // strStatus = "Not connected!";
        // strStatus.toArray(wlan_status_string,255) 
        strcpy(wlan_status_string, strStatusFehler);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        printf("Bre - IP Adresse: " IPSTR, IP2STR(&event->ip_info.ip));
        // snprintf(wlan_status_string, sizeof(wlan_status_string), "got ip: %s", IP2STR(&event->ip_info.ip));
        // strStatus = "IP : " + IP2STR(&event->ip_info.ip);
        strcpy(wlan_status_string, strStatusOk);
        wlan_status=1; //WLAN connect
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_LOGI(TAG, "strncpy");
    ssid = get_setting("SSID");
    pwd = get_setting("PWD");
    strlcpy((char*)wifi_config.sta.ssid, ssid, 32);
    strlcpy((char*)wifi_config.sta.password, pwd, sizeof(wifi_config.sta.password));
    ESP_LOGI(TAG, "strncpy finished");
    printf("strncopy finished \n");
    printf(get_setting("SSID"));
    printf("\n");
    printf(get_setting("PWD"));
    printf("\n");
    printf( "wifi_init_softap finished. SSID:%s  password:%s\n",ssid,pwd);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    // esp_wifi_start();
    esp_err_t ret = esp_wifi_set_max_tx_power(10);
    printf("Fehler 10 %i\n",ret);
    printf(esp_err_to_name(ret));

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    printf("\nBre WIFI INIT Ende\n");
    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
                 printf("Bre - WLAN Connected\n");
                 // wlan_status=1;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
                 printf("Bre - WLAN nicht connected\n");
                 // wlan_status=0;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        // wlan_status=0;
    }
    printf("Bre - Start WLAN\n");
    printf("connected to ap SSID:%s password:%s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}