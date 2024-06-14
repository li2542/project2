#include "receive_interface.h"
// #include "lrled_gdevice.h"
// #include "bled_gdevice.h"
// #include "beep_gdevice.h"
// #include "lock_gdevice.h"

#include "msg_queue.h"
#include "uartTool.h"
#include <pthread.h>
#include "global.h"
#include "socket.h"
#include "face.h"
#include "myoled.h"
#include "gdevice.h"
#include "ini.h"

typedef struct {
    int msg_len;//消息长度
    unsigned char *buffer;//缓冲区，存储消息
    ctrl_info_t *ctrl_info;
}recv_msg_t;

static int oled_fd = -1;
static struct gdevice *pdevhead = NULL;


#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

static int handler(void* user, const char* section, const char* name,
                   const char* value)
{
    //printf("section=%s,name=%s,value=%s\n",section,name,value);
    struct gdevice *pdev = NULL;
    if(pdevhead == NULL){
        pdevhead = (struct gdevice *)malloc(sizeof(struct gdevice));
        pdevhead->next = NULL;
        memset(pdevhead,'\0',sizeof(struct gdevice));
        strcpy(pdevhead->dev_name,section);
    }else if(0 != strcmp(section,pdevhead->dev_name)){
        pdev = (struct gdevice *)malloc(sizeof(struct gdevice));
        memset(pdev,'\0',sizeof(struct gdevice));
        strcpy(pdev->dev_name,section);
        pdev->next = pdevhead;
        pdevhead = pdev;
    }

    if(pdevhead != NULL){
        if(MATCH(pdevhead->dev_name,"key")){
            sscanf(value,"%x",&pdevhead->key);
            printf("%d|pdevhead->key=%x\n",__LINE__,pdevhead->key);
        }else if(MATCH(pdevhead->dev_name,"gpio_pin")){
            pdevhead->gpio_pin = atol(value);
        }else if(MATCH(pdevhead->dev_name,"gpio_mode")){
            if(strcmp(value,"OUTPUT") == 0){
                pdevhead->gpio_mode = OUTPUT;
            }else if(strcmp(value,"INPUT") == 0){
                pdevhead->gpio_mode = INPUT;
            }
        }else if(MATCH(pdevhead->dev_name,"gpio_status")){
            if(strcmp(value,"LOW ") == 0){
                pdevhead->gpio_status = LOW;
            }else if(strcmp(value,"HIGH") == 0){
                pdevhead->gpio_status = HIGH;
            }
        }else if(MATCH(pdevhead->dev_name,"check_face_status")){
            pdevhead->check_face_status = atoi(value);
        }else if(MATCH(pdevhead->dev_name,"voice_set_status")){
            pdevhead->voice_set_status = atoi(value);
        }
    }
    return 1;
}

static int receive_init(){
    // pdevhead = add_lrled_to_gdevice_list(pdevhead);
    // pdevhead = add_bled_to_gdevice_list(pdevhead);
    // pdevhead = add_beep_to_gdevice_list(pdevhead);
    // pdevhead = add_lock_to_gdevice_list(pdevhead);
    if (ini_parse("/etc/gdevice.ini", handler, NULL) < 0) {
        printf("Can't load 'gdevice.ini'\n");
        return 1;
    }

    struct gdevice *pdev = pdevhead;    
    while(pdev){
        printf("pdev->dev_name=%s\n",pdev->dev_name);
        printf("pdev->key=%x\n",pdev->key);
        printf("pdev->gpio_pin=%d\n",pdev->gpio_pin);
        printf("pdev->gpio_mode=%d\n",pdev->gpio_mode);
        printf("pdev->gpio_status=%d\n",pdev->gpio_status);
        printf("pdev->check_face_status=%d\n",pdev->check_face_status);
        printf("pdev->voice_set_status=%d\n",pdev->voice_set_status);

        pdev = pdev->next;
    }

    oled_fd = myoled_init();//初始化oled灯
    face_init();//初始化人脸识别c调python接口代码
    return oled_fd;
}

