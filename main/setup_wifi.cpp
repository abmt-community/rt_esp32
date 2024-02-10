#include "setup_wifi.h"
#include "../main.h"
#include <abmt/os.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_wifi_he.h"
#include "esp_sleep.h"

#include "abmt_link.h"


#define WIFI_CONNECTED_BIT 1
#define WIFI_FAIL_BIT      2

volatile bool wifi_connected = false;
bool wifi_was_connected = false;
bool twt_request_send = true;
uint8_t twt_counter = 0;
EventGroupHandle_t wifi_event_group;
TaskHandle_t wifi_con_mrg_handle = NULL;

void setup_wifi_itwt();

extern "C" {
void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        abmt::log("connecting to wifi...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifi_connected = true;
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        //ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        char ipstr[16];
        snprintf(ipstr,sizeof(ipstr),IPSTR,IP2STR(&event->ip_info.ip));
        abmt::log("connected :) my ip: " + std::string(ipstr)); 
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
} //extern c
 


void wifi_con_mrg(void* arg){
    while(true){
            xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(15*1000)); // wait 15 sec

            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

            if(wifi_connected && twt_request_send == false){
                // setup twt 15 sec after connect for higher throughput at the begin
                twt_request_send = true;
                setup_wifi_itwt();
            }
            if(wifi_connected && wifi_was_connected == false){
                wifi_was_connected = true;
                twt_request_send = false;
            }else if(wifi_connected == false){
                if(wifi_was_connected){
                    wifi_was_connected = false;
                    abmt::log("AP disconnected. Retry to connect to wifi...");
                }else{
                    abmt::log("Connecting to wifi failed. Retry in 5s...");
                    vTaskDelay(pdMS_TO_TICKS(1) * 5 * 1000); // 10sec
                }
                esp_wifi_connect();
            }
            
            lnk->poll();
    } // while
}


void setup_wifi(){
    wifi_connected = false;
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));


    wifi_config_t wifi_config = {};
    if(RT_CFG_WIFI_SLEEP != 0){
        wifi_config.sta.listen_interval = RT_CFG_WIFI_INTERVAL / 100; // beacon listeninterval has a unit of 100ms
        // limit listen_interval to 5s
        if(wifi_config.sta.listen_interval > 50){
            wifi_config.sta.listen_interval = 50;
        }
    }
    
    std::string ssid = RT_WIFI_SSID;
    std::string pwd = RT_WIFI_PWD;
    strncpy((char*)wifi_config.sta.ssid,ssid.c_str(),sizeof(wifi_config.sta.ssid) -1);
    strncpy((char*)wifi_config.sta.password,pwd.c_str(),sizeof(wifi_config.sta.password) -1);

    if(ssid == "ssid"){
       abmt::log("Wifi not configured. Modify wifi_cfg.h in runtime directory.");
       return;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());


    auto ret = xTaskCreate(wifi_con_mrg, "wifi_con_mrg", 2048, NULL, tskIDLE_PRIORITY, &wifi_con_mrg_handle );
    abmt::die_if(ret != pdPASS, "unable to create task");

    if(RT_CFG_WIFI_SLEEP == 0){
        esp_wifi_set_ps(WIFI_PS_NONE);
    }else if(RT_CFG_WIFI_SLEEP == 1){
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    }else if(RT_CFG_WIFI_SLEEP == 2){
        esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    }else if(RT_CFG_WIFI_SLEEP == 3){
        esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    }
}

void setup_wifi_itwt(){
    #ifdef CONFIG_SOC_WIFI_HE_SUPPORT
        wifi_phy_mode_t phymode;
        ESP_ERROR_CHECK(esp_wifi_sta_get_negotiated_phymode(&phymode));
        bool enable_listen_interval = false;
        uint8_t wake_duration = 50; 
        if(RT_CFG_WIFI_SLEEP == 4){
            wake_duration = 25;
        }
        if (phymode == WIFI_PHY_MODE_HE20 && RT_CFG_WIFI_INTERVAL > 0 && RT_CFG_WIFI_SLEEP >= 3) {
            wifi_twt_setup_config_t setup_config = {};
            setup_config.setup_cmd = TWT_REQUEST;
            setup_config.flow_id = 0;
            setup_config.twt_id = 0;
            setup_config.flow_type = 1; // 1 = Announced ( the AP that device is ready for new data); 0 = unannounced
            setup_config.min_wake_dura = wake_duration; // unit ms
            setup_config.wake_duration_unit = 1; // 0: 256us 1: 1024us -> 1ms
            // interval calculation: wake_invl_mant*2**wake_invl_expn in us
            setup_config.wake_invl_expn = 10; // unit_ us [0-31]; 2**10 -> 1024 -> 1ms
            setup_config.wake_invl_mant = RT_CFG_WIFI_INTERVAL*1000LL/1024; // uint us [1-65535]
            setup_config.trigger = 1;
            setup_config.timeout_time_ms = 5000; // unit us [100-65535]
            esp_err_t err = esp_wifi_sta_itwt_setup(&setup_config);
            if (err != ESP_OK) {
                abmt::log("unable to setup itwt");
                enable_listen_interval = true;
            }
        }
    #endif
}
