/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */
//#define SERIAL_USE_USBSERIAL

#define PRNT(x)
// Serial.print(x)
#define PRNTLN(x)
// Serial.println(x)


#include "opencm3_wiring.H"
#include "opencm3_wiring_stm32f0.H"
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/adc.h>
#include "qn8007.H"
#include "dfplayer.H"
#include "buttons_manager.H"
#include "flash.H"

int cpu_mhz;

QN8007 qn8007;
int led=PA4;

uint32_t frequency = 8790;
int volume = 24;

uint32_t SONG_MAX_LENGTH_SECONDS = 60*10; // songs longer than 10 minutes will be cut off

//static const uint8_t FLASH_CONFIGURATION_PAGE[1024] __attribute__ ((aligned (1024))) = {};
uint8_t* FLASH_CONFIGURATION_PAGE = (uint8_t*)(0x08000000u+16384-1024);

buttonsManager<4> bm;

volatile uint64_t systemCounter = 0;
volatile uint32_t entropy;
uint8_t ledState = 3; // 0: unlit; 1: lit; 2: temperature monitor blink; 3: do not change
volatile bool shouldSkip = false;
uint16_t songCount = 0;

void handleButtonEvent(buttonEvent event);

bool loadSettings() {
	const uint8_t* cfg = FLASH_CONFIGURATION_PAGE;
	if(!(cfg[0] == 'f' && cfg[1] == 'u' && cfg[2] == 'c' && cfg[3] == 'k'))
		return false;
	frequency = (uint32_t(cfg[4])<<8) | (uint32_t(cfg[5]));
	return true;
}
void saveSettings() {
	uint8_t tmp[] = "fuck....";
	tmp[4] = uint8_t(frequency>>8);
	tmp[5] = uint8_t(frequency);
	
	uint32_t addr = (uint32_t)FLASH_CONFIGURATION_PAGE;
	if(addr < 0x08000000u) addr += 0x08000000u;
	flash_program_data(addr, tmp, 8);
}
// set up system tick interrupt
void timer1_setup() {
	// set the timer to count one tick per us
	rcc_periph_clock_enable(RCC_TIM1);
	rcc_periph_reset_pulse(RST_TIM1);
	timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_set_prescaler(TIM1, cpu_mhz-1);
	timer_set_repetition_counter(TIM1, 0);
	timer_continuous_mode(TIM1);
	
	// BUG(xaxaxa): this doesn't really set the period, but the "autoreload value"; actual period is this plus 1.
	// this should be fixed in libopencm3.
	timer_set_period(TIM1, 32767);
	
	timer_enable_preload(TIM1);
	timer_enable_preload_complementry_enable_bits(TIM1);
	
	timer_enable_irq(TIM1, TIM_DIER_UIE);
	nvic_enable_irq(NVIC_TIM1_BRK_UP_TRG_COM_IRQ);
	
	TIM1_EGR = TIM_EGR_UG;
	
	timer_enable_counter(TIM1);
}

extern "C" void tim1_brk_up_trg_com_isr() {
	TIM1_SR = 0;
	systemCounter++;
	//digitalWrite(led,!digitalRead(led));
	buttonEvent event = bm.doEvents();
	if(event.type != buttonEventType::NONE) handleButtonEvent(event);
}

// wraps around every ~4000 seconds
// can be used for delays up to 2000 seconds
uint32_t micros() {
	return uint32_t(systemCounter)<<15 | uint32_t(TIM1_CNT);
}

uint64_t micros64() {
	return uint64_t(systemCounter)<<15 | uint64_t(TIM1_CNT);
}

uint32_t seconds() {
	//return uint32_t(micros64()/1000000);
	
	// inaccurate but takes less space
	// t_s = t_us/1000000
	// = (t_us/(1024*1024)) * (1.024*1.024)
	// = (t_us/(1024*1024)) * (1 + .048576)
	uint32_t tmp = micros64()/(1024*1024);
	uint32_t scale1 = uint32_t(0.048576 * 16 * 256 + 0.5);
	tmp += (tmp/256) * scale1 / 16;
	return tmp;
}

