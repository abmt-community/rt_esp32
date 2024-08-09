#include "com_device.h"
#include "sdkconfig.h"

#ifdef CONFIG_SOC_USB_SERIAL_JTAG_SUPPORTED
#include "driver/usb_serial_jtag.h"
#include "hal/usb_serial_jtag_ll.h"
#endif

#include "driver/uart.h"
#include "driver/gpio.h"

#include <functional>
#include "os.h"
#include <abmt/io/buffer.h>
#include <abmt/mutex.h>
#include "../main.h"

struct dummy_com: public com_device{
	dummy_com(){

	}

	virtual bool connected(){
		return false;
	}

	virtual void rcv(void* buff, uint32_t buff_size, uint32_t* bytes_received){
		*bytes_received = 0;
	}

	virtual void snd(void* buff, uint32_t len, uint32_t* bytes_send){
		*bytes_send = len;
	}

	virtual ~dummy_com(){

	}
};

#ifdef CONFIG_SOC_USB_SERIAL_JTAG_SUPPORTED
struct esp32_jtag_serial: public com_device{

	esp32_jtag_serial(){
		usb_serial_jtag_driver_config_t cfg = {};
		cfg.tx_buffer_size = 64;
		cfg.rx_buffer_size = 128;
    	usb_serial_jtag_driver_install(&cfg);
    	// fix hanging console after flash
    	usb_serial_jtag_ll_txfifo_flush();
	}

	virtual void rcv(void* buff, uint32_t buff_size, uint32_t* bytes_received){
		*bytes_received = usb_serial_jtag_read_bytes(buff, buff_size, 0);
	}

	virtual void snd(void* buff, uint32_t len, uint32_t* bytes_send){
		if(len > 64){
			len = 64;
		}
		*bytes_send = usb_serial_jtag_write_bytes(buff, len, pdMS_TO_TICKS(2));
		usb_serial_jtag_ll_txfifo_flush();
	}
};
#endif

struct esp32_uart: public com_device{
	esp32_uart(){
	    uart_config_t cfg = {};
		cfg.baud_rate = RT_CFG_SERIAL_BAUDRATE;
		cfg.data_bits = UART_DATA_8_BITS;
		cfg.parity    = UART_PARITY_DISABLE;
		cfg.stop_bits = UART_STOP_BITS_1;
		cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
			
		ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 256, 256, 0, NULL, 0));
		ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &cfg));
		ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, GPIO_NUM_1, GPIO_NUM_3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	}

	virtual void rcv(void* buff, uint32_t buff_size, uint32_t* bytes_received){
		*bytes_received = uart_read_bytes(UART_NUM_0, buff, buff_size, 0);
	}

	virtual void snd(void* buff, uint32_t len, uint32_t* bytes_send){
		if(len > 256){
			len = 256;
		}

		int bs = uart_tx_chars(UART_NUM_0, (const char *) buff, len);
		if(bs > 0){
			*bytes_send = bs;
		}
	}

};


com_device* get_com_device(){
	#if RT_CFG_SERIAL_DEVICE == 0
		#if RT_CFG_SLEEP == true
			#error "USB/JTAG communication and light-sleep do not work together."
		#endif
		#ifdef CONFIG_SOC_USB_SERIAL_JTAG_SUPPORTED
			static esp32_jtag_serial device; // constructor called at first call. Important for USB.
		#else
			static esp32_uart device;
		#endif
	#elif RT_CFG_SERIAL_DEVICE == 1
		static esp32_uart device;
	#else
		static dummy_com device;
	#endif
	return &device;
}
