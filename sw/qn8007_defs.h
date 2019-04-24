
#define FREQ2CHREG(freq)   (uint32_t(freq-7600)/5)
#define CHREG2FREQ(ch)     (ch*5+7600)

#define QND_MODE_SLEEP      0     
#define QND_MODE_WAKEUP     1
#define QND_MODE_DEFAULT    2
// RX / TX value is using upper 8 bit

#define QND_MODE_RX         0x8000
#define QND_MODE_TX         0x4000


#define R_TXRX_MASK    0xd0
#define R_FMAM_MASK    0xc0


// stereo mode
#define QND_TX_AUDIO_MONO              0x10
#define QND_TX_AUDIO_STEREO            0x00

#define QND_RX_AUDIO_MONO              0x20
#define QND_RX_AUDIO_STEREO            0x00

#define  QND_CONFIG_MONO               0x01
#define  QND_CONFIG_MUTE			   0x02	
#define  QND_CONFIG_SOFTCLIP		   0x03
#define  QND_CONFIG_AUTOAGC			   0x04
#define  QND_CONFIG_AGCGAIN			   0x05	

#define  QND_CONFIG_EQUALIZER		   0x06	
#define  QND_CONFIG_VOLUME			   0x07          
#define	 QND_CONFIG_BASS_QUALITY       0x08
#define  QND_CONFIG_BASS_FREQ		   0x09
#define  QND_CONFIG_BASS_GAIN		   0x0a
#define  QND_CONFIG_MID_QUALITY        0x0b
#define  QND_CONFIG_MID_FREQ           0x0c
#define  QND_CONFIG_MID_GAIN           0x0d
#define  QND_CONFIG_TREBLE_FREQ        0x0e
#define  QND_CONFIG_TREBLE_GAIN        0x0f

#define  QND_ENABLE_EQUALIZER          0x10
#define  QND_DISABLE_EQUALIZER         0x00


#define  QND_CONFIG_AUDIOPEAK_DEV      0x11
#define  QND_CONFIG_PILOT_DEV          0x12
#define  QND_CONFIG_RDS_DEV            0x13

// input format
#define QND_INPUT_ANALOG     0
#define QND_INPUT_IIS        1

// i2s mode
#define QND_I2S_RX_ANALOG   0x00
#define QND_I2S_RX_DIGITAL  0x40
#define QND_I2S_TX_ANALOG   0x00
#define QND_I2S_TX_DIGITAL  0x20

//i2s clock data rate
#define QND_I2S_DATA_RATE_32K  0x00
#define QND_I2S_DATA_RATE_40K  0x10
#define QND_I2S_DATA_RATE_44K  0x20
#define QND_I2S_DATA_RATE_48K  0x30

//i2s clock Bit Wise
#define QND_I2S_BIT_8    0x00
#define QND_I2S_BIT_16   0x40
#define QND_I2S_BIT_24   0x80
#define QND_I2S_BIT_32   0xc0

//i2s Control mode
#define QND_I2S_MASTER   1
#define QND_I2S_SLAVE    0

//i2s Control mode
#define QND_I2S_MSB   0x00
#define QND_I2S_I2S   0x01
#define QND_I2S_DSP1  0x02
#define QND_I2S_DSP2  0x03
#define QND_I2S_LSB   0x04

#define QND_EQUALIZE_BASS	0x00
#define QND_EQUALIZE_MID	0x01
#define QND_EQUALIZE_TREBLE 0x02
// RDS, TMC
#define QND_EUROPE_FLEXIBILITY_DISABLE  0
#define QND_EUROPE_FLEXIBILITY_ENABLE   1
#define QND_RDS_OFF              0
#define QND_RDS_ON               1
#define QND_RDS_BUFFER_NOT_READY 0
#define QND_RDS_BUFFER_READY     1


#define CHIPID_QN8000	0x00
#define CHIPID_QN8005	0x20
#define CHIPID_QN8005B1 0x21
#define CHIPID_QN8006	0x30
#define CHIPID_QN8007B1 0x11
#define CHIPID_QN8007	0x10
#define CHIPID_QN8006A1 0x30
#define CHIPID_QN8006B1 0x31
#define CHIPID_QN8016   0xe0
#define CHIPID_QN8016_1 0xb0
#define CHIPID_QN8015   0xa0
#define CHIPID_QN8065   0xa0
#define CHIPID_QN8067   0xd0
#define CHIPID_QN8065N  0xa0
#define CHIPID_QN8027   0x40
#define CHIPID_QN8025   0x80