void delayMicroseconds_timer(uint32_t t) {
	uint32_t t1 = micros();
	while(int32_t(micros()-t1) < int32_t(t));
}


void errorBlink(int cnt) {
	while (1) {
		for(int i=0;i<cnt;i++) {
			digitalWrite(led, HIGH);
			delay(100);
			digitalWrite(led, LOW);
			delay(100);
		}
		delay(1000);
	}
}

// set up temperature monitor timer interrupt
void timer3_setup() {
	uint32_t tim = TIM3; // changing this requires changing code below too
	rcc_periph_clock_enable(RCC_TIM3);
	rcc_periph_reset_pulse(RST_TIM3);
	// set the timer to count one tick per us
	timer_set_mode(tim, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	// timer clock: 1kHz
	// warning: cpu frequency must be < 64MHz
	timer_set_prescaler(tim, (cpu_mhz*1000)-1);
	timer_set_repetition_counter(tim, 0);
	timer_continuous_mode(tim);
	
	// BUG(xaxaxa): this doesn't really set the period, but the "autoreload value"; actual period is this plus 1.
	// this should be fixed in libopencm3.
	timer_set_period(tim, 10); // milliseconds
	
	timer_enable_preload(tim);
	timer_enable_preload_complementry_enable_bits(tim);
	
	timer_enable_irq(tim, TIM_DIER_UIE);
	nvic_enable_irq(NVIC_TIM3_IRQ);
	
	TIM3_EGR = TIM_EGR_UG;
	
	timer_enable_counter(tim);
}




void lfsr_advance(uint64_t& state) {
	if(state==0) state = 0x12345678deadbeef;
	uint32_t state_lower = (uint32_t)state;
	uint32_t bit = state_lower ^ (state_lower>>1) ^ (state_lower>>3) ^ (state_lower>>4);
	bit = (bit) & 1;
	state = (state>>1) | (uint64_t(bit) << 63);
}

uint64_t getEntropy() {
	return entropy;
	/*
	// sample the external clock signal to get entropy
	int clkPin=PF0;
	pinMode(clkPin, INPUT);
	uint64_t ret=0;
	for(int i=0;i<5;i++) {
		uint64_t tmp=0;
		for(int j=0;j<64;j++)
			tmp = (tmp<<1)|digitalRead(clkPin);
		ret = ret ^ tmp;
	}
	return ret;*/
}

struct recentInfo {
	uint16_t songID;
	// when the song will be eligible for playback again,
	// in units of 5 minutes since bootup; e.g. expires=2 means 10 minutes
	// assumed to be always within +/- 32767*5 minutes from current time
	uint16_t expires;
};
static const int recentDepth = 256;
recentInfo recentSongs[recentDepth];
int recentSongsPtr = 0; // the next write address

// wraps around every ~227 days
uint16_t recent_curTime() {
	return uint16_t(seconds()/(60*5));
}
void recent_add(recentInfo inf) {
	recentSongs[recentSongsPtr] = inf;
	recentSongsPtr = (recentSongsPtr+1) % recentDepth;
}
void recent_add(uint16_t songID, uint16_t songLengthSeconds) {
	// calculate how long to inhibit this song for based on its length
	// formula is songLengthSeconds*60 seconds; a N second song will be
	// eligible again after N minutes
	
	uint16_t inhibitFor = songLengthSeconds/5; // units is 5 minutes
	recentInfo inf;
	inf.songID = songID;
	inf.expires = recent_curTime() + inhibitFor;
	recent_add(inf);
}
// returns true if song was recently played and is ineligible
bool recent_check(uint16_t songID) {
	int16_t curTime = recent_curTime();
	for(int i=0;i<recentDepth;i++) {
		recentInfo& tmp = recentSongs[i];
		if(tmp.songID == songID) {
			int16_t timeDiff = int16_t(tmp.expires-curTime);
			if(timeDiff>=0) return true;
		}
	}
	return false;
}

uint16_t calculateTrack(uint64_t lfsrState) {
	return (uint32_t(lfsrState) ^ uint32_t(lfsrState>>32)) % songCount;
}



// if this function returns, an error occured and the dfplayer should be reset
void play_loop() {
	if(songCount < 3) return;
	uint64_t lfsrState=getEntropy();
	lfsr_advance(lfsrState);
	lfsrState = lfsrState ^ getEntropy();
	
	// play songs
	uint32_t trackStartTime_seconds=0;
	uint32_t trackLength_seconds=0;
	uint16_t track = calculateTrack(lfsrState);
	uint16_t prevTrack = uint16_t(-1);
	
	// how many times a random song was picked until an eligible one was found
	int attempts = 1;
	while(1) {
		// play track by number
		dfplayer_sendCommand(0x03, (track>>8), (track&0xff));
		trackStartTime_seconds = seconds();
		
		// set volume
		dfplayer_sendCommand(0x06, volume);
		
		// add previous track to the recent list
		if(prevTrack != uint16_t(-1))
			recent_add(uint16_t(prevTrack), uint16_t(trackLength_seconds));
		
		// blink leds
		if(attempts>10) attempts=10;
		for(int i=0;i<attempts;i++) {
			digitalWrite(led, HIGH);
			delay(50);
			digitalWrite(led, LOW);
			delay(150);
		}
		
		// pick next song
		prevTrack = track;
		attempts = 0;
		do {
			// advance random number generator
			lfsr_advance(lfsrState);
			lfsrState = lfsrState ^ getEntropy();
			
			track = calculateTrack(lfsrState);
			attempts++;
			// if song was played recently pick another one;
			// the currently playing song is also ineligible
		} while((recent_check(track) || track==prevTrack) && attempts < 500);
		
		for(int i=0;i<20;i++) {
			if(shouldSkip) {
				shouldSkip = false;
				goto cont;
			}
			delay(100);
		}
		
		while(1) {
			trackLength_seconds = seconds()-trackStartTime_seconds;
			if(trackLength_seconds > SONG_MAX_LENGTH_SECONDS) goto cont;
			// query status
			switch(dfplayer_isPlaying()) {
				case 0: goto cont;
				case 1: break;
				case 2: return;
				default: break;
			}
			if(shouldSkip) {
				shouldSkip = false;
				goto cont;
			}
		}
	cont: ;
	}
}


uint8_t channel_array[] = {6};
void adc_setup() {
	rcc_periph_clock_enable(RCC_ADC);
	rcc_periph_clock_enable(RCC_GPIOA);

	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO6);

	adc_power_off(ADC1);
	adc_set_clk_source(ADC1, ADC_CLKSOURCE_ADC);
	adc_calibrate(ADC1);
	adc_set_operation_mode(ADC1, ADC_MODE_SCAN);
	adc_disable_external_trigger_regular(ADC1);
	adc_set_right_aligned(ADC1);
	adc_enable_temperature_sensor();
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPTIME_071DOT5);
	adc_set_regular_sequence(ADC1, sizeof(channel_array)/sizeof(*channel_array), channel_array);
	// the adc will wait for the application to read the data register before
	// starting a new conversion
	ADC1_CFGR1 |= ADC_CFGR1_WAIT;
	adc_set_resolution(ADC1, ADC_RESOLUTION_12BIT);
	adc_disable_analog_watchdog(ADC1);
	adc_power_on(ADC1);
}
int32_t readTemperature() {
	int count = 8;
	int32_t tmp = 0;
	int32_t fs = 4096*count;
	for(int i=0; i<count; i++) {
		ADC_CR(ADC1) |= ADC_CR_ADSTART;
		while (!(adc_eoc(ADC1)));
		uint32_t res = adc_read_regular(ADC1);
		tmp += res;
		// collect entropy
		entropy = ((entropy<<1) | (entropy>>31)) ^ res;
	}
	
	int32_t minADC = int32_t(0.15 * fs);
	int32_t maxADC = int32_t(0.5 * fs);
	
	//tmp = (tmp-minADC)*1024/(maxADC-minADC);
	int32_t scale1 = int32_t((1024.*(1024*128))/double(maxADC-minADC));
	tmp = (tmp-minADC)*scale1/(1024*128);
	
	tmp = 1024 - tmp;
	
	
	if(tmp<0) tmp = 0;
	if(tmp>1023) tmp = 1023;
	return tmp;
}



