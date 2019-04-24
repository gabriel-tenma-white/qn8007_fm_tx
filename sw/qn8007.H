
#include "qn8007_defs.h"


class QN8007 {
public:
	int clk=0,mosi=0;
	bool extClk=true;
	
	int lastError=0;
	int prevMode=QND_MODE_TX;

	void _delay() {
		delayMicroseconds(10);
		/*asm __volatile__ (
		  "nop\nnop\nnop\nnop\nnop\n"
		);*/
	}

	void i2c_scl_low() {
		//digitalWrite(qn8007.clk, 0);
		pinMode(clk, OUTPUT);
	}
	void i2c_scl_high() {
		//digitalWrite(qn8007.clk, 1);
		pinMode(clk, INPUT_PULLUP);
	}
	void i2c_sda_low() {
		//digitalWrite(qn8007.mosi, 0);
		pinMode(mosi, OUTPUT);
	}
	void i2c_sda_high() {
		//digitalWrite(qn8007.mosi, 1);
		pinMode(mosi, INPUT_PULLUP);
	}

	void i2c_init() {
		_delay();
		digitalWrite(mosi, 0);
		_delay();
		digitalWrite(clk, 0);
		_delay();
		
		i2c_scl_high();
		_delay();
		i2c_sda_high();
		_delay();
	}
	void i2c_begin() {
		i2c_sda_low();
		_delay();
		i2c_scl_low();
		_delay();
	}
	void i2c_end() {
		i2c_sda_low();
		_delay();
		i2c_scl_high();
		_delay();
		i2c_sda_high();
		_delay();
		for(int i=0;i<10;i++) _delay();
	}
	void i2c_send(uint32_t data, int bits) {
		data <<= (32-bits);
		for(int i=0;i<bits;i++) {
			// put data on bus
			if(data>>31) i2c_sda_high();
			else i2c_sda_low();
			
			_delay();
			i2c_scl_high();
			_delay();
			_delay();
			i2c_scl_low();
			_delay();
			
			data <<= 1;
		}
	}
	uint32_t i2c_recv(int bits) {
		uint32_t ret = 0;
		i2c_sda_high();
		//pinMode(qn8007.mosi, INPUT_PULLUP);
		for(int i=0;i<bits;i++) {
			_delay();
			i2c_scl_high();
			_delay();
			ret = uint32_t(digitalRead(mosi)?1:0) | (ret << 1);
			_delay();
			i2c_scl_low();
			_delay();
		}
		//pinMode(qn8007.mosi, OUTPUT);
		//_delay();
		return ret;
	}

	bool i2c_probe(uint8_t devAddr) {
		i2c_begin();
		
		// device address
		i2c_send(devAddr, 8);
		// ack bit
		if(i2c_recv(1) == 1) {
			i2c_end();
			return false;
		}
		i2c_end();
		return true;
	}
	// return value: 0: success; -1: no device ack; -2: no register addr ack; -3: no data ack
	int i2c_write(uint8_t addr, uint8_t data) {
		i2c_begin();
		
		// device address
		i2c_send(0x56, 8);
		// ack bit
		if(i2c_recv(1) == 1) {
			i2c_end();
			return -1;
		}
		
		// register address
		i2c_send(addr, 8);
		// ack bit
		if(i2c_recv(1) == 1) {
			i2c_end();
			return -2;
		}
		
		// data
		i2c_send(data, 8);
		// ack bit
		if(i2c_recv(1) == 1) {
			i2c_end();
			return -3;
		}
		
		i2c_end();
		return 0;
	}

	int i2c_read(uint8_t addr) {
		i2c_begin();
		
		// device address
		i2c_send(0x57, 8);
		// ack bit
		if(i2c_recv(1) == 1) {
			i2c_end();
			return -1;
		}
		
		// register address
		i2c_send(addr, 8);
		// ack bit
		if(i2c_recv(1) == 1) {
			i2c_end();
			return -2;
		}
		
		// data
		int res=i2c_recv(8);
		
		
		i2c_end();
		return res;
	}

