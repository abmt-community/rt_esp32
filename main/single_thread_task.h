/**
 * Author: Hendrik van Arragon, 2022
 * SPDX-License-Identifier: MIT
 */

#ifndef RT_SINGLE_THREAD_TASK_H_
#define RT_SINGLE_THREAD_TASK_H_

#include <cstdint>
#include <abmt/time.h>
#include <abmt/rt/raster.h>
#include "model_adapter_std.h"

struct single_thread_task {
	abmt::rt::raster *raster;
	uint32_t raster_index;
	model_adapter_std* adapter;
	abmt::time next_run;
	bool init_tick_executed = false;

	single_thread_task(abmt::rt::raster* r, uint32_t r_idx, model_adapter_std* a):
			raster(r), raster_index(r_idx), adapter(a)
	{
		next_run = abmt::time::now();
		raster->init();
	}

	// returns true if the raster was executed
	void run() {
		if (raster->is_sync) {
			next_run = abmt::time::now() + raster->interval;
		} else {
			abmt::time sleep_time = raster->poll();
			if (sleep_time > 0) {
				if(sleep_time > 1){
					next_run = abmt::time::now() + sleep_time;
				}else{
					// ignore next_run-update when the next poll should occur
					// in one nanosecond. This improves latency for platforms
					// with low clock resolutions
				}
				return;
			}
		}
		if (init_tick_executed) {
			raster->tick();
		} else {
			raster->init_tick();
			init_tick_executed = true;
		}

		adapter->send_daq(raster_index);
		raster->n_ticks++;
	}
};

#endif /* RT_SINGLE_THREAD_TASK_H_ */
