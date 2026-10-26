// LED out PB0, pulse in PB2
// input pulses active low, 20 ms width, 1..3 pulses for LED off, 50%on, 100%on
// with sleep (idle) mode -> #define SLEEP
// 128 kHz vs. 1 MHz -> #define LSI

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define SLEEP  // use idle mode
#define LSI  // use 128 kHz

volatile uint8_t status = 0b00000000;  // 0b.....mm  mm=mode:00-off,01-100%on,10/11-50%on
volatile uint8_t pulseCtr = 0;
volatile uint8_t pulseDuration = 0;

int main(void) {
// set sys clock to 128 kHz
#ifdef LSI
  CCP = 0xd8;  // enable register change
  CLKMSR = 0b00000001;  // set LSI (128 kHz)
  CCP = 0xd8;  // enable register change
  CLKPSR = 0b00000000;  // set prescaler to 1x
#endif

// init GPIO
  DDRB |= 0b00000001;  // PB0 as output
  // PB2 as input by default

// init SysTick  
#ifndef LSI
  ICR0 = 4000;  // CTC MAX, divide by 4000 => 1 MHz -> 250 Hz SysTick (with 1 MHz SysClk)
#endif
#ifdef LSI
  ICR0 = 512;  // CTC MAX with 128 kHz for 250 Hz SysTick
#endif
//  TCCR0A &= 0b11111100;  // CTC mode bits WGM01/WGM00 (0b00 by default)
  TCCR0B = 0b00011001;  // CTC mode bits WGM03/WGM02; enable timer, no prescaling
  TIMSK0 = 0b00100000;  // enable ICF IRQ

// init PulseIn PB2
  EICRA = 0b00000010;  // falling edge IRQ enable INT0
  EIMSK = 0b00000001;  // unmask INT0

// power safe
  ACSR = 0b10000000;  // power down analog comparator
  PRR = 0b00000010;  // power down ADC
  WDTCSR = 0b00000000;  // shut off watchdog timer
  PUEB = 0b00001010;  // pullup unused GPIO

#ifdef SLEEP
  SMCR = 0b00000000;  // set idle mode as sleep mode
#endif
  
  sei();  // enable IRQ
  
#ifdef SLEEP
  SMCR = 0b00000001;  // enable sleep
#endif
  for(;;) {
#ifdef SLEEP
    __asm__("sleep");  // go to sleep until IRQ
#endif
  }
  return 0;
}

ISR(INT0_vect) {  // capture input pulses INT0 IRQ
  if (pulseCtr == 0) {
    pulseDuration = 0;
  }
  pulseCtr++;
}

ISR(TIM0_CAPT_vect) {  // SysTick every 4 ms TIM9 capture IRQ
// set output
  if (status & 0b00000010) {  // 50%on
    PORTB ^= 0b00000001;  // PB0 toggle 125 Hz
  } else {
    if (status & 0b00000001) PORTB |= 0b00000001;  // 100%on
    else PORTB &= 0b11111110;  // off
  }

// get input pulses and set mode
  pulseDuration++;
  if (pulseDuration >= 50) {  // after 200 ms pulseCtr should be final
    switch (pulseCtr) {
      case 1:  // off
        status = 0b00000000;
        break;
      case 2:  // 50%on
        status = 0b00000010;
        break;
      case 3:  // 100%on
        status = 0b00000001;
        break;
    }
    pulseCtr = 0;
    pulseDuration = 0;
  }
}
