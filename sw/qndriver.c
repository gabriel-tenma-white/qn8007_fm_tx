#include "qndriver.h"

//extern UINT8 QND_ReadReg(UINT8 adr);
//extern UINT8 QND_WriteReg(UINT8 adr, UINT8 value);

#define RSSINTHRESHOLD 4
UINT8  clearscanflag = 0;
UINT16 clearchannel = 0;
UINT8  firstscan = 1;

#define R_TXRX_MASK    0xd0
#define R_FMAM_MASK    0xc0
UINT8  S_XDATA  RSSIn = 255;
UINT8  S_XDATA  Rssinarray[4];
UINT8  S_XDATA  prevMode;
UINT16 S_XDATA  prevVal;
UINT8  S_XDATA  qnd_Country  = COUNTRY_CHINA ;
UINT16 S_XDATA  QND_CH_START = 7600;
UINT16 S_XDATA  QND_CH_STOP  = 10800;
UINT8  S_XDATA  QND_CH_STEP  = 1;
UINT8 S_XDATA qnd_Band;

UINT8  S_XDATA autoScanAll;
UINT8  S_XDATA isStereo;
UINT8  S_XDATA chCount;
UINT16 S_XDATA chList[QN_CCA_MAX_CH];
UINT8  S_XDATA steparray[6]={5,10,20,9,10,1};
QND_SeekCallBack callbackfunc = 0;

UINT8 S_XDATA chumby_XCLK = 1;



/**********************************************************************
void QNF_SetRegBit(UINT8 reg, UINT8 bitMask, UINT8 data_val)
**********************************************************************
Description: set register specified bit

Parameters:
reg:		register that will be set
bitMask:	mask specified bit of register
data_val:	data will be set for specified bit
Return Value:
None
**********************************************************************/
void QNF_SetRegBit(UINT8 reg, UINT8 bitMask, UINT8 data_val) 
{
	UINT8 temp;
	temp = QND_ReadReg(reg);
	temp &= (UINT8)(~bitMask);
	temp |= data_val;
	QND_WriteReg(reg, temp);
}

/**********************************************************************
UINT16 QNF_GetCh()
**********************************************************************
Description: get channel frequency of CCS

Parameters:
None
Return Value:
channel frequency
**********************************************************************/
UINT16 QNF_GetCh() 
{
    UINT8 tCh;
    UINT8  tStep; 
    UINT16 ch = 0;
    // set to reg: CH_STEP
    tStep = QND_ReadReg(CH_STEP);
    tStep &= CH_CH;
    ch  =  tStep ;
    tCh= QND_ReadReg(CH);    
    ch = (ch<<8)+tCh;
    return CHREG2FREQ(ch);
}

/**********************************************************************
UINT8 QNF_SetCh(UINT16 freq)
**********************************************************************
Description: set specified frequency 

Parameters:
		freq:  specified frequency that is set
Return Value:
         1: set success
**********************************************************************/
UINT8 QNF_SetCh(UINT16 freq) 
{
    // calculate ch para
    UINT8 tStep;
	UINT8 tCh;
    UINT16 f; 
		f = FREQ2CHREG(freq); 
		// set to reg: CH
		tCh = (UINT8) f;
		QND_WriteReg(CH, tCh);
		// set to reg: CH_STEP
		tStep = QND_ReadReg(CH_STEP);
		tStep &= ~CH_CH;
		tStep |= ((UINT8) (f >> 8) & CH_CH);
		QND_WriteReg(CH_STEP, tStep);

    return 1;
}

/**********************************************************************
void QNF_ConfigScan(UINT16 start,UINT16 stop, UINT8 step)
**********************************************************************
Description: config start, stop, step register for FM/AM CCA or CCS

Parameters:
	start
		Set the frequency (10kHz) where scan will be started,
		eg: 76.00MHz will be set to 7600.
	stop
		Set the frequency (10kHz) where scan will be stopped,
		eg: 108.00MHz will be set to 10800.
	step		
		1: set leap step to (FM)100kHz / 10kHz(AM)
		2: set leap step to (FM)200kHz / 1kHz(AM)
		0:  set leap step to (FM)50kHz / 9kHz(AM)
Return Value:
         None
**********************************************************************/
void QNF_ConfigScan(UINT16 start,UINT16 stop, UINT8 step) 
{
	// calculate ch para
	UINT8 tStep = 0;
	UINT8 tS;
	UINT16 fStart;
	UINT16 fStop;
		fStart = FREQ2CHREG(start);
		fStop = FREQ2CHREG(stop);
		// set to reg: CH_START
	tS = (UINT8) fStart;
	QND_WriteReg(CH_START, tS);
	tStep |= ((UINT8) (fStart >> 6) & CH_CH_START);
	// set to reg: CH_STOP
	tS = (UINT8) fStop;
	QND_WriteReg(CH_STOP, tS);
	tStep |= ((UINT8) (fStop >> 4) & CH_CH_STOP);
	// set to reg: CH_STEP
	tStep |= step << 6;
	QND_WriteReg(CH_STEP, tStep);
}

