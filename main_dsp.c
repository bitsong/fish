/*
 * audio_loop.c
 *
 *  Created on: 2016-12-20
 *      Author: ws
 */

#include <ti/sysbios/BIOS.h>
#include <math.h>

#include "syslink_init.h"
#include "main.h"
#include "audio_queue.h"
#include "amc7823.h"


#define SYNC_CODE_PREPARE	100

#define RPE_DATAFLOW_END	101

CSL_GpioRegsOvly     gpioRegs = (CSL_GpioRegsOvly)(CSL_GPIO_0_REGS);

short ring_ar[RPE_DATA_SIZE/2];
UInt	tx_flag, rx_flag, tx_submit, rx_submit;
Semaphore_Handle sem1,sem2;
unsigned char *buf_transmit;

unsigned char buf_adc[REC_BUFSIZE];

double RSSI_db=0;
UInt RXSS_THRESHOLD =0;

/* private functions */
Void smain(UArg arg0, UArg arg1);
Void task_enque(UArg arg0, UArg arg1);
Void task_receive(UArg arg0, UArg arg1);
extern Void task_mcbsp(UArg arg0, UArg arg1);

extern unsigned int lmx_init[];
extern float FSK_FAST_SPI_calc(void);
extern void LMX2571_FM_CAL(unsigned short ch, double fm);
void data_process(unsigned int *buf_in,	unsigned char *buf_out,	unsigned int size);

Void data_send(uint8_t *buf_16);

extern void audioSignalRX();
void sys_configure(void);
void dsp_logic(message_t *msg_send);

/*
 *  ======== main =========
 */
Int main(Int argc, Char* argv[])
{
    Error_Block     eb;
    Task_Params     taskParams;

    log_init();
    Error_init(&eb);

    log_info("-->main:\n");
    /* create main thread (interrupts not enabled in main on BIOS) */
    Task_Params_init(&taskParams);
    taskParams.instance->name = "smain";
    taskParams.arg0 = (UArg)argc;
    taskParams.arg1 = (UArg)argv;
    taskParams.stackSize = 0x1000;
    Task_create(smain, &taskParams, &eb);
    if(Error_check(&eb)) {
        System_abort("main: failed to create application startup thread");
    }

    /* start scheduler, this never returns */
    BIOS_start();

    /* should never get here */
    log_info("<-- main:\n");
    return (0);
}


/*
 *  ======== smain ========
 */
