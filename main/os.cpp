#include <sys/time.h>
#include "abmt_link.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void abmt::log(std::string_view s){
	lnk->adapter->log(s);
}

void abmt::die(std::string_view s){
	abmt::log("Fatal Error:");
	abmt::log(s);
	abmt::log("Reset!");
	vTaskDelay(100);
    esp_restart();
	while(true){};
}

void abmt::die_if(bool condition, std::string_view msg){
	if(condition){
		abmt::die(msg);
	}
}

abmt::time abmt::time::now(){
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
    return abmt::time::sec(tv_now.tv_sec) + abmt::time::us(tv_now.tv_usec);
}