int qn8007_setup() {
	qn8007.clk = PA5;
	qn8007.mosi = PA7;
	int err = qn8007.init();
	if(err < 0) return err;
	
	qn8007.QNF_SetCh(frequency);
	qn8007.QNF_SetRegBit(SYSTEM1, CHSC, CCA_CH_DIS);
	
	//QND_TXSetPower(32);
	
	return 0;
}
void qn8007_enable() {
	qn8007.QND_SetSysMode(QND_MODE_TX);
	qn8007.i2c_write(0x5a, 0b01000000);
}
void qn8007_disable() {
	qn8007.QND_SetSysMode(QND_MODE_SLEEP);
	qn8007.i2c_write(0x5a, 0b01111111);
}


extern "C" void tim3_isr() {
	TIM3_SR = 0;
	
	// sense temperature and handle overtemp shutdown
	int32_t temp = readTemperature();
	static bool overTemp = false;
	if(temp > 1000 && !overTemp) {
		qn8007_disable();
		qn8007_disable();
		overTemp = true;
	}
	if(temp < 950 && overTemp) {
		qn8007_enable();
		overTemp = false;
	}
	
	//return;
	// blink led
	static uint32_t state = 0;
	int ledState1 = 0;
	if(state == 0) {
		state = (1023-temp)/8 + 10;
		ledState1 = 1;
	} else if(state<10) {
		ledState1 = 1;
	} else {
		ledState1 = 0;
	}
	state--;
	if(state > 100) state = 100;
	if(ledState == 2)
		digitalWrite(led,ledState1);
	else if(ledState == 0 || ledState == 1)
		digitalWrite(led,ledState);

	//digitalWrite(led,!digitalRead(led));
}

