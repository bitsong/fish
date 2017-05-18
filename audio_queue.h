#ifndef _AUDIO_QUEUE_H
#define _AUDIO_QUEUE_H
#include <xdc/std.h>


#define AUDIO_QUEUE_RX_LENGTH   8192

#ifndef AudioQueue_DATA_TYPE
#define AudioQueue_DATA_TYPE     unsigned short
#endif

#define OK 1  
#define FAIL 0
#define ERROR 0
//#define TRUE 1  
//#define FALSE 0
typedef int state;

typedef struct audioQueue {
    Int      front,rear;
    Int      capacity;
    Int      size;
	AudioQueue_DATA_TYPE    *buf;
} audioQueue;



void audioQueueInit(audioQueue * q, Int size, AudioQueue_DATA_TYPE *buf);
state enAudioQueue(audioQueue * q, AudioQueue_DATA_TYPE data);
AudioQueue_DATA_TYPE deAudioQueue(audioQueue * q);
Int audioQueueLength(audioQueue *q);
Int getAudioQFrontPosition(audioQueue *q);
AudioQueue_DATA_TYPE audioQueuePoll(audioQueue *q,Int position);
void audioQueueFlush(audioQueue *q);
//void removeRearPosition(audioQueue *q);

#endif