static void receive_final(){ 
    face_final();//释放缓存，关闭人脸识别
    if(oled_fd != -1){
        close(oled_fd);//关闭oled灯
        oled_fd = -1;
    }
}

//从消息队列拿到消息然后发送给设备，让设备处理
static void *handle_device(void* arg){
    recv_msg_t *recv_msg = NULL;
    struct gdevice *cur_gdev = NULL;
    char success_or_failed[20] = "success";
    int ret = -1;
    pthread_t tid = -1;
    int smoke_status = 0;
    double face_result = 0.0;

    pthread_detach(pthread_self());

    if(arg != NULL){//判断传进来的消息队列
        recv_msg = (recv_msg_t*)arg;
        printf("%s|%s|%d,recv_msg->msg_len=%d\n", __FILE__, __func__,__LINE__, recv_msg->msg_len);
        printf("%s|%s|%d,handle_device_get:0x%x, 0x%x,0x%x, 0x%x, 0x%x,0x%x\n", __FILE__, __func__,
        __LINE__, recv_msg->buffer[0], recv_msg->buffer[1], recv_msg->buffer[2],recv_msg->buffer[3], recv_msg->buffer[4],recv_msg->buffer[5]);
    }
    if(recv_msg !=NULL && NULL != recv_msg->buffer){
        cur_gdev = find_device_by_key(pdevhead,recv_msg->buffer[2]);//根据buffer[2]查找设备
    }
    if(cur_gdev != NULL){
        //引脚状态赋值
        cur_gdev->gpio_status = recv_msg->buffer[3] == 0 ? LOW : HIGH;//LOW低电平，HIGH高电平

        if(1 == cur_gdev->check_face_status){//判断是否需要人脸识别
            face_result = face_category();//调用阿里云人脸识别接口
            if(face_result > 0.6){
                ret = set_gpio_gdevice_status(cur_gdev);//设置引脚状态
                recv_msg->buffer[2] = 0x47;
            }else{
                recv_msg->buffer[2] = 0x46;
                ret = -1;
            }
        }else if(0 == cur_gdev->check_face_status){
            ret = set_gpio_gdevice_status(cur_gdev);
        }
  
        //printf("%s|%s|%d,cur_gdev->voice_set_status=%d\n", __FILE__, __func__,__LINE__, cur_gdev->voice_set_status);
        if(1 == cur_gdev->voice_set_status){//判断是否需要语音播报
            printf("%s|%s|%d,cur_gdev->voice_set_status=%d\n", __FILE__, __func__,__LINE__, cur_gdev->voice_set_status);
            //进行数据判空操作
            if(recv_msg != NULL && recv_msg->ctrl_info != NULL && recv_msg->ctrl_info->ctrl_phead !=NULL){
                //拿到对应设备控制结构体信息
                struct control* pcontrol = recv_msg->ctrl_info->ctrl_phead;
                //对设备模块名字进行判断，成功后发送buffer数据给语音模块进行播报
                while(NULL != pcontrol){
                    printf("%s|%s|%d,pcontrol->control_name=%s\n", __FILE__, __func__,__LINE__, pcontrol->control_name);
                    if(strstr(pcontrol->control_name,"voice")){
                        if(0x45 == recv_msg->buffer[2] && 0 == recv_msg->buffer[3]){
                            
                            smoke_status = -1;
                            
                        }
                        pthread_create(&tid,NULL,pcontrol->set,(void*)(recv_msg->buffer));
                        break;
                    }
                    pcontrol = pcontrol->next;//指向下一个节点
                }
            }
        }
        //下面是oled屏显示
        if(-1 == ret){
            memset(success_or_failed,'\0',sizeof(success_or_failed));
            strncpy(success_or_failed,"failed",6);
        }

        char oled_msg[512];//设置oled信息传输大小
        memset(oled_msg,'\0',sizeof(oled_msg));
        char *change_status = cur_gdev->gpio_status == LOW ? "Open" : "Close";
        sprintf(oled_msg,"%s %s %s!\n",change_status,cur_gdev->dev_name,success_or_failed);
        if(smoke_status == -1){
            memset(oled_msg,0,sizeof(oled_msg));
            strcpy(oled_msg,"A risk of file!\n");
        }
        printf("oled_msg=%s\n",oled_msg);
        oled_show(oled_msg);

        //special for lock, close lock
        if (1 == cur_gdev->check_face_status && 0 == ret && face_result > 0.6)
        {
            sleep(5);
            cur_gdev->gpio_status = HIGH;
            set_gpio_gdevice_status(cur_gdev);
        }
    }
    pthread_exit(0);
}

