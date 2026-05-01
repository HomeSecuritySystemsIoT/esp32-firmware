/*
 * soft_ap_sub.c
 *
 *  Created on: 1 maj 2026
 *      Author: Dawid
 */
#include "includes.h"
#include"wifi_common.h"
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
                 //PAR ICI FAUDRA FAIRE LE FAIT D'ENVOYER A UN CAPTIVE PORTAL
   }
   
   
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}




int handler_registered=0;
void wifi_init_softap(void){
	 wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	   switch(wifi_mode){
		case 0:
			ESP_ERROR_CHECK(esp_wifi_init(&cfg));
			wifi_netif=esp_netif_create_default_wifi_ap();
			if(!wifi_netif)
				exit(800);
			wifi_mode=1;
			break;
		case 2:
			esp_wifi_stop();
			esp_wifi_deinit();
			esp_netif_destroy(wifi_netif);
			ESP_ERROR_CHECK(esp_wifi_init(&cfg));
			wifi_netif=esp_netif_create_default_wifi_ap();
			if(!wifi_netif)
				exit(800);
			wifi_mode=1;
			break;
	}
	
	
    
    if(!handler_registered){

    	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
        handler_registered=1;
	}
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                    .required = true,
            },
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
	esp_wifi_set_ps(WIFI_PS_NONE);
    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

