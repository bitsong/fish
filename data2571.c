/****************************************************************************/
/*                                                                          */
/*              McBSP1 							                              */
/*                                                                          */
/*	auther:wangsong															*/
/*	date:20160831															*/
/*	Right:shanghai haotong													*/

#include "data2571.h"
#include <math.h>
/****************************************************************************/
/*                                                                          */
/*              锟疥定锟斤拷                                                      */
/*                                                                          */
/****************************************************************************/
//osc 16.8M
#define	PFDin		(21.0*1000)			// 锟斤拷锟斤拷频锟绞ｏ拷指锟斤拷:21.0MHz锟斤拷

#define FR_BAUD		24		// 120K
#define DEN			1048576 //2^20		//16777216	//2^24
#define Prescaler	2
#define Pi			3.1415926f
/****************************************************************************/
/*                                                                          */
/*              锟斤拷锟斤拷锟斤拷锟斤拷                                                    */
/*                                                                          */
/****************************************************************************/

double	VCO_FM;							// VCO频锟斤拷
double	PLL_N_CAL;						// PLL锟斤拷锟斤拷锟斤拷荩锟絅值锟斤拷
double	PLL_NUM;						// PLL锟斤拷锟斤拷锟斤拷荩锟叫★拷锟�
unsigned char	CHDIV1,CHDIV2;					// 锟斤拷锟斤拷VCO锟斤拷锟斤拷锟狡抵�
unsigned short	    PLL_N;							// PLL锟斤拷锟斤拷锟斤拷荩锟斤拷锟斤拷锟�
unsigned short		PLL_NUM_H8;						// PLL锟斤拷锟斤拷锟斤拷莞甙锟轿伙拷锟叫★拷锟�
unsigned short		PLL_NUM_L16;					// PLL锟斤拷锟斤拷锟斤拷莸锟�6位锟斤拷小锟斤拷
unsigned short	    PLL_DEN_H8;
unsigned short	    PLL_DEN_L16;

unsigned char	CHDIV1_DATA[]	= {4,5,6,7};				// 锟斤拷锟斤拷VCO锟斤拷锟斤拷锟狡�
unsigned char	CHDIV2_DATA[]	= {1,2,4,8,16,32,64};		// 锟斤拷锟斤拷VCO锟斤拷锟斤拷锟狡�

void FSK_FAST_SPI(double FMout)
{
	unsigned int i2,FSK_steps;
	double Freq_dev;

	FSK_steps=0;
#if 0
	for(i2=0; i2<FR_BAUD; i2++){
		Freq_dev=FMout*sin(2*Pi*i2/FR_BAUD);
//		Freq_dev=3;
		if(Freq_dev>=0)
			FSK_steps=((Freq_dev*DEN/PFDin)*(CHDIV1_DATA[CHDIV1]*CHDIV2_DATA[CHDIV2]/Prescaler));
		else
			FSK_steps=65535+((Freq_dev*DEN/PFDin)*(CHDIV1_DATA[CHDIV1]*CHDIV2_DATA[CHDIV2]/Prescaler))+1;

		lmx_init[54+i2]=0x210000|(unsigned int)(FSK_steps);

	}

#else
	for(i2=0; i2<FR_BAUD; i2++){
					Freq_dev=FMout*sin(2*Pi*i2/FR_BAUD);
//					Freq_dev=5;
					FSK_steps=(unsigned int)((Freq_dev*DEN/PFDin)*(CHDIV1_DATA[CHDIV1]*CHDIV2_DATA[CHDIV2]/Prescaler));
					lmx_init[54+i2]=0x210000|(FSK_steps&0x00FFFF);
			}
#endif
}


float  FSK_FAST_SPI_calc(void)
{
	float factor;

	factor=DEN/(float)(PFDin*1000)*(CHDIV1_DATA[CHDIV1]*CHDIV2_DATA[CHDIV2]/Prescaler);
//	FSK_steps=(unsigned int)((float)Freq_dev*factor);
//	FSK_steps=(unsigned int)(((double)Freq_dev*DEN/PFDin/32000)*(CHDIV1_DATA[CHDIV1]*CHDIV2_DATA[CHDIV2]/Prescaler));
//	FSK_steps=0x210000|(FSK_steps&0x00FFFF);
//	return FSK_steps;
	return	factor;
}


//================================================================ 锟斤拷锟斤拷锟斤拷锟斤拷锟狡抵�
void VCO_FM_CAL(double FMout)
{
	int i,j;
	double fm;

	double	VCO_Range[] 			= {4800000000,5184000000};	// VCO 锟斤拷锟�围(4993.6MHz,5184MHz)
	//------------------------------------------------------------
	for (j=0; j<7; j++)
	{
		for (i=0; i<4; i++)
		{
			fm =  CHDIV1_DATA[i]*CHDIV2_DATA[j]*FMout*1000000;

			if (fm > VCO_Range[0] & fm < VCO_Range[1])
			{
				VCO_FM = fm;
				CHDIV1 = i;	//Correspond to CHDIV1_DATA[i] in register;
				CHDIV2 = j;	//Correspond to CHDIV2_DATA[j] in register;
				return;
			}
		}
	}
}
#if 0
//============================================================ 锟斤拷锟斤拷N值
void VCO_N_CAL(double FMout)
{
	double	fm;
	double	Fm;
	//-------------------------------------------- N
	Fm = PFDin;
	Fm = Fm*1000;

	PLL_N_CAL = VCO_FM/2;
	PLL_N_CAL = PLL_N_CAL/Fm;
	//-------------------------------------------- N锟斤拷锟斤拷
	PLL_N = (int) PLL_N_CAL;
	//-------------------------------------------- N小锟斤拷
	fm = PLL_N_CAL - PLL_N;
	Fm = fm * 16777215;
	PLL_NUM = (int) Fm;
	PLL_NUM_H8 = PLL_NUM/65536;					// N小锟斤拷甙锟轿伙拷锟�
	PLL_NUM_L16 = PLL_NUM - PLL_NUM_H8*65536;	// N小锟斤拷锟�6位锟斤拷

	PLL_DEN_H8=255;
	PLL_DEN_L16=65535;
//	PLL_DEN_H8=0;
//	PLL_DEN_L16=0;
}

