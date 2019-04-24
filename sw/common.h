#include <string.h>
inline int intdigits(int i) {
	int ret=0;
	while(i!=0) {
		i/=10;
		ret++;
	}
	return ret;
}
static inline int itoa1(int i, char* b) {
	static char const digit[] = "0123456789";
	char* p = b;
	if (i < 0) {
		*p++ = '-';
		i = -i;
	}
	p += (i == 0 ? 1 : intdigits(i));
	*p = '\0';
	int l = p - b;
	do { //Move back, inserting digits as u go
		*--p = digit[i % 10];
		i = i / 10;
	} while (i);
	return l;
}

inline void wait_cycles( uint32_t n ) {
    uint32_t l = n/3;
    asm volatile( ".syntax unified;" "0:" "SUBS %[count], %[count], #1;" "BNE 0b;" :[count]"+r"(l) );
    asm volatile(".syntax divided");
}

// not working
inline void delay(uint32_t micros) {
	wait_cycles(micros*4);
}

int cat(char* str, int size, int pos, const char* src) {
	int len=strlen(src);
	if(len>(size-pos-1)) len=size-pos-1;
	memcpy(str+pos, src, len);
	*(str+pos+len)=0;
	return pos+len;
}
int cat_itoa(char* str, int size, int pos, int src) {
	if((size-pos-1) < 10) return pos;
	return pos+itoa1(src, str+pos);
}
