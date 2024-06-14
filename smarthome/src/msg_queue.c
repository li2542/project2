#include "msg_queue.h"

#define QUEQUE_NAME "/test_queue"

mqd_t msg_queue_create(){
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
    
    printf("%s| %s |%d: mqd=%d\n",__FILE__,__func__,__LINE__,mqd);
    return mqd;
}
//发送消息进消息队列函数
int send_message(mqd_t mqd,void* msg,int msg_len){
    int byte_send = -1;
    byte_send = mq_send(mqd,(char*)msg,msg_len,0);
    return byte_send;
}

void msg_queue_final(mqd_t mqd){
    if(mqd !=-1){
        mq_close(mqd);
    }
    mq_unlink(QUEQUE_NAME);
    mqd = -1;
}

