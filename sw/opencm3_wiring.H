//#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>


#define PA 0
#define PB 16
#define PC 32
#define PD 48
#define PE 64
#define PF 80
#define GPIOCONST(i) (i)


#define PA0 GPIOCONST(PA+0)
#define PA1 GPIOCONST(PA+1)
#define PA2 GPIOCONST(PA+2)
#define PA3 GPIOCONST(PA+3)
#define PA4 GPIOCONST(PA+4)
#define PA5 GPIOCONST(PA+5)
#define PA6 GPIOCONST(PA+6)
#define PA7 GPIOCONST(PA+7)
#define PA8 GPIOCONST(PA+8)
#define PA9 GPIOCONST(PA+9)
#define PA10 GPIOCONST(PA+10)
#define PA11 GPIOCONST(PA+11)
#define PA12 GPIOCONST(PA+12)
#define PA13 GPIOCONST(PA+13)
#define PA14 GPIOCONST(PA+14)
#define PA15 GPIOCONST(PA+15)

#define PB0 GPIOCONST(PB+0)
#define PB1 GPIOCONST(PB+1)
#define PB2 GPIOCONST(PB+2)
#define PB3 GPIOCONST(PB+3)
#define PB4 GPIOCONST(PB+4)
#define PB5 GPIOCONST(PB+5)
#define PB6 GPIOCONST(PB+6)
#define PB7 GPIOCONST(PB+7)
#define PB8 GPIOCONST(PB+8)
#define PB9 GPIOCONST(PB+9)
#define PB10 GPIOCONST(PB+10)
#define PB11 GPIOCONST(PB+11)
#define PB12 GPIOCONST(PB+12)
#define PB13 GPIOCONST(PB+13)
#define PB14 GPIOCONST(PB+14)
#define PB15 GPIOCONST(PB+15)

#define PC0 GPIOCONST(PC+0)
#define PC1 GPIOCONST(PC+1)
#define PC2 GPIOCONST(PC+2)
#define PC3 GPIOCONST(PC+3)
#define PC4 GPIOCONST(PC+4)
#define PC5 GPIOCONST(PC+5)
#define PC6 GPIOCONST(PC+6)
#define PC7 GPIOCONST(PC+7)
#define PC8 GPIOCONST(PC+8)
#define PC9 GPIOCONST(PC+9)
#define PC10 GPIOCONST(PC+10)
#define PC11 GPIOCONST(PC+11)
#define PC12 GPIOCONST(PC+12)
#define PC13 GPIOCONST(PC+13)
#define PC14 GPIOCONST(PC+14)
#define PC15 GPIOCONST(PC+15)

#define PF0 GPIOCONST(PF+0)
#define PF1 GPIOCONST(PF+1)
#define PF2 GPIOCONST(PF+2)
#define PF3 GPIOCONST(PF+3)
#define PF4 GPIOCONST(PF+4)
#define PF5 GPIOCONST(PF+5)
#define PF6 GPIOCONST(PF+6)
#define PF7 GPIOCONST(PF+7)
#define PF8 GPIOCONST(PF+8)
#define PF9 GPIOCONST(PF+9)
#define PF10 GPIOCONST(PF+10)
#define PF11 GPIOCONST(PF+11)
#define PF12 GPIOCONST(PF+12)
#define PF13 GPIOCONST(PF+13)
#define PF14 GPIOCONST(PF+14)
#define PF15 GPIOCONST(PF+15)

#define NULL 0
int strlen(const char* c) {
	int len=0;
	while(*c != 0) {
		c++;
		len++;
	}
	return len;
}

//############################################
//############## GPIO FUNCTIONS ##############
//############################################


typedef int gpioNumber;


#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

#define HIGH 1
#define LOW 0



#define GPIOS_PER_BANK 16


//############################################
//############## UTILITY FUNCTIONS ###########
//############################################


volatile int __i=0;
/* 3 Tcyc per tick, 4Tcyc call/ret, 1Tcyc hidden reg assignment */
static void _delay_3t(uint32_t cycles)
{
	/*asm __volatile__ (
		"1: \n"
		"	subs %[cyc],#1 \n"	// 1Tck
		"	bne 1b \n"		// 2Tck
		"	bx lr \n"
		: // No output 
		: [cyc] "r" (cycles)
		: // No memory
	);*/
	
	for(int i=0;i<cycles/2;i++) {
		asm __volatile__ ( "nop" );
	}
}

// to be declared and set by user
extern int cpu_mhz;

void delayMicroseconds_inaccurate(uint32_t us) {
	_delay_3t(us*cpu_mhz/5);
}
// overridable
void (*delayMicroseconds)(uint32_t us) = delayMicroseconds_inaccurate;

void delay(int ms) {
	delayMicroseconds(uint32_t(ms)*1000);
}

static inline int ndigits(int64_t i) {
	int n=0;
	while(i!=0) {
		n++;
		i/=10;
	}
	return (n==0)?1:n;
}
static inline int itoa1(int64_t i, char* b) {
	static char const digit[] = "0123456789";
	char* p = b;
	if (i < 0) {
		*p++ = '-';
		i = -i;
	}
	p += ndigits(i);
	*p = '\0';
	int l = p - b;
	do { //Move back, inserting digits as u go
		*--p = digit[i % 10];
		i = i / 10;
	} while (i);
	return l;
}

