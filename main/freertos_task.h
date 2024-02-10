#ifndef FREERTOS_TASK_
#define FREERTOS_TASK_ FREERTOS_TASK_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <abmt/os.h>
#include "abmt_link.h"

struct freertos_task{
	abmt::rt::raster* raster;
	uint32_t raster_index;
	uint32_t interval_ms;
	uint64_t start_ms;
	bool priority_set = false;

    TaskHandle_t handle = NULL;

	freertos_task(abmt::rt::raster* r, uint32_t r_idx):raster(r), raster_index(r_idx){
		interval_ms = raster->interval.ms();
		start_ms = abmt::time::now().ms();
		auto ret = xTaskCreate(
                                freertos_task_main,
                                raster->name,
                                4096,
                                (void*) this,
                                tskIDLE_PRIORITY,
                                &handle 
                              );
        abmt::die_if(ret != pdPASS, "unable to create task");
	}


    static void freertos_task_main(void* arg){
        freertos_task* t = (freertos_task*) arg;
        t->run();
    }


	void run(){
		raster->init();
		if(raster->is_sync == false){
			while(true){
                auto time_to_wait = raster->poll();
                if( time_to_wait > 0 ){
                    vTaskDelay(pdMS_TO_TICKS(time_to_wait.ms()));
                }else{
                    break;
                }
            }
		}
		raster->init_tick();
		lnk->adapter->send_daq(raster_index);
		raster->n_ticks++;
		while(true){
			if(raster->is_sync){
				int32_t sleep_time = (start_ms + raster->n_ticks*interval_ms) - abmt::time::now().ms();
				if(sleep_time >= 0){
                    vTaskDelay(pdMS_TO_TICKS(sleep_time));
				}else{
					raster->n_ticks = (abmt::time::now().ms()-start_ms)/interval_ms;
				}
			}else{
				auto time_to_wait = raster->poll();
				if( time_to_wait > 0 ){
					vTaskDelay(pdMS_TO_TICKS(time_to_wait.ms()));
					continue;
				}
			}
			raster->tick();
			lnk->adapter->send_daq(raster_index);
			raster->n_ticks++;
		}

	}

};

#endif