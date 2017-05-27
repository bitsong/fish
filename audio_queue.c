#include <stdio.h>
#include "audio_queue.h"


/* 全局变量 */
//#define AUDIO_QUEUE_RX_LENGTH   131072
AudioQueue_DATA_TYPE audioQueueRxBuf[AUDIO_QUEUE_RX_LENGTH] = {0};





/*
 * 功能：初始化队列
 */
void audioQueueInit(audioQueue * q, Int size, AudioQueue_DATA_TYPE *buf)
{
	q->front = 0;
	q->rear = 0;
	q->capacity = 0;
	q->size = size;
	q->buf = buf;
}

/*
 * 功能：获取队列长度
 */
Int audioQueueLength(audioQueue *q)
{
//	printf("length return %d %d\n",q->capacity,q->size);
	return q->capacity;
}

/*
 * 功能：将data放入队尾
 */
state enAudioQueue(audioQueue * q, AudioQueue_DATA_TYPE data)
{

	//如果队列满
    if(q->capacity >= q->size)
    {
//		printf("Warming:The AudioQueue is full %d!\n",q->size);
		return ERROR;
    }

    q->buf[q->rear] = data;
    q->rear = (q->rear+1) % q->size;
	q->capacity ++;
    return OK;
}

/*
 * 功能：将队列头的元素弹出队列
 */
AudioQueue_DATA_TYPE deAudioQueue(audioQueue * q)
{
	AudioQueue_DATA_TYPE data = 0;
	//printf("deAudioQueue cap=%d!!!!!!!!!!!\n",q->capacity);
	//如果队列空
    if(q->capacity == 0)
    {
		printf("Warming:the AudioQueue is empty!\n");
        return ERROR;
    }

    data = q->buf[q->front];
    q->front = (q->front+1) % q->size;
	q->capacity --;

    return data;	
}


/*
 * 功能：获取队头所在位置
 */
Int getAudioQFrontPosition(audioQueue *q)
{
	return q->front;
}


/*
 * 功能：访问队列中position位置的元素
 */
AudioQueue_DATA_TYPE audioQueuePoll(audioQueue *q,Int position)
{
	return q->buf[position  % q->size];
}


void audioQueueFlush(audioQueue *q)
{
	q->front = 0;
	q->rear = 0;
	q->capacity = 0;
}

