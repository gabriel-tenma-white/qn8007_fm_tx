
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>





static const uint32_t gpioBanks[6] = {GPIOA,GPIOB,GPIOC,GPIOD,GPIOE,GPIOF};


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
		gpio_mode_setup(bank, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, 1<<gpion);
	} else if(mode == INPUT_PULLUP) {
		gpio_mode_setup(bank, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, 1<<gpion);
	} else {
		gpio_mode_setup(bank, GPIO_MODE_INPUT, GPIO_PUPD_NONE, 1<<gpion);
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
	return (uint16_t(GPIO_IDR(gpioBank(pin))) & (uint16_t(1) << gpion)) ? 1 : 0;
}


void usart_configure_pin(uint32_t usart) {
    switch(usart) {
        case USART1:
            rcc_periph_clock_enable(RCC_USART1);
            gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
            gpio_set_af(GPIOA, 1, GPIO2);
            gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO3);
            gpio_set_af(GPIOA, 1, GPIO3);
            break;
    }
}

void enable_exti_irqs() {
    nvic_enable_irq(NVIC_EXTI0_1_IRQ);
    nvic_enable_irq(NVIC_EXTI2_3_IRQ);
    nvic_enable_irq(NVIC_EXTI4_15_IRQ);
}

void device_specific_wiringInit() {
    rcc_periph_clock_enable(RCC_GPIOF);
}


