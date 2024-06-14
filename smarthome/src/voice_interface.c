#include "voice_interface.h"
#include "msg_queue.h"
#include "uartTool.h"
#include <pthread.h>
#include "global.h"
//#include <stdio.h>

static int serial_fd = -1;

static int voice_init(){//uart5串口初始化
    serial_fd = myserialOpen(SERIAL_DEV,BAUD);
    printf("%s| %s |%d: serial_fd=%d",__FILE__,__func__,__LINE__,serial_fd);
    return serial_fd;
}

static void voice_final(){
    if(serial_fd != -1){
        close(serial_fd);
        serial_fd = -1;
    }
}

//接受语音指令
static void* voice_get(void *arg){ 
    mqd_t mqd = -1;
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0X55, 0xAA};
    int len = 0;
    printf("%s|%s|%d\n", __FILE__, __func__, __LINE__);
    if (-1 == serial_fd)//如果串口没有打开
    {
        serial_fd = voice_init();//打开uart5串口
        if(serial_fd == -1){
            pthread_exit(0);
        } 
    }

    if(NULL != arg){
        mqd = ((ctrl_info_t *)arg)->mqd;//拿到消息队列描述符

    }
    if(mqd == -1){
        pthread_exit(0);
    }
    
    pthread_detach(pthread_self());//子线程结束之后会自己去释放本·线程的资源
    printf("%s thread start\n",__func__);
    while(1)
    {
        len = serialGetstring(serial_fd, buffer);//如果没有数据传过来，线程会一直卡在read函数那里等待数据，
        //但是打开的串口会隔一段时间超时，导致read函数读取到0数据，函数继续往下走
        printf("%s|%s|%d:0x%x, 0x%x,0x%x, 0x%x, 0x%x,0x%x\n", __FILE__, __func__,
        __LINE__, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4],buffer[5]);
        printf("%s|%s|%d, len=%d\n", __FILE__, __func__, __LINE__,len);
        if (len > 0 )
        {
            if(buffer[0] == 0xAA && buffer[1] == 0x55 && buffer[4] == 0x55 && buffer[5] == 0xAA ){
                printf("%s|%s|%d:0x%x, 0x%x,0x%x, 0x%x, 0x%x,0x%x\n", __FILE__, __func__,
                __LINE__, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4],buffer[5]);
                send_message(mqd,buffer,len);
            }
            memset(buffer,0,sizeof(buffer));
        }
    }

    pthread_exit(0);
    return NULL;
}

//语音播报
static void* voice_set(void *arg){
    pthread_detach(pthread_self());
    unsigned char *buffer = (unsigned char *)arg;
    if (-1 == serial_fd)
    {
        serial_fd = voice_init();
        if (-1 == serial_fd)
        {
            pthread_exit(0);
        }
    }
    if (NULL != buffer)
    {
        serialSendstring(serial_fd, buffer, 6);
    }
    pthread_exit(0);
}


struct control voice_control = {
    .control_name = "voice",
    .init = voice_init,
    .final = voice_final,
    .get = voice_get,
    .set = voice_set,
    .next = NULL
};
/***
    struct control *phead 要插入的链表的头结点
    
*/
//头插法
struct control *add_voice_to_ctrl_list(struct control *phead){
    
    // if(phead == NULL){//如果链表为空
    //     phead = &voice_control;
    // }else{//不为空将当前节点指向链表头结点
    //     voice_control.next = phead;
    //     phead  = &voice_control;
    // }
    // return phead;
    return add_interface_to_ctrl_list(phead,&voice_control);
}