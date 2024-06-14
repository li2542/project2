#include <stdio.h>
#include <wiringPi.h>
#include "gdevice.h"


struct gdevice* add_device_to_gdevice_list(struct gdevice* phead,struct gdevice* interface_control){
    if(phead == NULL){//如果链表为空 
        phead = interface_control;
    }else{//不为空将当前节点指向链表头结点
        interface_control->next = phead;
        phead  = interface_control;
    }
    return phead;
}

struct gdevice* find_device_by_key(struct gdevice *pdevhead,int key){
    struct gdevice *p = NULL;
    if(NULL == pdevhead){
        return NULL;
    }
    p = pdevhead;
    while(p != NULL){
        if(p->key == key){
            return p;
        }
        p= p->next;
    }
    return NULL;
}

int set_gpio_gdevice_status(struct gdevice* pdev){
    if(NULL == pdev){
        return -1;
    }

    if(-1 != pdev->gpio_pin){
        if (-1 != pdev->gpio_mode)
        {
            pinMode(pdev->gpio_pin,pdev->gpio_mode);//配置引脚的输入输出模式
        }

        if (-1 != pdev->gpio_status)
        {
            digitalWrite(pdev->gpio_pin,pdev->gpio_status);//当配置为输出模式时，引脚的高低状态
        }
    }
    return 0;
}