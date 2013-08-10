Chibios-STM32F051-DISCOVERY-adc-shell-example code
==================================================

This is my example code for the STM32F0 Discovers using the RTOS ChibiOS.

requirements
------------
* Chibios 2.5.0+ (or a recent development snapshot)
* arm toolchain (e.g. arm-none-eabi from summon-arm)

features
--------
* serial console
* ADC measuring, continuous scan
* background blinker thread

usage
-----
* edit the Makefile and point "CHIBIOS = ../../chibios" to your ChibiOS folder
* make
* connect the STM32F0 Discovery with TTL/RS232 adapter, PA2(TX) and PA3(RX) are routed to USART2 38400-8-N-1.
* flash the STM32F0
* connect PC0 to 3.3V and PC1 to GND for analog measurements.
* use your favorite terminal programm to connect to the Serial Port 38400-8n1 

console commands
----------------
* help
* temp
* volt
* ledon
* ledoff