Void smain(UArg arg0, UArg arg1)
{
    Error_Block		eb;
    Task_Params     taskParams;
    Int 			status=0;

    log_info("-->smain:");
    if((status=syslink_prepare())<0){
    	log_error("syslink prepare log_error status=%d",status);
       	return;
    }


    sem1=Semaphore_create(0,NULL,&eb);
    sem2=Semaphore_create(0,NULL,&eb);
    tx_flag=0;
    rx_flag=1;
    sys_configure();

    status=NO_ERR;

    Spi_dev_init();
    status=amc7823_init();
    if(status<0)
    	log_error("amc7823 initial error:%d",status);
    else
    	log_info("amc7823 initial done.");

//    dac_write(0, 1.633);
//    dac_write(3, 0);

    //send ready message
    status=sync_send(SYNC_CODE_PREPARE);
    if(status<0){
      	log_error("send SYS_CODE_PREPARE failed");
      	goto cleanup;
    }
    status=sync_wait_for(SYNC_CODE_PREPARE);
    if(status<0){
    	log_error("receive SYNC_CODE_PREPAER failed");
    	goto cleanup;
    }

    //task create
    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.instance->name = "task_receive";
    taskParams.arg0 = (UArg)arg0;
    taskParams.arg1 = (UArg)arg1;
    taskParams.stackSize = 0x4000;
    Task_create(task_receive, &taskParams, &eb);
    if(Error_check(&eb)) {
        System_abort("main: failed to create application 0 thread");
    }

    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.instance->name = "task_mcbsp";
    taskParams.arg0 = (UArg)arg0;
    taskParams.arg1 = (UArg)arg1;
    taskParams.stackSize = 0x1000;
    Task_create(task_mcbsp, &taskParams, &eb);
    if(Error_check(&eb)) {
    	System_abort("main: failed to create application 1 thread");
    }

    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.instance->name = "task_enque";
    taskParams.arg0 = (UArg)arg0;
    taskParams.arg1 = (UArg)arg1;
    taskParams.stackSize = 0x1000;
    Task_create(task_enque, &taskParams, &eb);
    if(Error_check(&eb)) {
    	System_abort("main: failed to create application 2 thread");
    }

    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.instance->name = "audioSignalRX";
    taskParams.arg0 = (UArg)arg0;
    taskParams.arg1 = (UArg)arg1;
    taskParams.stackSize = 0x8000;
    Task_create(audioSignalRX, &taskParams, &eb);
    if(Error_check(&eb)) {
        System_abort("main: failed to create application 0 thread");
    }


    message_t *msg_send=(message_t *)message_alloc(msgbuf[0],sizeof(message_t));
    if(!msg_send){
    	log_warn("msgq out of memmory");
    }
    msg_send->type=0;
    msg_send->data.d[0]='\0';

    for(;;){
    	RSSI_db=-24.288+10*log10(RSSI);

    	dsp_logic(msg_send);

        Task_sleep(30);
    }

    //cleanup
cleanup:
        sync_send(SYNC_CODE_MSGOUT);
        sync_send(SYNC_CODE_NTFOUT);
        sync_send(SYNC_CODE_RPEOUT);
        status=syslink_cleanup();
        if(status<0){
        	log_error("syslink cleanup failed!");
        }
}

/*
 *  ======== task_receive ========
 */
Void task_receive(UArg arg0, UArg arg1)
{
    Error_Block         eb;
    struct rpe *prpe = NULL;
    rpe_attr_t attr=RPE_ATTR_INITIALIZER;
    prpe=&rpe[0];
	uint8_t 		*buf = NULL;
    uint32_t		size;
    Int status=0;
    buf_transmit=(unsigned char*)malloc(RPE_DATA_SIZE/2*15);

    log_info("-->task_receive:");
    Error_init(&eb);

//    do{
    	while(1){
    		Semaphore_pend(sem2, BIOS_WAIT_FOREVER);

    		size = RPE_DATA_SIZE;
    		status = rpe_acquire_reader(prpe,(rpe_buf_t*)&buf,&size);
    		if(status == ERPE_B_PENDINGATTRIBUTE){
    			status = rpe_get_attribute(prpe,&attr,RPE_ATTR_FIXED);
    			if(!status && attr.type == RPE_DATAFLOW_END){
    				log_info("data flow end!");
    				break;
    			}
    		}else if(status < 0){
    			if(status == ERPE_B_BUFEMPTY){
//    			    log_info("rpe data empty! status = %d",status);
    			}
    			else{
    				log_warn("rpe acquire data failed,status = %d",status);
    			}
//    			msleep(5);
    			continue;
    		}
//    		memset((unsigned char *)ring_ar,0,RPE_DATA_SIZE);
    		memcpy((unsigned char *)ring_ar,buf,size);
    		data_process((unsigned int *)ring_ar,buf_transmit,size);
//    		tx_submit=1;


    		status = rpe_release_reader_safe(prpe,buf,size);
    		if(status < 0){
    			log_warn("rpe release writer end failed!");
    			continue;
    		}
    	}
//    	Task_sleep(30);
//    }while(1);
}


