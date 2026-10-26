// output PD3 only 0%, 50%, 100% necessary -> puls via TIM4 IRQ toggle
// input pulses PA2 active low, 20 ms width, 1..3 pulses for LED off, 50%on, 100%on
// https://www.programmersought.com/article/27937086039/

#include "stm8_103.h"
#include <stdint.h>
#include <stdbool.h>

uint8_t ctr = 0;
bool receive = false;

void InitLED() {
	PD_DDR |= 0b00001000;  // PD3 output
	PD_CR1 |= 0b00001000;  // push-pull
}

// generate pulses for LED with 50% duty cycle
void InitTIM4(void) {
	TIM4_PSCR = 0x07;  // frequency value, 2MHz/2^7 = 15.625kHz
	TIM4_IER = 0x01;  // update interrupt enable
	TIM4_CNTR = 0;  // 255;  // initial value of the counter, 255*(1/15.625K)=0.01632s (ca. 61Hz)
	TIM4_ARR = 50;  // 255;  // automatic reload value 50 -> 153Hz
}

void tim4_isr (void) __interrupt (23) {  // stm8s003 TIM4 update/overflow IRQ
  PD_ODR ^= 0b00001000;  // toggle PD3
	TIM4_SR = 0x00;  // clear interrupt flag
}

void ClearLED() {
 	TIM4_CR1 &= 0b11111110;  // disable TIM4
	PD_ODR &= 0b11110111;  // LED off
}  

void SetLED50() {
 	TIM4_CR1 |= 0x01;  // enable TIM4 (LED 50%)
}

void SetLED100() {
 	TIM4_CR1 &= 0b11111110;  // disable TIM4
	PD_ODR |= 0b00001000;  // LED on
}

void SetLEDMode(uint8_t mode) {  // mode=1 off, mode=2 50%, mode=3 100%
  switch(mode) {
    case 1: ClearLED(); break;
    case 2: SetLED50(); break;
    case 3: SetLED100(); break;
    default: ClearLED(); break;
  }
}

void InitIN() {
	PA_DDR &= 0b11111011;  // PA2 input
	PA_CR1 |= 0b00000100;  // pull-up
  EXTI_CR1 = 0b00000010;  // PA falling edge
	PA_CR2 |= 0b00000100;  // enable IRQ
}

// start receive window with first pulse, count pulses on PA2 (active low)
void input_isr (void) __interrupt (3) {  // stm8s003 EXTI0 (PA) IRQ
  if (!receive) {  // if first pulse start receive window
    receive = true;
    TIM2_CR1 |= 0b00000001;  // enable TIM2
  }
  if (receive) {
    ctr++;
  }
}

// https://blog.junix.in/2018/01/20/stm8s-timer2-with-overflow-interrupt/
void InitTIM2(void) {
  const uint16_t reload_value = 5625;  // 360 ms
  TIM2_PSCR = 0b00000111; //  prescaler = 128
  TIM2_ARRH = reload_value >> 8;
  TIM2_ARRL = reload_value & 0x00FF;
  TIM2_IER |= 0b00000001; // Enable Update Interrupt
}

// receive window ended, set LED according to pulses received
void tim2_isr (void) __interrupt (13) {  // stm8s003 TIM2 update/overflow IRQ
  TIM2_CR1 &= 0b11111110;  // disable TIM2
  receive = false;
  SetLEDMode(ctr);
  ctr = 0;
  TIM2_SR1 &= 0b11111110;  // clear interrupt flag
}

void main(void) {
	__asm__("sim");  // disable IRQ
	InitLED();
	InitTIM4();
  InitIN();
  InitTIM2();
	__asm__("rim");  // enable IRQ
	while(1) {
  }
}
