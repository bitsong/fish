#ifndef __MAIN_H__
#define __MAIN_H__

#include <ti/csl/cslr_syscfg0_OMAPL138.h>
#include <ti/csl/cslr_gpio.h>
#include <ti/csl/cslr_device.h>
#include "syslink_init.h"

//debug command

#define VHF_DSC 		01
#define VHF_DSC_ON		02
#define VHF_DSC_OFF		03
#define	PWB_STATUS		04
#define PWB_ON			05
#define PWB_OFF			06
#define	DSC				07
#define DSC_CH			08
#define ADF4002_LD		09
#define RSSI_RD			10
#define IF_AGC_ON		11
#define IF_AGC_OFF		12
#define LNA_ATT_ON		13
#define LNA_ATT_OFF		14
#define TX_ON			15
#define TX_OFF			16
#define RX_ON			17
#define RX_OFF			18
#define LMX2571_TX		19
#define LMX2571_RX		20
#define LMX2571_LD		21
#define TX_CH			22
#define RX_CH			23
#define TX_DAC			24
#define TX_PWR_SET		25
#define	RX_TUNE			26
#define BAND_0			27
#define BAND_1			28
#define ANT_NET_SEL		29
#define DAC_SET_16M8	30
#define PA_CURRENT		31
#define PA_TEMP			32
#define TX_POWER		33
#define REV_POWER		34
#define VSWR			35
#define AMC7823_AD		36
#define AMC7823_DA		37
#define AMC7823_AD_SET	38
#define AMC7823_DA_SET	39
#define I2C_W			40
#define IWC_R			41

#define TX_CHF			100
#define RX_CHF			101
#define H_TX			110
#define L_TX			111
#define RSSTH			120
#define RXSSI			150
#define TXPI			151


#define RPE_DATA_SIZE		480
#define REC_BUFSIZE	 		768  //256*3

typedef volatile union reg_24bit{
	struct reg24_2571{
		unsigned char	data0;
		unsigned char	data1;
		unsigned char	data2;
		unsigned char	data3;
	}dataBit;
	unsigned int all;
}reg_24;

typedef volatile union reg_16bit{
	struct reg16_2571{
		unsigned char	data0;
		unsigned char	data1;
	}dataBit;
	unsigned short all;
}reg_16;

typedef volatile union reg2_16bit{
	struct reg2_16_2571{
		unsigned char	data0;
		unsigned char	data1;
	}dataBit;
	short all;
}reg2_16;

extern float RSSI;

struct mydata{
    char d[100];
};


declare_message_type(message_t,struct mydata);


#endif	//__MAIN_H__