int b_back_pin = PA1;
int b_left_pin = PA0;
int b_right_pin = PF1;
int b_enter_pin = PF0;
enum {
	b_back=0,
	b_left,
	b_right,
	b_enter
};

void buttons_setup() {
	bm.buttonPins[0] = b_back_pin;
	bm.buttonPins[1] = b_left_pin;
	bm.buttonPins[2] = b_right_pin;
	bm.buttonPins[3] = b_enter_pin;
	bm.init();
	for(int i=0;i<4;i++)
		pinMode(bm.buttonPins[i], INPUT_PULLUP);
}

int menuState = 0;
enum {
	MENU_HOME,
	MENU_FREQSET
};

void ui_changeFreq(bool down, bool roundToInt) {
	if(roundToInt) {
		if(down) frequency = uint32_t(uint32_t(frequency)/100)*100;
		else frequency = uint32_t(uint32_t(frequency+99)/100)*100;
	} else {
		if(down) frequency -= 10;
		else frequency += 10;
	}
	if(frequency < 8700) frequency = 8700;
	if(frequency > 10800) frequency = 10800;
	qn8007.QNF_SetCh(frequency);
	saveSettings();
}
void handleButtonEvent(buttonEvent event) {
	if(event.type == buttonEventType::DOWN) {
		if(menuState == MENU_FREQSET) {
			if(event.b1 == b_left) ui_changeFreq(true, false);
			if(event.b1 == b_right) ui_changeFreq(false, false);
		}
	} else if(event.type == buttonEventType::CLICK) {
		if(menuState == MENU_FREQSET) {
			if(event.b1 == b_back) {
				menuState = MENU_HOME;
				ledState = 2;
			}
		}
	} else if(event.type == buttonEventType::DBLCLICK) {
		if(menuState == MENU_HOME) {
			if(event.b1 == b_right)
				shouldSkip = true;
		}
		if(menuState == MENU_FREQSET) {
			if(event.b1 == b_left) ui_changeFreq(true, true);
			if(event.b1 == b_right) ui_changeFreq(false, true);
		}
	} else if(event.type == buttonEventType::MULTICLICK) {
		if(event.b1 == b_back && event.b2 == b_enter) {
			if(menuState == MENU_HOME) {
				menuState = MENU_FREQSET;
				ledState = 1;
			} else {
				menuState = MENU_HOME;
				ledState = 2;
			}
		}
	}
}

