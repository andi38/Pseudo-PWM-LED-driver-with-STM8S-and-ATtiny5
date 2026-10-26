# Pseudo PWM LED driver with STM8S and ATtiny5

One day my LED controller with remote broke. Since near the LED a Raspberry Pi is running I decided instead of buying a new controller to use the Pi‘s PWM. Not long I had to discover that the light was flickering since the Pi is not exactly running an RTOS.

That is how I was about using a so far unused STM8S103 controlled by one of the Pi‘s GPIO: One 20ms pulse means LED off, two pulses mean LED at 50% power, and three pulses command full power to the LED. The output is passed through a MOSFET to the 12V LED strip. The Pi gets its commands from a Zigbee remote pushbutton dimmer I had laying around.

The STM8S code uses two timers: TIM4 to generate pseudo (50% duty cycle) PWM at 150 Hz, TIM2 to facilitate the receive window for up to three pulses from the Pi, both to trigger interrupts.

Since I like the small is beautiful way, once I discovered the ATtiny4/5 at http://www.herbert-dingfelder.de/?page_id=499 in a still hand-solderable SOP23 housing with one timer, 512 bytes flash, and 32 bytes RAM that was the way to go. The ATtiny code uses the one timer and its interrupt for 4ms ticks that result in 125 Hz „PWM“ with again 50% duty cycle. With this timing the input capture is performed as well with the help of another interrupt.

Once playing with the ATtiny the power consumption raised my interest, although not driven by battery at all. I included sending the ATtiny to idle mode, and reducing the system clock to 128 kHz instead of the default 1 MHz.

The current consumption during 10 s averages as follows:

|mode|1 MHz|128 kHz|
|----------|------|------|
|no sleep|372 µA|41 µA|
|sleep (idle)|140 µA|17 µA|

Measurments were done with an STLINK-V3PWR at 3.3 volts and 100 kHz sampling rate and an ATtiny10 I was developing with, since I had no ATtiny4 at hand.


STM8S code was compiled with sdcc and flashed with stm8flash and ST-Link V2 programmer:

$ sdcc -mstm8 stm8-pseudo-pwm.c

$ stm8flash -c stlinkv2 -p stm8s103?3 -w stm8-pseudo-pwm.bin


ATtiny code was compiled with the -Os flag for size optimization with avr-gcc 4.0.0 toolchain I obtained from https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers according to the great help on how to use Fischl usbasp programmer from https://www.mikrocontroller.net/topic/564134 (german, newest firmware, TPIDATA to MOSI):

$ avr-gcc -Os -mmcu=attiny10 -o tiny-pseudo-pwm.elf tiny-pseudo-pwm.c

$ avr-objcopy tiny-pseudo-pwm.elf -O ihex tiny-pseudo-pwm-sleep128kHz-t10.hex

$ avrdude -p t10 -c usbasp -U tiny-pseudo-pwm-sleep128kHz-t10.hex


The ATtiny binary has a size of around 240 bytes depending on the #defines chosen.


Generally the code is in no way rocket science but demonstrates how to use timer and interrupt on venerable 8bit micros.


## Ressources

https://www.st.com/en/microcontrollers-microprocessors/stm8s103-105.html

https://www.microchip.com/en-us/product/ATtiny4

http://www.herbert-dingfelder.de/?page_id=499

https://www.mikrocontroller.net/topic/564134 (german)

https://sdcc.sourceforge.net/

https://github.com/vdudouyt/stm8flash

https://www.microchip.com/en-us/tools-resources/develop/microchip-studio/gcc-compilers