	void QND_WriteReg(uint8_t addr, uint8_t data) {
		int err = i2c_write(addr, data);
		if(err < 0) lastError = err;
	}
	uint8_t QND_ReadReg(uint8_t addr) {
		int ret = i2c_read(addr);
		if(ret < 0) lastError = ret;
		return (uint8_t)ret;
	}
	void QNF_SetRegBit(uint8_t reg, uint8_t bitMask, uint8_t data_val) {
		uint8_t temp;
		temp = QND_ReadReg(reg);
		temp &= (uint8_t)(~bitMask);
		temp |= data_val;
		QND_WriteReg(reg, temp);
	}
	void QND_Delay(int ms) {
		delayMicroseconds(ms*1000);
	}

	
	// returns 0 on success, -error otherwise
	int init() {
		lastError = 0;
		i2c_init();
		delayMicroseconds(5000);
		QND_Init();
		QNF_SetRegBit(SYSTEM1, CHSC, CCA_CH_DIS);
		return lastError;
	}
	
	void QND_Init() {
		QN_ChipInitialization();
		QND_WriteReg(REG_PD2,  MUTE); //mute to avoid noise
		QND_WriteReg(HYSTERSIS, 0xff);	
		QND_WriteReg(MPSTART, 0x12);

		QNF_SetRegBit(GAIN_SEL, 0x38, 0x28);
		QND_WriteReg(REG_PD2,  UNMUTE); //unmute
		QND_WriteReg(00,  0x01);
	}
	void QN_ChipInitialization() {
		QND_WriteReg(0x01,0x89); //reset
		QND_WriteReg(0x02,0x80);
		QND_WriteReg(ANACTL1, 0b00101011); // Set the crystal to 19.2 MHz.
		if(extClk) {
			QND_WriteReg(XLT3, 0x10);
			QND_WriteReg(REG_VGA, 0x3f);
		}
		else {
			QND_WriteReg(XLT3, 0x00);
			QND_WriteReg(REG_VGA, 0x00);
		}
		QND_WriteReg(0x50,0x00);
		QND_WriteReg(0x01,0x49); //recalibrate
		QND_WriteReg(0x01,0x09);
		QND_Delay(400);           //wait more than 400ms
		QND_WriteReg(0x3C,0x89); //for RDS SYNC
		QND_WriteReg(0x4a,0xba);
		QND_WriteReg(0x5c,0x05);  
		QND_WriteReg(0x52,0x0c);  //mute

		QND_WriteReg(0x00,0x81);
		QND_WriteReg(0x57,0x80);
		QND_WriteReg(0x57,0x00);
		QND_Delay(100);           //wait more than 100ms
		QND_WriteReg(0x00,0x01);
		QND_WriteReg(0x52,0x00);
	}
	void QND_SetSysMode(uint16_t mode) {	
		uint8_t val;
		switch(mode)		
		{		
		case QND_MODE_SLEEP:                       //set sleep mode		
			prevMode = QND_ReadReg(SYSTEM1);		
			QNF_SetRegBit(SYSTEM1, R_TXRX_MASK, STNBY); 		
			break;		
		case QND_MODE_WAKEUP:                      //set wakeup mode		
			QND_WriteReg(SYSTEM1, prevMode);		
			break;		
		case QND_MODE_DEFAULT:
			QNF_SetRegBit(SYSTEM2,0x80,0x80);
			break;
		default:	
			val = (uint8_t)(mode >> 8);		
			if (val) {
				QNF_SetRegBit(SYSTEM1, R_TXRX_MASK, val);
			}
			break;
		}	
	}
	uint8_t QNF_SetCh(uint16_t freq) 
	{
		// calculate ch para
		uint8_t tStep;
		uint8_t tCh;
		uint16_t f; 
		f = FREQ2CHREG(freq); 
		// set to reg: CH
		tCh = (uint8_t) f;
		QND_WriteReg(CH, tCh);
		// set to reg: CH_STEP
		tStep = QND_ReadReg(CH_STEP);
		tStep &= ~CH_CH;
		tStep |= ((uint8_t) (f >> 8) & CH_CH);
		QND_WriteReg(CH_STEP, tStep);
		return 1;
	}
};
