#ifndef COM_DEVICE_H_
#define COM_DEVICE_H_

#include <cstdint>

struct com_device{
	virtual void rcv(void* buff, uint32_t buff_size, uint32_t* bytes_received) = 0;
	virtual void snd(void* buff, uint32_t len, uint32_t* bytes_send) = 0;
	virtual void disable(){}
	virtual ~com_device(){};
};

com_device* get_com_device();

#endif
