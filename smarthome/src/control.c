#include "control.h"
#include <stdio.h>

struct control* add_interface_to_ctrl_list(struct control* phead,struct control* interface_control){
    if(phead == NULL){//如果链表为空 
        phead = interface_control;
    }else{//不为空将当前节点指向链表头结点
        interface_control->next = phead;
        phead  = interface_control;
    }
    return phead;
}