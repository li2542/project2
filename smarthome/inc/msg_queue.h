#include <stdio.h>
#include <pthread.h>
#include <mqueue.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

mqd_t msg_queue_create();
int send_message(mqd_t mqd,void* msg,int msg_len);
void msg_queue_final(mqd_t mqd);