static void* receive_get(void *arg){
    recv_msg_t *recv_msg = NULL;
    struct mq_attr attr;
    char *buffer = NULL;
    ssize_t read_len = -1;//定义消息读取长度
    pthread_t tid = -1;

    if(arg != NULL){
        recv_msg = (recv_msg_t *)malloc(sizeof(recv_msg_t));
        recv_msg->ctrl_info = (ctrl_info_t *)arg;
        recv_msg->msg_len = -1;
        recv_msg->buffer = NULL;
    }else{
        pthread_exit(0);
    }

    if(mq_getattr(recv_msg->ctrl_info->mqd,&attr) == -1){//拿到设置的消息队列信息
        pthread_exit(0);//获取失败，结束线程
    }
    recv_msg->buffer = (unsigned char *)malloc(attr.mq_msgsize);//为读取数据的缓冲区开辟空间
    buffer = (unsigned char *)malloc(attr.mq_msgsize);
    memset(recv_msg->buffer,0,attr.mq_msgsize);//空间数据初始化
    memset(buffer,0,attr.mq_msgsize);//空间数据初始化
    pthread_detach(pthread_self());
    while (1)
    {
        read_len = mq_receive(recv_msg->ctrl_info->mqd,buffer,attr.mq_msgsize,NULL);//读取消息队列里的数据进buffer,并返回其长度
        printf("%s|%s|%d,_get:0x%x, 0x%x,0x%x, 0x%x, 0x%x,0x%x\n", __FILE__, __func__,
        __LINE__, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4],buffer[5]);
        printf("%s|%s|%d,read_len=%ld\n", __FILE__, __func__,__LINE__, read_len);
        if (read_len == -1)
        {
            if (errno == EAGAIN)
            {
                printf("queue is empty!\n");
            }else{
                break;
            }
            
        }else if(buffer[0] == 0xAA && buffer[1] == 0x55 && buffer[4] == 0x55 && buffer[5] == 0xAA){
            printf("%s|%s|%d,send:0x%x, 0x%x,0x%x, 0x%x, 0x%x,0x%x\n", __FILE__, __func__,
            __LINE__, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4],buffer[5]);
            recv_msg->msg_len = read_len;//设置消息长度
            memcpy(recv_msg->buffer,buffer,read_len);//拷贝数据
            pthread_create(&tid,NULL,handle_device,(void*)recv_msg);//开启一个新线程处理设备,传递封装好的recv_msg过去
        }
         
    }
    
}


struct control receive_control = {
    .control_name = "receive",
    .init = receive_init,
    .final = receive_final,
    .get = receive_get,
    .set = NULL,
    .next = NULL
};

/***
    struct control *phead 要插入的链表的头结点
    
*/
//头插法
struct control *add_receive_to_ctrl_list(struct control *phead){
    
    // if(phead == NULL){//如果链表为空 
    //     phead = &receive_control;
    // }else{//不为空将当前节点指向链表头结点
    //     receive_control.next = phead;
    //     phead  = &receive_control;
    // }
    // return phead;
    return add_interface_to_ctrl_list(phead,&receive_control);
}