#ifndef ABMT_COM_H
#define ABMT_COM_H ABMT_COM_H


#include <abmt/rt/model.h>
#include <abmt/mutex.h>
#include <abmt/io/buffer.h>
#include "model_adapter_std.h"
#include "com_device.h"

struct dummy_sink_source{
    std::function<size_t(abmt::blob&)> on_new_data = [](abmt::blob& b){
        return b.size; // default flush buffer
    };
    void send(const void* data, size_t size){

    }
};

struct abmt_link{
    abmt::rt::model* model;
    model_adapter_std* adapter;
    com_device* device;
    abmt::io::buffer in = abmt::io::buffer(128);
    abmt::io::buffer out = abmt::io::buffer(256);

    dummy_sink_source dummy;

    bool buffer_error_send = false;

    abmt::time last_online_send;
    abmt::time start_time;

	abmt_link();
	void poll();
    void disable();
};

extern abmt_link* lnk; 

#endif // ABMT_COM_H