void data_process(unsigned int *buf_in,	unsigned char *buf_out,	unsigned int size)
{

	uint32_t tempdata=0;
	uint32_t tempCount;
	reg_16 reg16;

	static float factor;

	factor=FSK_FAST_SPI_calc();

    for (tempCount = 0; tempCount < RPE_DATA_SIZE/2; tempCount++){
//    	if(ring_ar[tempCount]>20000)
//    		ring_ar[tempCount]=20000;
//    	else if(ring_ar[tempCount]<-20000)
//    		ring_ar[tempCount]=-20000;

    	//Interpolation
    	reg16.all=(unsigned short)(factor*(ring_ar[tempCount]/4));

    	((uint8_t *)buf_out)[tempdata] 	 	 = reg16.dataBit.data0;
    	((uint8_t *)buf_out)[tempdata+3] 	 = reg16.dataBit.data0;
    	((uint8_t *)buf_out)[tempdata+6]	 = reg16.dataBit.data0;
    	((uint8_t *)buf_out)[tempdata+9] 	 = reg16.dataBit.data0;
    	((uint8_t *)buf_out)[12+tempdata++]  = reg16.dataBit.data0;

    	((uint8_t *)buf_out)[tempdata] 	 	 = reg16.dataBit.data1;
    	((uint8_t *)buf_out)[tempdata+3] 	 = reg16.dataBit.data1;
    	((uint8_t *)buf_out)[tempdata+6]	 = reg16.dataBit.data1;
    	((uint8_t *)buf_out)[tempdata+9] 	 = reg16.dataBit.data1;
    	((uint8_t *)buf_out)[12+tempdata++]  = reg16.dataBit.data1;

    	((uint8_t *)buf_out)[tempdata] 	     = (uint8_t)33;
    	((uint8_t *)buf_out)[tempdata+3] 	 = (uint8_t)33;
    	((uint8_t *)buf_out)[tempdata+6]	 = (uint8_t)33;
    	((uint8_t *)buf_out)[tempdata+9] 	 = (uint8_t)33;
    	((uint8_t *)buf_out)[12+tempdata++]  = (uint8_t)33;

    	tempdata+=12;
    }
//            CSL_FINS(gpioRegs->BANK[1].OUT_DATA,GPIO_OUT_DATA_OUT2,1);
}

/*
 *  ======== data_send ========
 */
Void data_send(uint8_t *buf_16)
{
    struct rpe *prpe = NULL;
    prpe=&rpe[0];
	uint8_t 		*buf = NULL;
    uint32_t		size;
    Int status=0;
    //data send
    size=RPE_DATA_SIZE;
    while(rx_flag){
		status=rpe_acquire_writer(prpe,(rpe_buf_t*)&buf,&size);
		if((status&&status!=ERPE_B_BUFWRAP)||size!=RPE_DATA_SIZE||status<0){
	        rpe_release_writer(prpe,0);
	        size=RPE_DATA_SIZE;
	        continue;
		}
        memcpy(buf,(unsigned char*)buf_16,size);	//buf_16: data to be sent
        status=rpe_release_writer_safe(prpe,buf,size);
        if(status<0){
        	log_warn("rpe release writer end failed!");
        	continue;
        }else
        	break;
    }

    return;
}


//24bit-->16bit
void data_extract(unsigned char *input, unsigned char *output)
{
	unsigned int i;

	for(i=0;i<256;i++){
		*output++=*input++;
		*output++=*input++;
		input++;
	}
	return;
}

/*
 *  ======== task_enque ========
 */
Void task_enque(UArg arg0, UArg arg1)
{
	extern audioQueue audioQ;
	extern AudioQueue_DATA_TYPE audioQueueRxBuf[];
	int i=0;
	unsigned short buf_16[REC_BUFSIZE/3] = {0};

	audioQueueInit(&audioQ, AUDIO_QUEUE_RX_LENGTH, audioQueueRxBuf);

	do{
		Semaphore_pend(sem1,BIOS_WAIT_FOREVER);
		data_extract(buf_adc,(unsigned char*)buf_16);
		//printf("task_enque ~~~~~~~~~~~\n");
		//data enqueue
		for(i = 0; i < REC_BUFSIZE/3; i++)
			enAudioQueue(&audioQ, buf_16[i]);

	}while(1);
}

