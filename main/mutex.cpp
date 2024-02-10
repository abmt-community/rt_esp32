#include <abmt/mutex.h>
#include <abmt/os.h>

using namespace abmt;
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class FreeRTOSMutex {
public:
    FreeRTOSMutex() {
        mutex = xSemaphoreCreateMutex();
    }

    ~FreeRTOSMutex() {
        vSemaphoreDelete(mutex);
    }

    // Lock the mutex
    void lock() {
        if (xSemaphoreTake(mutex, portMAX_DELAY) != pdTRUE) {
            abmt::die("unable to take mutex!");
        }
    }

    // Unlock the mutex
    void unlock() {
        xSemaphoreGive(mutex);
    }

private:
    SemaphoreHandle_t mutex;
};


mutex::mutex(){
	mtx = new FreeRTOSMutex();
}
void mutex::lock(){
	((FreeRTOSMutex*)mtx)->lock();
}
void mutex::unlock(){
	((FreeRTOSMutex*)mtx)->unlock();
}

mutex::~mutex(){
	delete (FreeRTOSMutex*)mtx;
}

scope_lock mutex::get_scope_lock(){
	return scope_lock(*this);
}

scope_lock::scope_lock(mutex& m):m(m){
	m.lock();
}

scope_lock::~scope_lock(){
	m.unlock();
}
