// 18.04.26 - Hostnamen einstellen hinzu
//
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "settings.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "DemoSSID"
#define EXAMPLE_ESP_WIFI_PASS      "DemoPW"
#define EXAMPLE_ESP_MAXIMUM_RETRY  5

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;
char wlan_status_string[256] = "Keine Konfigurationsdaten" ;
int wlan_status=0; //0=> not connected; 1=> connected
char strStatusFehler[] = "Not connected";
char strStatusOk[] = "Connected AP";
char strStatusRetry[] = "WLAN retry";




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
            printf("Bre - WLAN Retry: %i", s_retry_num);
            strcpy(wlan_status_string, strStatusRetry);
            wlan_status=0; //Not connect

        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            printf("Bre - WLAN Connect Fehler: %i", s_retry_num);
            strcpy(wlan_status_string, strStatusFehler);

            wlan_status=0; //Not connect
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        strcpy(wlan_status_string, strStatusOk);
        wlan_status=1; //WLAN connect
        printf("Bre - IP Adresse: " IPSTR, IP2STR(&event->ip_info.ip));


    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    

    // esp_netif_create_default_wifi_sta();
    printf("Hostname: ");
    printf(get_setting("HOSTNAME"));
    printf("\n");
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();

    // Set the hostname for the network interface
    esp_netif_set_hostname(netif, get_setting("HOSTNAME"));
    //



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
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK

            
        },
    };

   /*
    char *ssid = get_setting("SSID");
    char *pwd = get_setting("PWD");
    */
    strlcpy((char*)wifi_config.sta.ssid, get_setting("SSID"), 32);
    strlcpy((char*)wifi_config.sta.password, get_setting("PWD"), sizeof(wifi_config.sta.password));
    ESP_LOGI(TAG, "strncpy finished");
    printf("strncopy finished \n");
    printf(get_setting("SSID"));
    printf("\n");
    printf(get_setting("PWD"));
    printf("\n");
    // printf( "wifi_init_softap finished. SSID:%s  password:%s\n",ssid,pwd);
    if (strlen(get_setting("SSID")) == 0) {
        printf("Keine WLAN Daten\n");
        printf("Textlänge: %i\n", strlen(get_setting("SSID"))); // Ausgabe: Länge SSID
        wlan_status=0; // Keine SSID
        return;
    } 
    
    printf( "wifi_init_ Start \n");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );

    ESP_ERROR_CHECK(esp_wifi_start() );
    esp_err_t ret = esp_wifi_set_max_tx_power(10);
    printf("\nFehler 10 %i\n",ret);
    printf(esp_err_to_name(ret));

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    printf("Bre WIFI INIT Ende 2\n");
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    printf("Bre WIFI INIT Ende 3\n");
}