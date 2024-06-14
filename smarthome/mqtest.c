#include <stdio.h>
#include <pthread.h>
#include <mqueue.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define QUEQUE_NAME "/test_queue"
#define MESSAGE "mqueque,test!"

void *sender_thread(void *arg){//发送消息线程

    //发送消息
    mqd_t mqd = *(mqd_t*)arg;
    // for (int i = 0; i < 11; i++)
    // {
        char message[128] = MESSAGE;
        // sprintf(message,"mqueque,test!+%d",i);
        //message = MESSAGE;
        printf("sender thread message=%s,mqd=%d\n",message,mqd);
        //mq_send(mqd,message,strlen(message)+1,0);
        if((mq_send(mqd,message,strlen(message)+1,0))== -1){
            if(errno == EAGAIN){
                printf("message queue is full\n");
            }else{
                perror("mq_send");
            }
        }
    // }
    
    
    
    return NULL;
}

void *receiver_thread(void *arg){//接受消息线程
    //sleep(15);  
    mqd_t mqd = *(mqd_t*)arg;
    //接受消息
    char buffer[256];
    ssize_t receiver_num = 0;
    printf("receive thread start\n");
    
    receiver_num = mq_receive(mqd,buffer,sizeof(buffer),NULL);
    printf("receiver thread message=%s,mqd=%d,receiver_num=%ld\n",buffer,mqd,receiver_num);
    return NULL;
}


int main(int argc,char *argv[]){
    pthread_t sender,receiver;

    //创建消息队列
    mqd_t mqd = -1;

    //设置消息队列属性
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 250;
    attr.mq_curmsgs = 0;

    //打开或创建一个消息队列,返回值是消息队列描述符
    mqd = mq_open(QUEQUE_NAME,O_CREAT|O_RDWR,0666,&attr);
    if (mqd == (mqd_t)-1)
    {
        perror("mq_open");
        return -1;
    }
    

    if(pthread_create(&sender,NULL,sender_thread,(void*)&mqd) != 0){
        perror("pthread_create sender");
        return -1;
    }

    // if(pthread_create(&receiver,NULL,receiver_thread,(void*)&mqd) != 0){
    //     perror("pthread_create receiver");
    //     return -1;
    // }
    
    pthread_join(sender,NULL);//阻塞主线程，等sender线程执行完了之后主线程才会继续执行
    // pthread_join(receiver,NULL);
    mq_close(mqd);
    //mq_unlink(QUEQUE_NAME);

    return 0;
}