#else
//============================================================ 锟斤拷锟斤拷N值
void VCO_N_CAL(double FMout)
{
	double	fm;
	double	Fm;
	//-------------------------------------------- N
	Fm = PFDin;
	Fm = Fm*1000;

	PLL_N_CAL = VCO_FM/2;
	PLL_N_CAL = PLL_N_CAL/Fm;
	//-------------------------------------------- N锟斤拷锟斤拷
	PLL_N = (int) PLL_N_CAL;
	//-------------------------------------------- N小锟斤拷
	fm = PLL_N_CAL - PLL_N;
	Fm = fm * (DEN-1);
	PLL_NUM = (int) Fm;
	PLL_NUM_H8 = PLL_NUM/65536;					// N小锟斤拷甙锟轿伙拷锟�
	PLL_NUM_L16 = PLL_NUM - PLL_NUM_H8*65536;	// N小锟斤拷锟�6位锟斤拷

	PLL_DEN_H8=15;
	PLL_DEN_L16=65535;
//	PLL_DEN_H8=0;
//	PLL_DEN_L16=0;
}
#endif
//================================================================ 锟斤拷锟斤拷锟斤拷锟斤拷锟狡抵�
void LMX2571_FM_CAL(unsigned short ch, double fm)
{
	VCO_FM_CAL(fm);
	VCO_N_CAL(fm);


	if (ch==0)
	{	//---------------------------------------- 锟斤拷ch=0锟斤拷锟斤拷锟斤拷锟紽1锟斤拷
		LMX2571_R06.Data=lmx_init[38]&0xffff;
		LMX2571_R06.CtrlBit.CHDIV1_F1 = CHDIV1;
		LMX2571_R06.CtrlBit.CHDIV2_F1 = CHDIV2;

		LMX2571_R04.Data=lmx_init[40]&0xffff;
		LMX2571_R04.CtrlBit.PLL_N_F1 = PLL_N;

		LMX2571_R03.Data=lmx_init[41]&0xffff;
		LMX2571_R03.CtrlBit.LSB_PLL_DEN_F1 = PLL_DEN_L16;

		LMX2571_R02.Data=lmx_init[42]&0xffff;
		LMX2571_R02.CtrlBit.LSB_PLL_NUM_F1 = PLL_NUM_L16;

		LMX2571_R01.Data=lmx_init[43]&0xffff;
		LMX2571_R01.CtrlBit.MSB_PLL_NUM_F1 = PLL_NUM_H8;
		LMX2571_R01.CtrlBit.MSB_PLL_DEN_F1 = PLL_DEN_H8;

		lmx_init[45]=lmx_init[36];
		lmx_init[46]=lmx_init[37];
		lmx_init[47]=LMX2571_R06.Data|0x060000;
		lmx_init[48]=lmx_init[39];
		lmx_init[49]=LMX2571_R04.Data|0x040000;
		lmx_init[50]=LMX2571_R03.Data|0x030000;
		lmx_init[51]=LMX2571_R02.Data|0x020000;
		lmx_init[52]=LMX2571_R01.Data|0x010000;//R1
	}
	else if (ch==1)
	{	//---------------------------------------- 锟斤拷ch=1锟斤拷锟斤拷锟秸伙拷F2锟斤拷
		LMX2571_R22.Data=lmx_init[22]&0xffff;
		LMX2571_R22.CtrlBit.CHDIV1_F2 = CHDIV1;
		LMX2571_R22.CtrlBit.CHDIV2_F2 = CHDIV2;

		LMX2571_R20.Data=lmx_init[24]&0xffff;
		LMX2571_R20.CtrlBit.PLL_N_F2 = PLL_N;

		LMX2571_R19.Data=lmx_init[25]&0xffff;
		LMX2571_R19.CtrlBit.LSB_PLL_DEN_F2 = PLL_DEN_L16;

		LMX2571_R18.Data=lmx_init[26]&0xffff;
		LMX2571_R18.CtrlBit.LSB_PLL_NUM_F2 = PLL_NUM_L16;

		LMX2571_R17.Data=lmx_init[27]&0xffff;
		LMX2571_R17.CtrlBit.MSB_PLL_NUM_F2 = PLL_NUM_H8;
		LMX2571_R17.CtrlBit.MSB_PLL_DEN_F2 = PLL_DEN_H8;

		lmx_init[45]=lmx_init[20];
		lmx_init[46]=lmx_init[21];
		lmx_init[47]=LMX2571_R22.Data|0x160000;
		lmx_init[48]=lmx_init[23];
		lmx_init[49]=LMX2571_R20.Data|0x140000;
		lmx_init[50]=LMX2571_R19.Data|0x130000;
		lmx_init[51]=LMX2571_R18.Data|0x120000;
		lmx_init[52]=LMX2571_R17.Data|0x110000;//R17
	}
	else
	{	//---------------------------------------- 锟斤拷ch=锟斤拷锟斤拷锟斤拷效
	}
	lmx_init[53]=lmx_init[44];
}