void usart_configure_pin(uint32_t usart);

class serial {
public:
	uint32_t usart;
	serial(uint32_t usart):usart(usart) {}
	void begin(int baud) {
		usart_configure_pin(usart);
		usart_set_baudrate(usart, baud);
		usart_set_databits(usart, 8);
		usart_set_stopbits(usart, 0);
		usart_set_mode(usart, USART_MODE_TX_RX);
		usart_set_parity(usart, USART_PARITY_NONE);
		usart_set_flow_control(usart, USART_FLOWCONTROL_NONE);
		
		USART_CR1(usart) |= USART_CR1_RXNEIE | USART_CR1_RE;
		usart_enable(usart);
	}
	void print(const char* s, int len) {
		for(int i=0;i<len;i++)
			usart_send_blocking(usart, s[i]);
	}
	void print(const char* s) {
		int len=strlen(s);
		for(int i=0;i<len;i++)
			usart_send_blocking(usart, s[i]);
	}
	void println(const char* s) {
		print(s);
		usart_send_blocking(usart, '\r');
		usart_send_blocking(usart, '\n');
	}
	void print(int64_t i) {
		char tmp[21];
		itoa1(i, tmp);
		print(tmp);
	}
	void println(int64_t i) {
		print(i);
		usart_send_blocking(usart, '\r');
		usart_send_blocking(usart, '\n');
	}
	void clear() {
		usart_recv(usart);
	}
	int read(char* buf, int bufSize, int timeoutMillis) {
#ifdef USART_ICR
		USART_ICR(usart) = 0xffffffff;
		uint32_t timeout1 = uint32_t(timeoutMillis)*100;
		uint32_t i=0;
		for(int j=0;j<bufSize;j++) {
			while((USART_ISR(usart) & USART_ISR_RXNE) == 0) {
				if((++i) > timeout1) return j;
				delayMicroseconds(10);
			}
			buf[j] = usart_recv(usart);
		}
		return bufSize;
#else
		return 0;
#endif
		
		/*int digitalRead(gpioNumber pin);
	
		int prev = 0;
		int cnt = 0;
		int t = 0;
		while((t++)<100000) {
			int tmp = digitalRead(PA3);
			if(tmp == 0 && prev == 1) cnt++;
			prev = tmp;
		}
		return cnt;*/
	}
};

#ifdef SERIAL_USE_USBSERIAL

//USBSerial Serial;

#else

#ifdef SERIAL_USE_SERIAL
serial Serial(SERIAL_USE_SERIAL);
#else
serial Serial(USART1);
#endif

#endif





//############################################
//############## IRQ FUNCTIONS ###############
//############################################



typedef struct {
	int i;
} irqNumber;

typedef struct {
	exti_trigger_type type;
} irqTrigger;

#define RISING irqTrigger { EXTI_TRIGGER_RISING }
#define FALLING irqTrigger { EXTI_TRIGGER_FALLING }

void (*irqHandlers[GPIOS_PER_BANK]) () = {};

irqNumber digitalPinToInterrupt(gpioNumber pin) {
	return irqNumber {pin};
}
uint32_t gpioBank(int pin);
void enable_exti_irqs();
void attachInterrupt(irqNumber irq, void (*handler) (), irqTrigger trigger) {
	int pin = irq.i;
	uint32_t bank = gpioBank(pin);
	int gpion = pin % GPIOS_PER_BANK;
	
	if(irqHandlers[gpion] != NULL) {
		/*Serial.print("attachInterrupt: pin ");
		Serial.print(gpion);
		Serial.println(" already has interrupt handler");*/
		return;
	}
	
	irqHandlers[gpion] = handler;
	enable_exti_irqs();
	exti_select_source(1 << gpion, bank);
	exti_set_trigger(1 << gpion, trigger.type);
	exti_enable_request(1 << gpion);
}

void handle_exti_irq(int first, int last) {
	uint32_t fired_irqs = exti_get_flag_status(0xffffffff);
	exti_reset_request(fired_irqs);
	for(int i=first; i<last; i++) {
		if(irqHandlers[i] != NULL && (fired_irqs & (1<<i)))
			irqHandlers[i]();
	}
}

void exti0_isr() {
	handle_exti_irq(0, GPIOS_PER_BANK);
}
void exti0_1_isr() {
	handle_exti_irq(0, 2);
}
void exti2_3_isr() {
	handle_exti_irq(2, 4);
}
void exti4_15_isr() {
	handle_exti_irq(4, 16);
}



void device_specific_wiringInit();
void wiringInit() {
	// set sane defaults
	rcc_clock_setup_in_hsi_out_48mhz();
	cpu_mhz = 48;
	
	// enable basic peripherals
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);
	
	
	/*rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_USART1);
	rcc_periph_clock_enable(RCC_USART2);
	rcc_periph_clock_enable(RCC_USART3);//*/
	
	device_specific_wiringInit();
}


