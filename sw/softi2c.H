
class softi2c {
public:
	int clk=0,sda=0;
	
	void _delay() {
		delayMicroseconds(1);
		/*asm __volatile__ (
		  "nop\nnop\nnop\nnop\nnop\n"
		);*/
	}

	void scl_low() {
		//digitalWrite(qn8007.clk, 0);
		digitalWrite(clk, 0);
		pinMode(clk, OUTPUT);
	}
	void scl_high() {
		//digitalWrite(qn8007.clk, 1);
		pinMode(clk, INPUT_PULLUP);
	}
	void sda_low() {
		//digitalWrite(qn8007.sda, 0);
		digitalWrite(sda, 0);
		pinMode(sda, OUTPUT);
	}
	void sda_high() {
		//digitalWrite(qn8007.sda, 1);
		pinMode(sda, INPUT_PULLUP);
	}

	void init() {
		_delay();
		digitalWrite(sda, 0);
		_delay();
		digitalWrite(clk, 0);
		_delay();
		
		scl_high();
		_delay();
		sda_high();
		_delay();
	}
	void begin() {
		sda_low();
		_delay();
		scl_low();
		_delay();
	}
	void end() {
		sda_low();
		_delay();
		scl_high();
		_delay();
		sda_high();
		_delay();
		for(int i=0;i<10;i++) _delay();
	}
	void send(uint32_t data, int bits) {
		data <<= (32-bits);
		for(int i=0;i<bits;i++) {
			// put data on bus
			if(data>>31) sda_high();
			else sda_low();
			
			_delay();
			scl_high();
			_delay();
			_delay();
			scl_low();
			_delay();
			
			data <<= 1;
		}
	}
	uint32_t recv(int bits) {
		uint32_t ret = 0;
		sda_high();
		//pinMode(qn8007.sda, INPUT_PULLUP);
		for(int i=0;i<bits;i++) {
			_delay();
			scl_high();
			_delay();
			ret = uint32_t(digitalRead(sda)?1:0) | (ret << 1);
			_delay();
			scl_low();
			_delay();
		}
		//pinMode(qn8007.sda, OUTPUT);
		//_delay();
		return ret;
	}

	bool probe(uint8_t devAddr) {
		begin();
		
		// device address
		send(devAddr, 8);
		// ack bit
		if(recv(1) == 1) {
			end();
			return false;
		}
		end();
		return true;
	}
	// return value: 0: success; -1: no device ack; -2: no register addr ack; -3: no data ack
	int write(uint8_t devAddr, uint8_t addr, uint8_t data) {
		begin();
		
		// device address
		send(devAddr, 8);
		// ack bit
		if(recv(1) == 1) {
			end();
			return -1;
		}
		
		// register address
		send(addr, 8);
		// ack bit
		if(recv(1) == 1) {
			end();
			return -2;
		}
		
		// data
		send(data, 8);
		// ack bit
		if(recv(1) == 1) {
			end();
			return -3;
		}
		
		end();
		return 0;
	}
	// return value: >= 0: the read data; -1: no device ack; -2: no register addr ack
	int read(uint8_t devAddr, uint8_t addr) {
		begin();
		
		// device address
		send(devAddr | 1, 8);
		// ack bit
		if(recv(1) == 1) {
			end();
			return -1;
		}
		
		// register address
		send(addr, 8);
		// ack bit
		if(recv(1) == 1) {
			end();
			return -2;
		}
		
		// data
		int res=recv(8);
		
		end();
		return res;
	}
	
	// return value: >= 0: the read data; -1: no device ack; -2: no register addr ack;
	// -3: no register ack (second transaction)
	int read_si5351(uint8_t devAddr, uint8_t addr) {
		begin();
		// device address
		send(devAddr, 8);
		// ack bit
		if(recv(1) == 1) {
			end();
			return -1;
		}
		
		// register address
		send(addr, 8);
		// ack bit
		if(recv(1) == 1) {
			end();
			return -2;
		}
		end();
		
		// second transaction
		
		begin();
		// device address
		send(devAddr | 1, 8);
		// ack bit
		if(recv(1) == 1) {
			end();
			return -3;
		}
		// data
		int res=recv(8);
		end();
		
		return res;
	}
};
