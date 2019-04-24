
enum class buttonEventType: uint8_t {
	NONE=0,
	DOWN,
	CLICK,
	DBLCLICK,
	MULTICLICK
};
struct buttonEvent {
	buttonEventType type;
	uint8_t b1,b2; // always in ascending order (b1 < b2)
};
template<int nButtons>
class buttonsManager {
public:
	int buttonPins[nButtons];
	int clickDelay = 6;
	
	// states:
	// 0: button up
	// 255: button down, pending click event
	// 254: button down, no click event
	// 1-63: button released, pending click event
	uint8_t bState[nButtons];
	
	void init() {
		for(int i=0; i<nButtons; i++) bState[i] = 0;
	}
	
	// should be called from timer interrupt every 32 ms
	buttonEvent doEvents() {
		for(int i=0; i<nButtons; i++) {
			bool state = readButton(i);
			
			if(bState[i] == 0) { // button up
				if(state) {
					bState[i] = 255;
					return {buttonEventType::DOWN, (uint8_t)i, 0};
				}
			} else if(bState[i] < 64) { // button up, pending click
				if((--bState[i]) == 0) {
					return {buttonEventType::CLICK, (uint8_t)i, 0};
				}
				if(state) {
					bState[i] = 254;
					return {buttonEventType::DBLCLICK, (uint8_t)i, 0};
				}
			} else { // button down
				if(!state) { // button released
					if(bState[i] == 255) {
						bState[i] = clickDelay;
						
						// check for other depressed buttons
						int otherButton = -1;
						for(int j=0; j<nButtons; j++) {
							if(j!=i && (bState[j] == 255)) {
								otherButton = j;
								bState[j] = 254;
							}
						}
						// this button is released with another button still depressed
						if(otherButton != -1) {
							bState[i] = 0;
							buttonEvent evt {buttonEventType::MULTICLICK, (uint8_t)i, (uint8_t)otherButton};
							if(evt.b2 < evt.b1) {
								uint8_t tmp = evt.b2;
								evt.b2 = evt.b1;
								evt.b1 = tmp;
							}
							return evt;
						}
					} else bState[i] = 0;
				}
			}
		}
		return {buttonEventType::NONE, 0, 0};
	}
	// returns whether button i is depressed
	bool readButton(int i) {
		return digitalRead(buttonPins[i]) == 0;
	}
};