#define RDS_INT_ENABLE  1
#define RDS_INT_DISABLE 0
//For antena impedance match
#define QND_HIGH_IMPEDANCE 		1
#define QND_LOW_IMPEDANCE 		0


#define SYSTEM1    0x00
#define SYSTEM2    0x01
#define DEV_ADD    0x02
#define ANACTL1    0x03
#define REG_VGA    0x04
#define CID1       0x05
#define CID2       0x06
#define I2S        0x07
#define CH         0x08
#define CH_START   0x09
#define CH_STOP    0x0A
#define CH_STEP    0x0B
#define PAC_TARGET 0x0C
#define TXAGC_GAIN 0x0D
#define TX_FDEV    0x0E
#define GAIN_TXPLT 0x0F
#define RDSD0      0x10
#define RDSD1      0x11
#define RDSD2      0x12
#define RDSD3      0x13
#define RDSD4      0x14
#define RDSD5      0x15
#define RDSD6      0x16
#define RDSD7      0x17
#define RDSFDEV    0x18
#define CCA        0x19
#define STATUS1    0x1A
#define STATUS3    0x1B
#define RSSISIG    0x1C
#define XLT3       0x49

// for engineering usage
// SYSTEM1   define 16bits macros for AM, RX|FM, TX|FM, RX|AM
#define RXREQ      0x80
#define TXREQ      0x40
#define CHSC       0x20
#define STNBY      0x10
#define RXI2S      0x08
#define TXI2S      0x04
#define RDSEN      0x02
#define CCA_CH_DIS 0x01

// SYSTEM2
#define SWRST      0x80
#define RECAL      0x40
#define FORCE_MO   0x20
#define ST_MO_TX   0x10
#define PETC       0x08
#define RDSTXRDY   0x04
#define TMOUT1     0x02
#define TMOUT0     0x01
// CH         0x08
#define FSTEP         0xc0
#define CH_CH_STOP    0x30
#define CH_CH_START   0x0c
#define CH_CH		  0x03

// CCA
#define TXCCAA      0xe0
#define RXCCAA      0x1f

// RDSFDEV
#define RDS_ONLY    0x80
#define  RDSFDEV_   0x3f
// TXAGC_GAIN 0x0D
#define TX_SFTCLPEN         0x80
#define TAGC_GAIN_SEL       0x40
#define IMR                 0x20
#define TXAGC_GDB           0x10
#define TXAGC_GVGA3         0x08
#define TXAGC_GVGA2         0x04
#define TXAGC_GVGA1         0x02
#define TXAGC_GVGA0         0x01
#define TXAGC_GAINALL       0x0f
// STATUS1
#define I2SOVFL       0x20
#define I2SUNDFL      0x10
#define INSAT         0x08
#define RXAGCSET      0x04
#define RXAGCERR      0x02
#define ST_MO_RX      0x01
// STATUS3
#define RDS_RXUPD     0x80
#define E_DET         0x40
#define RDSC0C1       0x20
#define RDSSYNC       0x10
#define RDS0ERR       0x08
#define RDS1ERR       0x04
#define RDS2ERR       0x02
#define RDS3ERR       0x01
#define RDSERR        0x0f
// PAG_CAL
#define PAG_HD      0x80
#define PAG_DIS     0x40
#define IPOW        0xc0
#define PAGAIN      0x3f

#define MUTE		 0x0c
#define UNMUTE		 0x00
#define RX_MONO_MASK 0x20
#define RSSIMP      0x21
#define PAG_CAL     0x5a
#define GAIN_SEL	0x1f
#define HYSTERSIS	0x3f
#define SMSTART		0x40
#define SNCSTART	0x41
#define HCCSTART	0x42
#define MPSTART		0x43
#define REG_LNA		0x4d
#define REG_PD2		0x52

#define CCOND1		0x38
#define CCA2		0x35
#define CCA4		0x36
#define CCA5        0x37
#define TXAGC5		0x2e
#define _QNSYS_H_