int main(void)
{
	int i;
	wiringInit();
	loadSettings();
	
	
	pinMode(led,OUTPUT);
	digitalWrite(led, HIGH);
	
	Serial.begin(9600);
	
	PRNTLN("asdf");
	delay(100);
	digitalWrite(led, LOW);

	// PA bias potentiometer
	pinMode(PA10, OUTPUT);
	digitalWrite(PA10, HIGH);
	
	buttons_setup(); // initialize buttons state machine
	timer1_setup(); // system tick & buttons poll
	//delayMicroseconds = delayMicroseconds_timer;
	
	adc_setup(); // temperature sensor adc
	
	
	if(qn8007_setup() < 0) {
		while (1) {
			for(int i=0;i<3;i++) {
				digitalWrite(led, HIGH);
				delay(300);
				digitalWrite(led, LOW);
				delay(300);
			}
			delay(2000);
		}
	}
	//qn8007_disable();
	
	bool qn8007_started = false;
	
	ledState = 3;
	if(!qn8007_started) {
		qn8007_started = true;
		// set up overtemp monitor irq
		timer3_setup();
		// enable tx
		qn8007_enable();
	}

rst:
	{
		// reset
		dfplayer_sendCommand(0x0c, 0x00);
		delay(1500);
		
		dfplayer_sendCommand(0x09, 0x01);
		/*dfplayer_sendCommand(0x03, 0x02);
		dfplayer_sendCommand(0x11, 0x00);
		delay(100);
		dfplayer_sendCommand(0x18, 0x00);*/
		
		// query tf card song count
		dfplayer_sendCommand(0x48, 0x00);
		
		uint16_t tmp;
		if(!dfplayer_receiveInt(tmp, 2000)) {
			for(int i=0;i<3;i++) {
				digitalWrite(led, HIGH);
				delay(100);
				digitalWrite(led, LOW);
				delay(100);
			}
			delay(1000);
			goto rst;
		}
		if(songCount == 0) songCount = tmp;
		
		
		
	
		
		// blink leds based on the total number of songs
		digitalWrite(led, HIGH);
		delay(500);
		digitalWrite(led, LOW);
		delay(500);
		
		int blinks = songCount/100;
		if(blinks > 10) blinks = 10;
		for(int i=0;i<blinks;i++) {
			digitalWrite(led, HIGH);
			delay(100);
			digitalWrite(led, LOW);
			delay(100);
		}
		delay(500);
		
		if(!qn8007_started) {
			qn8007_started = true;
			// enable tx
			qn8007_enable();
			// set up overtemp monitor irq
			timer3_setup();
		}
		
		ledState = 2;
		play_loop();
		ledState = 3;
		goto rst;
	}
}

extern "C" void abort() {
	while (1) {
		for(int i=0;i<3;i++) {
			digitalWrite(led, HIGH);
			delay(100);
			digitalWrite(led, LOW);
			delay(100);
		}
		delay(1000);
	}
}

extern "C" void *memcpy(char *dest, const char *src, uint32_t n) {
	for(int i=0;i<n;i++) dest[i] = src[i];
	return dest;
}