/**********************************************************************
void QNF_SetAudioMono(UINT8 modemask, UINT8 mode) 
**********************************************************************
Description:	Set audio output to mono.

Parameters:
  modemask: mask register specified bit
  mode
		QND_RX_AUDIO_MONO:	  RX audio to mono
		QND_RX_AUDIO_STEREO:  RX audio to stereo	
		QND_TX_AUDIO_MONO:    TX audio to mono
		QND_TX_AUDIO_STEREO:  TX audio to stereo 
Return Value:
  None
**********************************************************************/
void QNF_SetAudioMono(UINT8 modemask, UINT8 mode) 
{
	if (mode == QND_TX_AUDIO_MONO||mode == QND_RX_AUDIO_MONO) 
	{
		// set to 22.5K (22.5/0.69 ~= 32)
		QND_WriteReg(TX_FDEV, 0x20);
	} 
	else 
	{
		// back to default
		QND_WriteReg(TX_FDEV, 0x6c);
	}
	QNF_SetRegBit(SYSTEM2,modemask, mode);
}

/**********************************************************************
void QNF_GetFMRssiAvg()
**********************************************************************
Description: get the RSSIN value
Parameters:
None
Return Value:
None
**********************************************************************/
void  QNF_GetFMRssiAvg() 
{
	UINT8 i,j;
	UINT16 ch;
	UINT8 tmp,minrssi;

	for(i = 0; i < 4; i++)
	{
		minrssi = 255;
		for (j = 0; j < 4; j++)
		{

			tmp = QND_GetRSSI(7600 + i*800 + j*10);
			if (tmp < minrssi)
			{
				minrssi = tmp;
				ch = 7600 + i*800 + j*10;
			}
		}
		for (j = 0; j< 4; j++)
		{
			tmp = QND_GetRSSI(7600 + (i+1)*800 -j*10);
			if (tmp < minrssi)
			{
				minrssi = tmp;
				ch = 7600 + (i+1)*800 -j*10;
			}

		}
		Rssinarray[i] = minrssi;
		if (RSSIn > Rssinarray[i])
		{
			RSSIn = Rssinarray[i];
			clearchannel = ch;
		}
	}
	if (RSSIn >= 32)
		QNF_SetRegBit(SMSTART, 0x3f, 63);
	else
		QNF_SetRegBit(SMSTART, 0x3f, (RSSIn+31)); /*16->31,required by Qifa for glitch 08/05/22*/	

	if (RSSIn > 77) /*required by Qifa for make sure RSSIn + 50 no more  than 127*/
		QNF_SetRegBit(SNCSTART, 0x7f, 0x7f);
	else
		QNF_SetRegBit(SNCSTART, 0x7f, (RSSIn+50));

	if (RSSIn > 44) /*required by Qifa for make sure RSSIn + 19 no more  than 63*/
		QNF_SetRegBit(HCCSTART, 0xbf, 0xbf);
	else
		QNF_SetRegBit(HCCSTART, 0xbf, ((RSSIn+19)|0x80));

}

/**********************************************************************
void QNF_UpdateRssi()
**********************************************************************
Description: update the RSSI array and RSSIin value
Parameters:
		None
Return Value:
        None
**********************************************************************/
void QNF_UpdateRssi() 
{
	UINT8 temp;
	UINT8 v_abs;
	if (firstscan == 0 )
	{
		temp = QND_GetRSSI(clearchannel);

		if(temp > RSSIn)
		{
			v_abs = temp - RSSIn;
		}
		else
		{
			v_abs = RSSIn - temp;
		}

		if (v_abs< RSSINTHRESHOLD)
		{
			clearscanflag = 0;	
		}
		else
		{
			clearscanflag = 1;

		}
	}
	if (clearscanflag||firstscan)
	{
		QNF_GetFMRssiAvg();
		firstscan = 0;
	}
	return;
}


