#include "abmt_link.h"

abmt_link::abmt_link(){
    device = get_com_device();
    adapter = new model_adapter_std;
    model = abmt::rt::get_model();
    adapter->max_def_size = 100;
    adapter->set_model(model);
    adapter->connection.set_source(&in);
    adapter->connection.set_sink(&out);

    last_online_send = abmt::time::now();
    start_time = last_online_send;
}
void abmt_link::disable(){
    adapter->connection.set_source(&dummy);
    adapter->connection.set_sink(&dummy);
    out.flush();
    in.flush();
}

void abmt_link::poll(){
    auto now = abmt::time::now();

    uint32_t bytes_read = 0;
    device->rcv(in.data + in.bytes_used, in.size - in.bytes_used, &bytes_read);
    if (bytes_read > 0) {
        in.bytes_used += bytes_read;
        in.send();
        //nothing_done = false;
    }

    
    if ( (now - last_online_send) > abmt::time::sec(1)) {
        adapter->send_model_online();
        last_online_send = now;
        buffer_error_send = false;
    }
    auto lock = adapter->send_mtx.get_scope_lock();

    if (out.bytes_used > 0) {
        uint32_t bytes_send = 0;
        device->snd(out.data, out.bytes_used, &bytes_send);
        out.pop_front(bytes_send);
        //nothing_done = false;
    }

    if (out.bytes_used > 1000000 / 8 / 100) {
        if (buffer_error_send == false) {
            buffer_error_send = true;
            //nothing_done = false;
            out.flush();
            adapter->clear_daq_lists();
            abmt::log("Error: Data in outbuffer can't be send in 0.01s.");
            abmt::log("Datatransmission stopped. Reduce viewed signals...");
        }
    }
}