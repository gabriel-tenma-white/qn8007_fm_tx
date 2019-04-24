# qn8007_fm_tx
FM transmitter based on QN8007 with MP3 player and PA

* USB powered
* Uses DFPlayer MP3 player module (available from aliexpress)
* 1W PA using RQA0009
* Integrated output lowpass filter

### Building the software
* Download and build libopencm3 (https://github.com/libopencm3/libopencm3)
* Build:
```bash
cd qn8007_fm_tx/sw
export OPENCM3_DIR=/PATH/TO/LIBOPENCM3
make
```
* Download and build st-link software (https://github.com/texane/stlink)
* Flash device (using ST-Link, available from aliexpress)
```bash
cd qn8007_fm_tx/sw
rm st-flash
ln -s /PATH/TO/STLINK/build/Release/st-flash ./

./st-flash --reset --format ihex write binary.hex
# or 'make flash'
```


### PCB stackup
* 4 layers
* FR4
* 1.5mm total thickness
* 0.2mm top/bottom prepreg thickness

### Top view

![picture](pic_top.jpg)

### Bottom view

![picture](pic_bottom.jpg)

---
To open schematics, it is necessary to add all gEDA symbols here to your symbol library: https://github.com/gabriel-tenma-white/sym

To edit PCB layouts, make sure "packages" is a symlink to a cloned repository of: https://github.com/gabriel-tenma-white/packages2