void sys_configure(void)
{
    CSL_SyscfgRegsOvly syscfgRegs = (CSL_SyscfgRegsOvly)CSL_SYSCFG_0_REGS;

    //Select PLL0_SYSCLK2
//    syscfgRegs->CFGCHIP3 &= ~CSL_SYSCFG_CFGCHIP3_ASYNC3_CLKSRC_MASK;
//    syscfgRegs->CFGCHIP3 |= ((CSL_SYSCFG_CFGCHIP3_ASYNC3_CLKSRC_PLL0)
//        						  <<(CSL_SYSCFG_CFGCHIP3_ASYNC3_CLKSRC_SHIFT));
    //mcbsp1
    syscfgRegs->PINMUX1 &= ~(CSL_SYSCFG_PINMUX1_PINMUX1_7_4_MASK |
                             CSL_SYSCFG_PINMUX1_PINMUX1_11_8_MASK |
                             CSL_SYSCFG_PINMUX1_PINMUX1_15_12_MASK |
                             CSL_SYSCFG_PINMUX1_PINMUX1_19_16_MASK |
                             CSL_SYSCFG_PINMUX1_PINMUX1_23_20_MASK |
                             CSL_SYSCFG_PINMUX1_PINMUX1_27_24_MASK |
                             CSL_SYSCFG_PINMUX1_PINMUX1_31_28_MASK);
    syscfgRegs->PINMUX1   = 0x22222220;
    //spi1
    syscfgRegs->PINMUX5 &= ~(CSL_SYSCFG_PINMUX5_PINMUX5_3_0_MASK |
    						 CSL_SYSCFG_PINMUX5_PINMUX5_7_4_MASK |
                             CSL_SYSCFG_PINMUX5_PINMUX5_11_8_MASK |
                             CSL_SYSCFG_PINMUX5_PINMUX5_19_16_MASK |
                             CSL_SYSCFG_PINMUX5_PINMUX5_23_20_MASK );
    syscfgRegs->PINMUX5   |= 0x00110111;

    syscfgRegs->PINMUX6 &= ~(CSL_SYSCFG_PINMUX6_PINMUX6_23_20_MASK);
    syscfgRegs->PINMUX6  |= 0x00800000;

    syscfgRegs->PINMUX7  &= ~(CSL_SYSCFG_PINMUX7_PINMUX7_11_8_MASK|
    						CSL_SYSCFG_PINMUX7_PINMUX7_15_12_MASK);
    syscfgRegs->PINMUX7  |= 0X00008800;

//    syscfgRegs->PINMUX10 &= ~(CSL_SYSCFG_PINMUX10_PINMUX10_23_20_MASK|
//    						 CSL_SYSCFG_PINMUX10_PINMUX10_31_28_MASK);
//    syscfgRegs->PINMUX10 |= 0x80800000;
    syscfgRegs->PINMUX13 &= ~(CSL_SYSCFG_PINMUX13_PINMUX13_11_8_MASK|
    						CSL_SYSCFG_PINMUX13_PINMUX13_15_12_MASK);
    syscfgRegs->PINMUX13 |= 0x00008800;
    syscfgRegs->PINMUX14 &= ~(CSL_SYSCFG_PINMUX14_PINMUX14_3_0_MASK|
    						CSL_SYSCFG_PINMUX14_PINMUX14_7_4_MASK);
    syscfgRegs->PINMUX14 |= 0x00000088;


//    //GP2[2]	DSC_LE
//    CSL_FINS(gpioRegs->BANK[GP2].DIR,GPIO_DIR_DIR2,0);
////    CSL_FINS(gpioRegs->BANK[1].OUT_DATA,GPIO_OUT_DATA_OUT2,0);
//    CSL_FINS(gpioRegs->BANK[GP2].OUT_DATA,GPIO_OUT_DATA_OUT2,1);
    //GP4[0]	MUX
//    CSL_FINS(gpioRegs->BANK[2].DIR,GPIO_DIR_DIR0,1);
//    //GP4[2]	T/R
//    CSL_FINS(gpioRegs->BANK[2].DIR,GPIO_DIR_DIR2,0);
//    CSL_FINS(gpioRegs->BANK[2].OUT_DATA,GPIO_OUT_DATA_OUT2,1);
//    CSL_FINS(gpioRegs->BANK[2].OUT_DATA,GPIO_OUT_DATA_OUT2,0);

    //GP3[12] LNA_ATT_EN
//    CSL_FINS(gpioRegs->BANK[1].DIR,GPIO_DIR_DIR28,0);
//    CSL_FINS(gpioRegs->BANK[1].OUT_DATA,GPIO_OUT_DATA_OUT28,0);
    //GP3[13] IF_AGC_CTL
    CSL_FINS(gpioRegs->BANK[1].DIR,GPIO_DIR_DIR29,0);
    CSL_FINS(gpioRegs->BANK[1].OUT_DATA,GPIO_OUT_DATA_OUT29,1);

    //GP6[13]
    CSL_FINS(gpioRegs->BANK[3].DIR,GPIO_DIR_DIR13,0);
//    CSL_FINS(gpioRegs->BANK[3].OUT_DATA,GPIO_OUT_DATA_OUT13,0);
    CSL_FINS(gpioRegs->BANK[3].OUT_DATA,GPIO_OUT_DATA_OUT13,1); //R:F2
    //GP6[6] 	TX_EN:1 RX_EN:0
    CSL_FINS(gpioRegs->BANK[3].DIR,GPIO_DIR_DIR6,0);
    CSL_FINS(gpioRegs->BANK[3].OUT_DATA,GPIO_OUT_DATA_OUT6,0);

//    CSL_FINS(gpioRegs->BANK[3].DIR,GPIO_DIR_DIR7,0);//RX_SW
//    CSL_FINS(gpioRegs->BANK[3].OUT_DATA,GPIO_OUT_DATA_OUT7,1);

}

