typedef struct{
    mqd_t mqd;//消息队列描述符
    struct control *ctrl_phead;

}ctrl_info_t;