/**********************************************************************
UINT8 QND_GetRSSI(UINT16 ch)
**********************************************************************
Description:	Get the RSSI value, call QND_TuneToCH  to set specify CH
Parameters:
Return Value:
RSSI value  of the channel setted
**********************************************************************/
UINT8 QND_GetRSSI(UINT16 ch) 
{
	if (qnd_Band == BAND_FM)
	{
		QND_SetSysMode(QND_MODE_RX|QND_MODE_FM); 
	}
	else
	{
		QND_SetSysMode(QND_MODE_RX|QND_MODE_AM); 
	}
	QNF_SetCh(ch);
	QNF_SetRegBit(SYSTEM1, CHSC, CCA_CH_DIS);
    QND_Delay(QND_READ_RSSI_DELAY); 
	return QND_ReadReg(RSSISIG);  
}

void QN_ChipInitialization()
{
	QND_WriteReg(0x01,0x89); //reset
	QND_WriteReg(0x02,0x80);
    QND_WriteReg(ANACTL1, 0b00101011); // Set the crystal to 19.2 MHz.
    if(chumby_XCLK) {
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

/**********************************************************************
int QND_Init()
**********************************************************************
Description: Initialize device to make it ready to have all functionality ready for use.

Parameters:
None
Return Value:
1: Device is ready to use.
0: Device is not ready to serve function.
**********************************************************************/
UINT8 QND_Init() 
{
	QN_ChipInitialization();
    QND_WriteReg(REG_PD2,  MUTE); //mute to avoid noise
	QND_WriteReg(HYSTERSIS, 0xff);	
	QND_WriteReg(MPSTART, 0x12);

	QNF_SetRegBit(GAIN_SEL, 0x38, 0x28);


	QND_WriteReg(REG_PD2,  UNMUTE); //unmute

	QND_WriteReg(00,  0x01); //resume original status of chip /* 2008 06 13 */
	//QND_Delay(500);
	//QND_WriteReg(REG_PD2,  MUTE); //mute to avoid noise
	//QNF_GetFMRssiAvg();
	//QND_WriteReg(REG_PD2,  UNMUTE); //unmute
	//QND_WriteReg(0x00,  0x01); //resume original status of chip /* 2008 06 13 */
	qnd_Band = BAND_FM;
    return 1;
}

/**********************************************************************
void QND_SetSysMode(UINT16 mode)
***********************************************************************
Description: Set device system mode(like: sleep ,wakeup etc) 
Parameters:
mode:
set the system mode , it will be set by  some macro define usually:
	SLEEP:  it will set device system work on sleep mode
	WAKEUP: it will wakeup device system 
	TX:     it will set device system work on TX mode
	RX:     it will set device system work on RX mode
	FM:     it will set device system work on FM mode
	AM:     it will set device system work on AM mode
	TX|FM:  it will set device system work on FM,TX mode
	RX|AM;  it will set device system work on AM,RX mode
	RX|FM:	it will set device system work on FM,RX mode
Return Value:
	None	 
**********************************************************************/
void QND_SetSysMode(UINT16 mode) 
{	
	UINT8 val;
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
			val = (UINT8)(mode >> 8);		
			if (val)
			{
					QNF_SetRegBit(SYSTEM1, R_TXRX_MASK, val);	
				qnd_Band = BAND_FM;
			}	
        break;		
    }	
}

/**********************************************************************
void QND_TuneToCH(UINT16 ch)
**********************************************************************
Description:	Tune to the specified channel. Before use this function
make sure the QND_SetSysMode() has been called. If system is working 
on FM&TX mode,it will turn FM to ch, and start Transmit; If system is
working on FM&RX mode,it will turn FM to ch, and start FM play.
Parameters:
ch
Set the frequency (10kHz) to be tuned,
eg: 101.30MHz will be set to 10130.
Return Value:
	None
**********************************************************************/
void QND_TuneToCH(UINT16 ch) 
{
	UINT8 rssi;
	UINT8 minrssi;
	if ((ch - 7710) % 240 == 0) 
	{
		QNF_SetRegBit(TXAGC_GAIN, IMR, IMR);
	} 
	else 
	{
		QNF_SetRegBit(TXAGC_GAIN, IMR, 0x00);
	}
	QNF_SetCh(ch);
	QNF_SetRegBit(SYSTEM1, CHSC, CCA_CH_DIS);

    if (QND_ReadReg(SYSTEM1) & RXREQ) 
	{
		if (ch <= 8400)
			minrssi = Rssinarray[0];  
		else if (ch <= 9200)
			minrssi = Rssinarray[1]; 
		else if (ch <= 10000)
			minrssi = Rssinarray[2]; 
		else if (ch <= 10800)
			minrssi = Rssinarray[3]; 
		if (minrssi + 12 <= 0x3f)
		{
			QND_WriteReg(0x40,minrssi + 12);
			QND_WriteReg(0x42,minrssi + 12);
		}
		else
		{
			QND_WriteReg(0x40,0x3f);
			QND_WriteReg(0x42,0x3f);
		}
		QND_WriteReg(0x41,minrssi + 18);
		QND_Delay(100);
		rssi = QND_ReadReg(RSSISIG);
		if (rssi <= RSSIn+6)
		{
			QNF_SetRegBit(0x41,0x80,0x80);
			QND_WriteReg(0x38,0x1f);
			QND_WriteReg(0x3e,0x48);
		}
		else
		{
			QNF_SetRegBit(0x41,0x80,0x00);
			QND_WriteReg(0x3e,0x00);
		}

	}

}

/**********************************************************************
void QND_SetCountry(UINT8 country)
***********************************************************************
Description: Set start, stop, step for RX and TX based on different
			 country
Parameters:
country:
Set the chip used in specified country:
	CHINA:
	USA:
	JAPAN:
Return Value:
	None	 
**********************************************************************/
void QND_SetCountry(UINT8 country) 
{
	qnd_Country = country;
	switch(country)
	{
	case COUNTRY_EUROPE:
		QND_CH_START = 8700;
		QND_CH_STOP = 10800;
		QND_CH_STEP = 1;
		break;
	case COUNTRY_CHINA:
		QND_CH_START = 7600;
		QND_CH_STOP = 10800;
		QND_CH_STEP = 1;
		break;
	case COUNTRY_USA:
		QND_CH_START = 8790;
		QND_CH_STOP = 10790;
		QND_CH_STEP = 2;
		break;
	case COUNTRY_JAPAN:
		QND_CH_START = 7600;
		QND_CH_STOP = 9000;
		QND_CH_STEP = 1;
		break;
	}
}

/***********************************************************************
Description: set call back function which can be called between seeking 
channel
Parameters:
func : the function will be called between seeking
Return Value:
None
**********************************************************************/
void QND_SetSeekCallBack(QND_SeekCallBack func)
{
	callbackfunc = func;
}

/***********************************************************************
UINT16 QND_RXSeekCH(UINT16 start, UINT16 stop, UINT16 step, UINT8 db, UINT8 up);
***********************************************************************
Description: Automatically scans the frequency range, and detects the 
		first channel(AM or FM, it will be determine by the system mode which set 
		by QND_SetSysMode).
		A threshold value needs to be passed in for channel detection.
Parameters:
	start
		Set the frequency (10kHz) where scan will be started,
		eg: 76.00MHz will be set to 7600.
	stop
		Set the frequency (10kHz) where scan will be stopped,
		eg: 108.00MHz will be set to 10800.
	step
		FM:
			QND_FMSTEP_100KHZ: set leap step to 100kHz
			QND_FMSTEP_200KHZ: set leap step to 200kHz
			QND_FMSTEP_50KHZ:  set leap step to 50kHz
		AM:
			QND_AMSTEP_***:
	db:
		Set threshold for quality of channel to be searched. 
	up:
		Set the seach direction :
			Up;0,seach from stop to start
			Up:1 seach from start to stop
Return Value:
  The channel frequency (unit: 10kHz)
  -1: no channel found

***********************************************************************/
UINT16 QND_RXSeekCH(UINT16 start, UINT16 stop, UINT8 step, UINT8 db, UINT8 up) 
{

    UINT8 regValue;
    UINT16 c, chfreq;
    UINT16 ifcnt;
    UINT8 savevalue;
    UINT8 rssi,mprssi;
    UINT8 scanFlag = 1;
    UINT8 stepvalue;
    UINT8 timeout = 0;
    UINT8 r13;
    UINT8 r56;
	UINT8 minrssi;
    r13 = QND_ReadReg(TXAGC_GAIN);
    r56 = QND_ReadReg(CCOND1);
	if (autoScanAll == 0)
	{
			QND_WriteReg(REG_PD2,  MUTE); //mute to avoid noise	
	}
		stepvalue = steparray[step + qnd_Band * 3];
		if (autoScanAll == 0)
		{
			QNF_UpdateRssi();	
		}
		r56 &= 0x3f;
		QND_WriteReg(CCOND1, r56);
		//-----------------------------------
		QND_WriteReg(HCCSTART,0xb5); //r66
		QND_WriteReg(CCA2,0x97); //r53
		savevalue =QND_ReadReg(SYSTEM1);
		do
		{
			if (start <= 8400)
				minrssi = Rssinarray[0];  
			else if (start <= 9200)
				minrssi = Rssinarray[1]; 
			else if (start <= 10000)
				minrssi = Rssinarray[2]; 
			else if (start <= 10800)
				minrssi = Rssinarray[3]; 						
			QNF_SetCh(start);
			savevalue |= 0x81;
			QND_WriteReg(SYSTEM1,savevalue);
			QND_Delay(10); //10ms delay
			rssi = QND_ReadReg(RSSISIG);	//read RSSI
			if (rssi > minrssi+6+db)
			{
				QNF_ConfigScan(start, start, step);
				QNM_SetRxThreshold(minrssi-22+db);
				savevalue &=0xfe;
				savevalue |=0xa0;
				QND_WriteReg(SYSTEM1,savevalue);
                timeout = 0;
				do
				{
					QND_Delay(5);
					regValue = QND_ReadReg(SYSTEM1);
                    timeout++;
				} while((regValue &CHSC)&&timeout < 200);
                if (timeout < 200)
                {
					regValue = QND_ReadReg(CCA5); //55
					ifcnt = regValue;
					regValue = QND_ReadReg(CCA4); //54

					ifcnt =(((ifcnt&0x1f)<<8)+regValue);//

					if ((ifcnt < 2268) && (ifcnt > 1828))
					{
						if (rssi > minrssi + 12 + db)			
						{
							scanFlag = 0;
						}
						else 
						{
							QND_Delay(100);//100ms delay
							mprssi = QND_ReadReg(RSSIMP); 
							if (mprssi < 0x2a)	
							{
								scanFlag = 0;
							}
						}
					}
				}

			}
			chfreq = start;
			if (scanFlag ==1)
			{
				if (up)
				{
					start += stepvalue;
					if (stop < start)
					{
						scanFlag = 0;
						chfreq = 0;

					}
				}
				else
				{
					start -= stepvalue;
					if (start < stop)
					{
						scanFlag = 0;
						chfreq = 0;
					}
				}
			}
		} 
		while (scanFlag != 0);
		if (chfreq != 0)
		{
			if (callbackfunc)
				callbackfunc(chfreq, BAND_FM);
		}
		c = chfreq;
		QND_WriteReg(CCA2,0x93); //r53
		QND_WriteReg(CCOND1,0x6d); //R56

		if ((c - 7710) % 240 == 0) 
		{ 		
			//reverse  IMR for Yifeng request  /*Watt */
			QNF_SetRegBit(TXAGC_GAIN, IMR, IMR);
		} 
		else 
		{
			QNF_SetRegBit(TXAGC_GAIN, IMR, 0);
		}		

	if (autoScanAll == 0)
	{
		QND_WriteReg(REG_PD2,  UNMUTE); //unmute
	}
    //QND_LOGB("Finished AutoScan", c);
    //QND_LOG("=== Done QND_AutoScan_FMChannel_One ===");
    return c;  
}

/**********************************************************************
UINT8 QND_RXSeekCHAll(UINT16 start, UINT16 stop, UINT16 step, UINT8 db, UINT8 up)
**********************************************************************
Description:	Automatically scans the complete FM or AM band and detects 
			all the available  channels(AM or FM, it will be determine by 
			the workmode which set by QND_SetSysmode). A threshold value 
			needs to be passed in for the channel detection.
Parameters:
    start
        Set the frequency (10kHz) where scan will be started,
        eg: 76.00MHz will be set to 7600.
    stop
        Set the frequency (10kHz) where scan will be stopped,
        eg: 108.00MHz will be set to 10800.
	Step
		FM:
			QND_FMSTEP_100KHZ: set leap step to 100kHz
			QND_FMSTEP_200KHZ: set leap step to 200kHz
			QND_FMSTEP_50KHZ:  set leap step to 50kHz
		AM:
		QND_AMSTEP_***:
    db
        Set signal noise ratio for channel to be searched.
	up:
		Set the seach direction :
		Up;0,seach from stop to start
		Up:1 seach from start to stop

Return Value:
  The channel count found by this function
  -1: no channel found 
**********************************************************************/
UINT8 QND_RXSeekCHAll(UINT16 start, UINT16 stop, UINT8 step, UINT8 db, UINT8 up) 
{
    UINT8 c;
    UINT16 prevCh = 0;
    //UINT16 chfreq;
    UINT16 pStart = start;
    UINT8  stepvalue;
    UINT16 temp;
	stepvalue = steparray[step + qnd_Band * 3];
    autoScanAll = 1;
    QND_WriteReg(REG_PD2,  MUTE); //mute
	QNF_GetFMRssiAvg();
    chCount = 0;
    do{  
        
		temp = QND_RXSeekCH(pStart, stop, step, db, up);

        if (temp) 
		{
            chList[chCount++] =temp;
        }
		else
		{
			temp = stop;
		}
		pStart = temp + stepvalue;

    }
    while((pStart < stop) && pStart < QND_CH_STOP && chCount < QN_CCA_MAX_CH);


    // Modification 25-jul-2009 by SMC
    // We don't want to automatically tune to the active channel, because
    // seeking for available channels is different from tuning to a
    // channel.
//	c = chCount;  
//    if (c >= 1)
//    {
//        QND_TuneToCH(chList[0]); /*2008-05-27 phong*/
//    }
//    autoScanAll = 0;
//    QND_WriteReg(REG_PD2,  UNMUTE); //unmute	
    return c;
}

/**********************************************************************
Description:	Config the Rx Audio performance like volume adjust, equalizer set, 
mute set:
Parameters:
optiontype:
It indicates what you want to config,   it can use some macro define to control:
eg: VOLUMECONFIG, EQUALIZERCONFIG,MUTECONFIG, MONOCONFIG,OUTPUTFORMATCONFIG
option: 
set the config value;

if (optiontype == QND_CONFIG_MUTE); ¡®option¡¯ will control muteable, 0:mutedisable,
1:mute enable 
if (optiontype == QND_CONFIG_MONO); ¡®option¡¯ will control mono, 0: QND_AUDIO_STEREO,
1: QND_AUDIO_STEREO
if (optiontype == QND_CONFIG_EQUALIZER): 'option' will control the EQUALIZER,
		0:disable  EQUALIZER; 1: enable EQUALIZER;
if (optiontype == QND_CONFIG_VOLUME): 'option' will control the volume gain,
		range : 0~83(0: -65db, 65: 0db, 83: +18db)
if (optiontype == QND_CONFIG_BASS_QUALITY): 'option' will set BASS quality factor,
		0: 1, 1: 1.25, 2: 1.5, 3: 2
if (optiontype == QND_CONFIG_BASS_FREQ): 'option' will set BASS central frequency,
		0: 60Hz, 1: 70Hz, 2: 80Hz, 3: 100Hz
if (optiontype == QND_CONFIG_BASS_GAIN): 'option' will set BASS control gain,
		range : 0x0~0x1e (00000 :-15db, 11110 :15db)
if (optiontype == QND_CONFIG_MID_QUALITY): 'option' will set MID quality factor,
		0 :1, 1 :2
if (optiontype == QND_CONFIG_MID_FREQ): 'option' will set MID central frequency,
		0: 0.5KHz, 1: 1KHz, 2: 1.5KHz, 3: 2KHz
if (optiontype == QND_CONFIG_MID_GAIN): 'option' will set MID control gain,
		range : 0x0~0x1e (00000 :-15db, 11110 :15db)
if (optiontype == QND_CONFIG_TREBLE_FREQ): 'option' will set TREBLE central frequency,
		0: 10KHz, 1: 12.5KHz, 2: 15KHz, 3: 17.5KHz
if (optiontype == QND_CONFIG_TREBLE_GAIN): 'option' will set TREBLE control gain,
		range : 0x0~0x1e (00000 :-15db, 11110 :15db)

Return Value:
none
**********************************************************************/
void QND_RXConfigAudio(UINT8 optiontype, UINT8 option ) 
{
	switch(optiontype)
	{
	case QND_CONFIG_MONO:
		if (option)
			QNF_SetAudioMono(RX_MONO_MASK, QND_RX_AUDIO_MONO);
		else
			QNF_SetAudioMono(RX_MONO_MASK, QND_RX_AUDIO_STEREO);
		break;
	case QND_CONFIG_MUTE:
		if (option)
			QND_WriteReg(REG_PD2, MUTE);
		else
			QND_WriteReg(REG_PD2, UNMUTE);
		break;
	default:
		break;
	}
}

/**********************************************************************
UINT8 QND_SetRDSMode(UINT8 mode) 
**********************************************************************
Description: Enable or disable chip to work with RDS related functions.
Parameters:
          on: QND_RDS_ON:  Enable chip to receive/transmit RDS data.
                QND_RDS_OFF: Disable chip to receive/transmit RDS data.
Return Value:
           QND_SUCCESS: function executed

**********************************************************************/
UINT8 QND_RDSEnable(UINT8 on) 
{
    UINT8 val;

	QND_LOG("=== QND_SetRDSMode === ");
    // get last setting
    val = QND_ReadReg(SYSTEM1);
    if (on == QND_RDS_ON) 
    {
        val |= RDSEN;
    } 
    else if (on == QND_RDS_OFF)
    {
        val &= ~RDSEN;
    }
    else 
    {
        return 0;
    }
    QND_WriteReg(SYSTEM1, val);
	return 1;
}

/**********************************************************************
UINT8 QND_DetectRDSSignal(void)
**********************************************************************
Description:	detect the RDSS signal .

Parameters:
          None
Return Value:
		 the value of STATUS3
**********************************************************************/
UINT8 QND_RDSDetectSignal(void) 
{
    UINT8 val = QND_ReadReg(STATUS3);	
    return val;
}

 /**********************************************************************
 UINT8 QND_LoadRDSData( UINT8 *buf)
 **********************************************************************
 Description: Load (TX) or unload (RX) RDS data to on-chip RDS buffer. 
 			  Before calling this function, always make sure to call the 
			  QND_RDSBufferReady function to check that the RDS is capable 
			  to load/unload RDS data.
 Parameters:
	      rdsRawData : 8 bytes data buffer to load (on TX mode) or unload (on RX
		  mode) to chip RDS buffer.
		  Upload:   1-upload, 0--download
 Return Value:
	     QND_SUCCESS: function executed
**********************************************************************/
void QND_RDSLoadData(UINT8 *rdsRawData, UINT8 upload)
{
	UINT8 i;
	UINT8 temp;
    if (upload) 
	{	   //TX MODE
        for (i = 0; i <= 7; i++) 
		{
            QND_WriteReg(RDSD0 + i, rdsRawData[i]);
        }	
    } 
	else 
	{
		//RX MODE
        for (i = 0; i <= 7; i++) 
		{
            temp = QND_ReadReg(RDSD0 + i);
            rdsRawData[i] = temp;
        }
    }
}

/**********************************************************************
UINT8 QND_RDSBufferReady(void)
**********************************************************************
Description: Check chip RDS register buffer status before doing load/unload of
			 RDS data.

Parameters:
         None
Return Value:
         QND_RDS_BUFFER_NOT_READY: RDS register buffer is not ready to use.
         QND_RDS_BUFFER_READY: RDS register buffer is ready to use. You can now
                                   load (for TX) or unload (for RX) data 
                                   from/to RDS buffer
**********************************************************************/
UINT8 QND_RDSCheckBufferReady(void) 
{
    // get last setting
    UINT8 ret = QND_RDS_BUFFER_NOT_READY;
    UINT8 val;
	UINT8 temp;
	UINT8 rdsUpdated;

    rdsUpdated = ((QND_ReadReg(STATUS3)&RDS_RXUPD) ? 1 : 0);
		do
		{
			val = QND_ReadReg(STATUS3);
			temp = val&RDS_RXUPD ? 1 : 0;	  
		}while(!(rdsUpdated^temp)) ;
		ret = QND_RDS_BUFFER_READY;
    return ret;
}

