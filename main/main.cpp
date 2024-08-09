
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_pm.h"
#include "mdns.h"
#include <nvs_flash.h>
#include "esp_log.h"

#include "setup_wifi.h"
#include "abmt_link.h"
#include "single_thread_task.h"

#include "../main.h"
#include "freertos_task.h"
#include "esp_private/esp_clk.h"

abmt_link* lnk; 

extern "C" int esp_log_to_abmt(const char* format, va_list args){
	char buffer[200];
	int size = (int) vsnprintf (buffer,sizeof(buffer),format, args);
	abmt::log({buffer,(unsigned int)size});
	return size;
} 

extern "C" void app_main(void){
    lnk = new abmt_link();

	esp_log_set_vprintf(esp_log_to_abmt);
	esp_log_level_set("*", ESP_LOG_WARN);
	
	esp_pm_config_t pm_cfg = {};
	pm_cfg.min_freq_mhz = esp_clk_xtal_freq() / 1000 / 1000 / 2; // -> 20MHz on most systems
	pm_cfg.max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
	pm_cfg.light_sleep_enable = false;
	esp_pm_configure(&pm_cfg);

	esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        abmt::log("format nvs");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

	lnk->poll();

	if(RT_CFG_ENABLE_WIFI){
		setup_wifi();
		lnk->poll();
		mdns_init();
		mdns_hostname_set(RT_CFG_MDNS_NAME);
	}

	lnk->poll();

    freertos_task** raster_array = new freertos_task*[lnk->model->rasters.length];
	for (size_t raster_index = 0; raster_index < lnk->model->rasters.length; raster_index++) {
		raster_array[raster_index] = new freertos_task(lnk->model->rasters[raster_index], raster_index);
	}

	// set prio for async tasks
	auto async_prio = tskIDLE_PRIORITY;
	for (size_t raster_index = 0; raster_index < lnk->model->rasters.length; raster_index++) {
		if(raster_array[raster_index]->raster->is_sync == false){
			vTaskPrioritySet(raster_array[raster_index]->handle, async_prio);
			raster_array[raster_index]->priority_set = true;
		}
	}
	
	// set prio for sync tasks
	auto next_prio = configMAX_PRIORITIES - 1;
	bool found_raster = true;
	freertos_task* task_to_change = 0;
	while (found_raster){
		found_raster = false;
		auto min_raster_interval = abmt::time::sec(2);
		for (size_t raster_index = 0; raster_index < lnk->model->rasters.length; raster_index++) {
			if(
				   raster_array[raster_index]->raster->is_sync 
				&& raster_array[raster_index]->priority_set == false
				&& raster_array[raster_index]->raster->interval < min_raster_interval
			){
				min_raster_interval = raster_array[raster_index]->raster->interval;
				task_to_change = raster_array[raster_index];
				found_raster = true;
			}
		}
		if(found_raster){
			vTaskPrioritySet(task_to_change->handle, next_prio);
			task_to_change->priority_set = true;
			next_prio--;
			if(next_prio <= tskIDLE_PRIORITY + 1){
				break; // abort
			}
		}
	}

	auto wait_until = abmt::time::now() + abmt::time::sec(5);
    while(wait_until > abmt::time::now()){  	
		lnk->poll();
		vTaskDelay(pdMS_TO_TICKS(10));
    }
	if(lnk->adapter->connected || RT_CFG_SLEEP == false){
		// Don't sleep
		while(true){  	
			lnk->poll();
			vTaskDelay(pdMS_TO_TICKS(10));
    	}
	}else{
		abmt::log("enable sleep");
		lnk->disable();
		pm_cfg.light_sleep_enable = RT_CFG_SLEEP;
		pm_cfg.max_freq_mhz = pm_cfg.max_freq_mhz / 2; // half speed
		esp_pm_configure(&pm_cfg);
		// exit thread
	}
}
