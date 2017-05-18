#ifndef __SYSLINK_PREPARE_COMMON_H
#define __SYSLINK_PREPARE_COMMON_H

#define SYSBIOS					//所使用的操作系统，linux:SYSLINUX , sysbios：SYSBIOS


#if defined (SYSLINUX)
#include <unistd.h>
#include "debug.h"
#else

#endif

#include "syslink.h"
/*---------------------------------------------------------*/
/*重试次数、重试间隔*/
#define TRYTIMES				100
#define TYRINTERVAL				500

#define SYNC_WAIT_TIMEOUT		60 * 1000000	//60s
#define SYNC_WAIT_TRYTIMES		2			//最大等待两分钟

/*批量初始化个数*/
#define RPE_MAX					1
#define NTF_MAX					0
#define MSG_MAX					2
#define MBUF_MAX				1		//不能超过8个	
/*---------------------------------------------------------*/

#define MAX_NAME_LEN			128

/*option 标志位*/
#define O_RD					(1U << 0)
#define O_WR					(1U << 1)
#define O_IGN					(1U << 31)
#define O_CBUF					(1U << 2)
#define O_RW 					(O_RD | O_WR)
#define O_RCV					O_RD
#define O_SND					O_WR
#define O_RS					O_RW

/*cpu name、rpe name*/
#define arm_proc_name			"HOST"
#define dsp_proc_name			"DSP"
#define armbuf_base_name		"buf_arm"		//message buffer base name
#define dspbuf_base_name		"buf_dsp"
#define armslk_base_name		"slk_arm"
#define dspslk_base_name		"slk_dsp"
#define armntf_base_name		"ntf_arm"
#define dspntf_base_name		"ntf_dsp"
#define armrpe_base_name		"rpe_arm"
#define dsprpe_base_name		"rpe_dsp"
#define armmsg_base_name		"msg_arm"
#define dspmsg_base_name		"msg_dsp"


/*单位*/
#define KB						1024
#define MS						1000


#ifdef  SYSLINUX

#define msleep(n)				usleep((n) * MS)
#define warn( format,args...)	log_warn( format,##args)//?????
#define info( format,args...)	log_info( format,##args)
#define error(format,args...)	log_error(format,##args)

#define slk_role				PROC_ROLE_HOST

/*arm 本地cpu名称、对象名称前缀*/
#define local_proc_name			arm_proc_name
#define local_slk_basenm		armslk_base_name
#define local_ntf_basenm		armntf_base_name
#define local_buf_basenm		armbuf_base_name
#define local_msg_basenm		armmsg_base_name
#define local_rpe_basenm		armrpe_base_name
/*arm 对端cpu名称、对象名称前缀*/
#define remote_proc_name		dsp_proc_name
#define remote_slk_basenm		dspslk_base_name
#define remote_ntf_basenm		dspntf_base_name
#define remote_buf_basenm		dspbuf_base_name
#define remote_msg_basenm		dspmsg_base_name
#define remote_rpe_basenm 		dsprpe_base_name
	
#else

/*dsp 本地cpu名称、对象名称前缀*/
#define local_proc_name			dsp_proc_name
#define local_slk_basenm		dspslk_base_name
#define local_ntf_basenm		dspntf_base_name
#define local_buf_basenm		dspbuf_base_name
#define local_msg_basenm		dspmsg_base_name
#define local_rpe_basenm		dsprpe_base_name
/*dsp 对端cpu名称、对象名称前缀*/
#define remote_proc_name		arm_proc_name
#define remote_slk_basenm		armslk_base_name
#define remote_ntf_basenm		armntf_base_name
#define remote_buf_basenm		armbuf_base_name
#define remote_msg_basenm		armmsg_base_name
#define remote_rpe_basenm 		armrpe_base_name
	
#define msleep(n)				Task_sleep(n)
//
//#define  info(format, ...)      \
//    do {                           \
//        printf("[%s|%s@%s,%d] " format "\n", "INFO ", __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
//    } while (0)
//#define  warn(format, ...)      \
//    do {                           \
//        printf("[%s|%s@%s,%d] " format "\n", "WARN ", __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
//    } while (0)
//#define  error(format, ...)      \
//    do {                           \
//        printf("[%s|%s@%s,%d] " format "\n", "ERROR ", __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
//    } while (0)

#define slk_role				PROC_ROLE_SLAVE

#endif

#define SYNC_CODE_MSGOK			1
#define SYNC_CODE_RPEOK			3
#define SYNC_CODE_NTFOK			4

#define SYNC_CODE_MSGOUT		9
#define SYNC_CODE_NTFOUT		7
#define SYNC_CODE_RPEOUT		6

#define SYNC_CODE_OUT0			11
#define SYNC_CODE_ACK0			19



#endif