void ch_chPara(){
	reg_24 reg_24data;
	uint32_t tempCount=0;


	for (tempCount = 0; tempCount < 36; tempCount++){
	    reg_24data.all=lmx_init[45+tempCount/3];
	    buf_transmit[tempCount++] = reg_24data.dataBit.data0;
	    buf_transmit[tempCount++] = reg_24data.dataBit.data1;
	    buf_transmit[tempCount]   = reg_24data.dataBit.data2;
	}
}

void dsp_logic(message_t *msg_send)
{
	int status=NO_ERR;
	message_t msg_temp;
	int ad_ch;
	float ad_value;
	char str_temp[32];
	char* pstr=NULL;
	char* poffs=NULL;
    message_t *msg =NULL;

    //
    status=messageq_receive(&msgq[0],&msg,0);
    if (status>=0){
    	msg_temp.type=msg->type;
    	strcpy(msg_temp.data.d,msg->data.d);
    	log_info("message type	  is %d",msg->type);
    	log_info("message content is %s",msg->data.d);
    	msg->data.d[0]='\0';
    }
    if(status>=0){
    		switch (msg_temp.type){
    		case LMX2571_TX:
    			CSL_FINS(gpioRegs->BANK[3].OUT_DATA,GPIO_OUT_DATA_OUT6,1);
    			CSL_FINS(gpioRegs->BANK[3].OUT_DATA,GPIO_OUT_DATA_OUT13,0); //T:F2
    		    //TX_DAC
//    		    dac_write(3, 1.633);
    			rx_flag=0;
    			tx_flag=1;
    			break;
    		case LMX2571_RX:
//    			dac_write(3, 0);

    			CSL_FINS(gpioRegs->BANK[3].OUT_DATA,GPIO_OUT_DATA_OUT6,0);
    			CSL_FINS(gpioRegs->BANK[3].OUT_DATA,GPIO_OUT_DATA_OUT13,1); //R:F1

				tx_flag=0;
				tx_submit=0;
				memset(buf_transmit,0x80,RPE_DATA_SIZE/2*15);
    			rx_flag=1;
    			break;
    		case RSSI_RD:
    			while(msg_send->data.d[0]!='\0'){
    				Task_sleep(10);
    				log_warn("message is not empty");
    			}
    			msg_send->type=RSSI_RD;
    			sprintf(msg_send->data.d,"%lf",RSSI_db);
    			status=messageq_send(&msgq[0],(messageq_msg_t)msg_send,0,0,0);
    			if(status<0){
    				log_error("message send error");
    			}
    			break;
    		case IF_AGC_ON:
    			CSL_FINS(gpioRegs->BANK[1].OUT_DATA,GPIO_OUT_DATA_OUT29,1);
    			break;
    		case IF_AGC_OFF:
    		    CSL_FINS(gpioRegs->BANK[1].OUT_DATA,GPIO_OUT_DATA_OUT29,0);
    		    break;
    		case LNA_ATT_ON:
    			CSL_FINS(gpioRegs->BANK[1].OUT_DATA,GPIO_OUT_DATA_OUT28,1);
    			break;
    		case LNA_ATT_OFF:
    		    CSL_FINS(gpioRegs->BANK[1].OUT_DATA,GPIO_OUT_DATA_OUT28,0);
    		    break;
    		case TX_ON:
    			CSL_FINS(gpioRegs->BANK[3].OUT_DATA,GPIO_OUT_DATA_OUT6,1);
    			break;
    		case TX_OFF:
    			CSL_FINS(gpioRegs->BANK[3].OUT_DATA,GPIO_OUT_DATA_OUT6,0);
    			break;
    		case RX_ON:
    			CSL_FINS(gpioRegs->BANK[3].OUT_DATA,GPIO_OUT_DATA_OUT7,1);
    			break;
    		case RX_OFF:
    			CSL_FINS(gpioRegs->BANK[3].OUT_DATA,GPIO_OUT_DATA_OUT7,0);
    			break;
    		case LMX2571_LD:
    			while(msg_send->data.d[0]!='\0'){
    				Task_sleep(10);
    				log_warn("message is not empty");
    			}
    			msg_send->type=LMX2571_LD;
    			sprintf(msg_send->data.d,"%d",CSL_FEXT(gpioRegs->BANK[3].IN_DATA,GPIO_IN_DATA_IN12));
    			log_info("lmx2571_ld is %s",msg_send->data.d);
    			status=messageq_send(&msgq[0],(messageq_msg_t)msg_send,0,0,0);
    			if(status<0){
    				log_error("message send error");
    			}
    			break;
    		case TX_CH:
    			log_info("tx_ch:%f",atof(msg_temp.data.d));
    			LMX2571_FM_CAL(1,atof(msg_temp.data.d));
    			lmx_init[53]=0xB83;
    			lmx_init[54]=lmx_init[55]=0x983;
    			lmx_init[56]=0x983;
    			ch_chPara();
    			Task_sleep(10);
    			memset(buf_transmit,0x80,RPE_DATA_SIZE/2*15);
    			break;
    		case RX_CH:
    			log_info("rx_ch:%f",10.7+atof(msg_temp.data.d));
    			LMX2571_FM_CAL(0,10.7+atof(msg_temp.data.d));
    			lmx_init[53]=0xBC3;
    			lmx_init[54]=lmx_init[55]=0x9C3;
    			lmx_init[56]=0x9C3;
    			ch_chPara();
    			Task_sleep(10);
    			memset(buf_transmit,0x80,RPE_DATA_SIZE/2*15);
    			break;
    		case AMC7823_AD:
    			ad_ch=atoi(msg_temp.data.d);
    			if(ad_ch>8||ad_ch<0)
    				log_error("error ad_ch parameter %d",ad_ch);
    			ad_value=adc_read(ad_ch);
    			while(msg_send->data.d[0]!='\0'){
    			    Task_sleep(10);
    			    log_warn("message is not empty");
    			}
    			msg_send->type=AMC7823_AD;
    			sprintf(msg_send->data.d,"%f",ad_value);
    			status=messageq_send(&msgq[0],(messageq_msg_t)msg_send,0,0,0);
    			if(status<0){
    			    log_error("message send error");
    			}
    			break;
    		case AMC7823_DA:
    			poffs=strstr(msg_temp.data.d,":");
    			strncpy(str_temp,msg_temp.data.d,poffs-msg_temp.data.d);
    			str_temp[poffs-msg_temp.data.d]='\0';
    			ad_ch=atoi(str_temp);
    			if(ad_ch>8||ad_ch<0)
    				log_error("error ad_ch parameter %d",ad_ch);
    			pstr=poffs+1;
//    			poffs=strstr(pstr,":");
//    			strncpy(str_temp,pstr,poffs-pstr);
    			strcpy(str_temp,pstr);
//    			str_temp[poffs-pstr]='\0';
    			ad_value=atof(str_temp);
    			dac_write(ad_ch,ad_value);
    			break;
    		case PA_TEMP:
    			ad_value=temperature_read();
    		    msg_send->type=PA_TEMP;
    		    sprintf(msg_send->data.d,"%f",ad_value);
    		    status=messageq_send(&msgq[0],(messageq_msg_t)msg_send,0,0,0);
    		    if(status<0){
    		    	log_error("message send error");
    		    }
    		    break;
    		case TX_CHF:
    			log_info("tx_ch:%f",atof(msg_temp.data.d));
    			LMX2571_FM_CAL(1,atof(msg_temp.data.d));
    			lmx_init[53]=0xB83;
    			lmx_init[54]=lmx_init[55]=0x983;
    			lmx_init[56]=0x983;
    			ch_chPara();
    			Task_sleep(10);
    			memset(buf_transmit,0x80,RPE_DATA_SIZE/2*15);
    			break;
    		case RX_CHF:
    			log_info("rx_ch:%f",10.7+atof(msg_temp.data.d));
    			LMX2571_FM_CAL(0,10.7+atof(msg_temp.data.d));
    			lmx_init[53]=0xBC3;
    			lmx_init[54]=lmx_init[55]=0x9C3;
    			lmx_init[56]=0x9C3;
    			ch_chPara();
    			Task_sleep(10);
    			memset(buf_transmit,0x80,RPE_DATA_SIZE/2*15);
    			break;
    		case H_TX:
    			dac_write(3, 1.633);
    			break;
    		case L_TX:
    			dac_write(3, 0.8);
    			break;
    		case RSSTH:
    			RXSS_THRESHOLD=atoi(msg_temp.data.d);
    			break;
    		case RXSSI:
    			while(msg_send->data.d[0]!='\0'){
    				Task_sleep(10);
    				log_warn("message is not empty");
    			}
    			msg_send->type=RXSSI;
    			sprintf(msg_send->data.d,"%f",RSSI_db);
    			log_info("lmx2571_ld is %f",msg_send->data.d);
    			status=messageq_send(&msgq[0],(messageq_msg_t)msg_send,0,0,0);
    			if(status<0){
    				log_error("message send error");
    			}
    			break;
    		case TXPI:
    			while(msg_send->data.d[0]!='\0'){
    				Task_sleep(10);
    				log_warn("message is not empty");
    			}
    			msg_send->type=TXPI;
    			ad_value=adc_read(6);
    			sprintf(msg_send->data.d,"%f",ad_value);
    			status=messageq_send(&msgq[0],(messageq_msg_t)msg_send,0,0,0);
    			if(status<0){
    			    log_error("message send error");
    			}
    			break;
    		default:
    			log_error("unknown message type");
    			break;
    		}
    }
}


