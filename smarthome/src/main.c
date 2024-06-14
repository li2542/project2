#include <stdio.h>
#include <pthread.h>
#include "voice_interface.h"
#include "socket_interface.h"
#include "smoke_interface.h"
#include "receive_interface.h"
#include "msg_queue.h"
//#include "control.h"
#include "global.h"
#include <stdlib.h>
#include <wiringPi.h>



int main(int argc,char *argv[]){
    
    wiringPiSetup();

    pthread_t thread_id;
    struct control *control_phead = NULL;
    struct control *pointer = NULL;

    ctrl_info_t *ctrl_info = NULL;
    ctrl_info  =(ctrl_info_t*)malloc(sizeof(ctrl_info_t));
    ctrl_info->ctrl_phead = NULL;//先做初始化
    ctrl_info->mqd = -1;

    int node_num = 0;

    ctrl_info->mqd = msg_queue_create();
    if(ctrl_info->mqd == -1){//创建消息队列失败
        perror("ctrl_info->mqd");
        return -1;
    }

    ctrl_info->ctrl_phead = add_voice_to_ctrl_list(ctrl_info->ctrl_phead);//将语音模块头插法方式插入链表
    ctrl_info->ctrl_phead = add_tcpsocket_to_ctrl_list(ctrl_info->ctrl_phead);//将socket连接数头插法方式插入链表
    ctrl_info->ctrl_phead = add_smoke_to_ctrl_list(ctrl_info->ctrl_phead);//将连接数头插法方式插入链表
    ctrl_info->ctrl_phead = add_receive_to_ctrl_list(ctrl_info->ctrl_phead);

    pointer = ctrl_info->ctrl_phead;
    while(pointer != NULL){
        if(pointer->init != NULL){
            printf("%s|%s|%d:pointer->control_name=%s\n", __FILE__, __func__, __LINE__, pointer->control_name);
            pointer->init();//pointer->init意思是指向函数指针init,然后再结合()执行函数，为每一个节点做初始化
        }
        
        pointer = pointer->next;
        node_num++;//统计节点个数
    }

    pthread_t *tid = malloc(sizeof(int) * node_num);

    pointer = ctrl_info->ctrl_phead;
    for(int i = 0;i< node_num;i++){//开启线程
        if(pointer->get !=NULL){
            printf("%s|%s|%d:pointer->control_name=%s\n", __FILE__, __func__, __LINE__, pointer->control_name);
            pthread_create(&tid[i],NULL,(void*)pointer->get,(void*)ctrl_info);
        }
        pointer=pointer->next;  
    }

    for(int i = 0;i<node_num;i++){//
        pthread_join(tid[i],NULL);
    }
    for(int i =0;i< node_num;i++){//关闭串口
        if(NULL != pointer->final){
            pointer->final();
        }
        pointer=pointer->next;
    }

    msg_queue_final(ctrl_info->mqd);
    return 0;
}