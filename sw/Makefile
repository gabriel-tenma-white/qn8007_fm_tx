DEVICE          = stm32f030f4
OPENCM3_DIR     = /persist/libopencm3
OBJS            += main2.o

CFLAGS          += -Os -g0
CPPFLAGS	+= -Os --std=c++0x -mthumb -fno-exceptions
LDFLAGS         += -static -nostartfiles
LDLIBS          += -Wl,--start-group -lgcc -lnosys -Wl,--end-group

LDSCRIPT = ./stm32f030.ld

include $(OPENCM3_DIR)/mk/genlink-config.mk
include $(OPENCM3_DIR)/mk/gcc-config.mk

.PHONY: clean all

all: binary.elf binary.hex

clean:
	$(Q)$(RM) -rf binary.* *.o

flash: binary.hex
	./st-flash --reset --format ihex write binary.hex


include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk
