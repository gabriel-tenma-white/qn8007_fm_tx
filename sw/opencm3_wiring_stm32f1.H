//#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include "usbserial.H"


#define GPIOS_PER_BANK 16


static const uint32_t gpioBanks[5] = {GPIOA,GPIOB,GPIOC,GPIOD,GPIOE};


uint32_t gpioBank(int pin) {
	return gpioBanks[pin/GPIOS_PER_BANK];
}
uint32_t gpioIndex(int pin) {
	return pin % GPIOS_PER_BANK;
}
uint32_t gpioMask(int pin) {
	return uint16_t(1) << (pin % GPIOS_PER_BANK);
}


void pinMode(gpioNumber pin, int mode) {
	uint32_t bank = gpioBank(pin);
	int gpion = pin % GPIOS_PER_BANK;
	if(mode == OUTPUT) {
		gpio_set_mode(bank, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, uint16_t(1) << gpion);
	} else if(mode == INPUT_PULLUP) {
		gpio_set(bank, uint16_t(1) << gpion);
		gpio_set_mode(bank, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, uint16_t(1) << gpion);
	} else {
		gpio_set_mode(bank, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, uint16_t(1) << gpion);
	}
}
void digitalWrite(gpioNumber pin, int bit) {
	uint32_t bank = gpioBank(pin);
	int gpion = pin % GPIOS_PER_BANK;
	if(bit) gpio_set(bank, uint16_t(1) << gpion);
	else gpio_clear(bank, uint16_t(1) << gpion);
}
int digitalRead(gpioNumber pin) {
	int gpion = pin % GPIOS_PER_BANK;
	return (gpio_port_read(gpioBank(pin)) & (uint16_t(1) << gpion)) ? 1 : 0;
}

void usart_configure_pin(uint32_t usart) {
    switch(usart) {
        case USART1:
            rcc_periph_clock_enable(RCC_USART1);
            gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
            break;
        case USART2:
            rcc_periph_clock_enable(RCC_USART2);
            gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART2_TX);
            break;
        case USART3:
            rcc_periph_clock_enable(RCC_USART3);
            gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART3_TX);
            break;
    }
}

void enable_exti_irqs() {
    nvic_enable_irq(NVIC_EXTI0_IRQ);
}
void device_specific_wiringInit() {
	rcc_periph_clock_enable(RCC_AFIO);
    // jtag pins should be used as GPIOs (SWD is used for debugging)
	gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON,0);
}

