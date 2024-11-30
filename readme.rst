Install
=======
- Install dependencies: apt-get install xxd git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util 
- Download latest esp-idf-vx.x.x.zip at https://github.com/espressif/esp-idf/releases/ (not source_code.zip)
- Extract zip somewere (/opt/esp-idf) and remember folder.
- call /opt/esp-idf/install.sh
- Add your user to dailout group (usermod -a -G dailout <username>)
  (logout or restart maybe requiered)
- Add this repo and the esp32_nodes repo to your workspace
- Edit rt_esp32/wifi_cfg.h to setup wifi. 


Serial Connection
=================
When the conntroler supports a USB-JTAG-SERIAL interface, this is used for the connection between ABMT and the device.
Otherwise UART0 (GPIO1/GPIO3) is used. You can change this behavior in main/com_device.cpp.

Setup Wifi
==========
Edit the wifi_cfg.h in this directory and add your wifi-login data.

Background
----------
Unfortunatly BLE-Provisioning is very buggy so it was removed an replaces by a configruation header (11.2023: I wasted hours without get it to work on a C6).
To prevent saving your password in an versioned config file, it is stored in a header. This header must define RT_WIFI_SSID and RT_WIFI_PWD.
The location of this header can be changed in your runtime configuration. For advanced usecases you can define define RT_WIFI_* as a function call. 

Low Power Notes
===============
- When light-sleep is enabled, the max cpu fequency is halfed.
- After 5 seconds, when abmt is not connected to the model, ligth-sleep is enabled.
  You have to reset to connect to the model. (Main reason: usb_jtag and flashing)
- Sending Data consumes BY FAR the most power!
- mdns consumes a view few uA (main/main.cpp)
  A empty hostname prevents mdns to be initialized.
- CONFIG_LWIP_TCP_TMR_INTERVAL also consumes a few uA (sdkconfig)
- TWT: wake duration can be changed in main/setup_wifi.cpp
- Wifi sleep modes:
  - min: Modem wakes up for every beacon
  - max / no_itwt: Modem wakes up in listen interval
  - max / itwt_50ms: Modem wakes up every listen interval vor 50ms
  - max / itwt_25ms: Modem wakes up every listen interval vor 25ms
  - depending on your throughput no_itwt might be the better option
  - itwt makes sens for longer intervals with nearly no throughput
  - Long intervals may not work with MDNS
  - Deep sleep makes only sense with very very long sleep intervals
- You can observe the sleeping behavior with connecting a pin to an led and enable the "disable_during_sleep" option.
- Light-sleep disables the native usb connection. 
- lwip cyclic timers 1s: in timeouts.c
  -etharp_tmr
     lwip/src/include/lwip/etharp.h:#define ARP_TMR_INTERVAL 1000
  -dhcp_coarse_tmr
    Set in sdkconfig to 1sec
  -nd6_tmr 1000
     lwip/src/include/lwip/nd6.h:#define ND6_TMR_INTERVAL 1000


Known Issues
============
- For ESP32 Power Management is disabled because it messes around with uart.
- MQTT only works with one connection.
- The ESP-IDF is a good example of collapsing technical dept.

Common Pins
===========
- GPIO 1: ESP32: TX
- GPIO 2: ESP32-C3: Low or floating on bootloader select -> serial bootloader
          Many ESP32 boards: LED
- GPIO 3: ESP32: RX
- GPIO 4: C6: RX
- GPIO 5: C6: TX
- GPIO 8: Boot-Selection on C3/C6; Pulled up externaly and often used for LED;
          On ESP32 this pin is connected to the internal flash
- GPIO 9: ESP32-C3/C6 Bootloader select. Low on reset enters Bootloader. 
- GPIO 0: ESP32 Bootloader select. Low on reset enters Bootloader

Todos
=====
- C6: enable mbed hardware tls when fix for https://github.com/espressif/esp-idf/issues/12528 is released

Change sdkconfig
================
- build project
- go to build directory
- ``$ ./idf menuconfig``
- copy new sdkconfig to rt_esp32/sdkconfigs/<target_cfg>

Options
=======
Important Options
------------------
- disable console
- disable log_color
- partition table csv
- flash size

Optional Options
----------------
- pm_enable
- freertos_hz 1000
- tickless idle 
- flash size 4m
- enable partion via csv 
- optimize for speed (-O2)
- idle_time_before_sleep 2
- CONFIG_ESP_PHY_RF_CAL_FULL=y
- ipv6

Debug 
=====
- udev rules: https://raw.githubusercontent.com/espressif/openocd-esp32/master/contrib/60-openocd.rules
- ``udevadm control --reload-rules && udevadm trigger``
- go to build directory
- ``$ ./idf openocd``
- ``$ ./idf gdbtui``
