
uint16_t dfplayer_checksum (uint8_t* data, int dataLen) {
	uint16_t sum = 0;
	for (int i=0; i<dataLen; i++) {
		sum += data[i];
	}
	return -sum;
}
void dfplayer_sendCommand(uint8_t cmd, uint8_t arg1, uint8_t arg2) {
	delay(100);
	uint8_t buf[11] = "\x7e\xff\x06\x00\x00\x00\x00\x00\x00\xef";
	buf[3] = cmd;
	buf[5] = arg1;
	buf[6] = arg2;
	uint16_t checksum = dfplayer_checksum(buf+1, 6);
	buf[7] = uint8_t(checksum>>8);
	buf[8] = uint8_t(checksum);
	Serial.print((char*)buf, 10);
}
// result must be of size 2
bool dfplayer_receive(uint8_t* result, int timeoutMillis) {
	char buf[10];
	int br = Serial.read(buf, sizeof(buf), timeoutMillis);
	/*
	digitalWrite(led, HIGH);
	delay(500);
	digitalWrite(led, LOW);
	delay(500);
	for(int i=0;i<br;i++) {
		digitalWrite(led, HIGH);
		delay(100);
		digitalWrite(led, LOW);
		delay(100);
	}*/
	
	if(br < sizeof(buf)) return false;
	if(buf[3] == 0x40) {
		PRNT("received error code from dfplayer");
		return false;
	}
	result[0] = buf[5];
	result[1] = buf[6];
	return true;
}
bool dfplayer_receiveInt(uint16_t& result, int timeoutMillis) {
	Serial.clear();
	uint8_t buf[2];
	if(!dfplayer_receive(buf, timeoutMillis)) return false;
	result = (uint16_t(buf[0]) << 8) | uint16_t(buf[1]);
	return true;
}

void dfplayer_sendCommand(uint8_t cmd, uint8_t arg) {
	dfplayer_sendCommand(cmd, 0, arg);
}

// returns: 0 if not playing; 1 if playing; 2 if error
int _dfplayer_isPlaying() {
	dfplayer_sendCommand(0x42, 0);
	uint16_t tmp;
	if(!dfplayer_receiveInt(tmp, 500)) return 2;
	return (tmp==513)?1:0;
}
// returns: 0 if not playing; 1 if playing; 2 if error
int dfplayer_isPlaying() {
redo:
	int tmp = _dfplayer_isPlaying();
	if(tmp == 1) return tmp;
	for(int i=0;i<5;i++) {
		if(_dfplayer_isPlaying() != tmp) goto redo;
	}
	return tmp;
}
