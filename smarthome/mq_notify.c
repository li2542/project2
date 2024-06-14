#include <stdio.h>
#include <pthread.h>
#include <mqueue.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

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

// void *receiver_thread(void *arg){//接受消息线程
//     //sleep(15);  
//     mqd_t mqd = *(mqd_t*)arg;
//     //接受消息
//     char buffer[256];
//     ssize_t receiver_num = 0;
//     printf("receive thread start\n");
    
//     receiver_num = mq_receive(mqd,buffer,sizeof(buffer),NULL);
//     printf("receiver thread message=%s,mqd=%d,receiver_num=%ld\n",buffer,mqd,receiver_num);
//     return NULL;
// }

void notify_thread(union sigval arg){
    // 获取消息队列描述符
    mqd_t mqd = -1;
    mqd = *((mqd_t *)arg.sival_ptr);
    // 定义一个缓冲区，用于存储接收到的消息
    char buffer[256];
    memset(buffer,0,sizeof(buffer));
    // 定义一个变量，用于存储接收到的消息的大小
    ssize_t bytes_read;
    // 定义一个结构体，用于重新注册消息队列的通知
    struct sigevent sev;
    // 打印提示信息
    printf("notify_thread started, mqd=%d\n", mqd);
    // 循环接收消息，直到队列为空
    while (1)
    {
        // 从消息队列中接收消息
        bytes_read = mq_receive(mqd, buffer, 256, NULL);
        // 如果接收失败，检查错误码
        if (bytes_read == -1)
        {
        // 如果错误码是EAGAIN，说明队列为空，跳出循环
            if (errno == EAGAIN)
            {
                printf("queue is empty\n");
                break;
            }// 否则，打印错误信息，退出程序
            else
            {
                perror("mq_receive");
                exit(1);
            }
        }
    // 如果接收成功，打印接收到的消息的大小和内容
    printf("read %ld bytes: %s\n", (long)bytes_read, buffer);
    }
    // 重新注册消息队列的通知，使用同样的回调函数和参数
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = notify_thread;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = &mqd;
    if (mq_notify(mqd, &sev) == -1)
    {
        perror("mq_notify");
        exit(1);
    }
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

    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_value.sival_ptr = &mqd;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_notify_function = notify_thread; //回调函数
    if (mq_notify(mqd, &sev) == -1)
    {
        perror("mq_notify");
        exit(1);
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
    sleep(5);
    mq_close(mqd);
    mq_unlink(QUEQUE_NAME);

    return 0;
}