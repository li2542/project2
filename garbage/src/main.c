#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <wiringPi.h>
#include <pthread.h>

#include "socket.h"
#include "pwm.h"
#include "uartTool.h"
#include "garbage.h"
#include "myoled.h"

int serial_fd = -1;
pthread_cond_t cond;
pthread_mutex_t mutex;

static int detect_process(const char * process_name) //判断进程是否在运行
{
    int n = -1;
    FILE *strm;
    char buf[128]={0};
    sprintf(buf,"ps -ax | grep %s|grep -v grep", process_name);
    if((strm = popen(buf, "r")) != NULL)
    {
        if(fgets(buf, sizeof(buf), strm) != NULL)
        {
            n = atoi(buf);
        }
    }
    else
    {
        return -1;
    }
    pclose(strm);
    return n;
}

void *pget_voice(void *arg){
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0X55, 0xAA};
    int len = 0;
    printf("%s|%s|%d\n", __FILE__, __func__, __LINE__);
    if (-1 == serial_fd)
    {
        printf("%s|%s|%d: open serial failed\n", __FILE__, __func__, __LINE__);
        pthread_exit(0);
    }
    printf("%s|%s|%d\n", __FILE__, __func__, __LINE__);
    while(1)
    {
        len = serialGetstring(serial_fd, buffer);
        printf("%s|%s|%d, len=%d\n", __FILE__, __func__, __LINE__,len);
        if (len > 0 && buffer[2] == 0x46)
        {
            printf("%s|%s|%d\n", __FILE__, __func__, __LINE__);
            pthread_mutex_lock(&mutex);
            buffer[2] = 0x00;
            //当调用此函数时，它会唤醒一个正在等待条件变量的线程。
            //如果当前没有线程在等待，那么信号会被丢弃，直到有线程开始等待
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);
        }
    }

    pthread_exit(0);

}

void *psend_voice(void *arg){
    pthread_detach(pthread_self());//子线程结束之后会自己去释放本·线程的资源
    unsigned char *buffer = (unsigned char *)arg;
    if (-1 == serial_fd)
    {
        printf("%s|%s|%d: open serial failed\n", __FILE__, __func__, __LINE__);
        pthread_exit(0);
    }
    if (NULL != buffer)
    {
        serialSendstring(serial_fd, buffer, 6);
    }
    pthread_exit(0);
}

void *popen_trash_can(void *arg){
        pthread_detach(pthread_self());
    unsigned char *buffer = (unsigned char *)arg;
    if (buffer[2] == 0x43)
    {
        printf("%s|%s|%d: buffer[2]=0x%x\n", __FILE__, __func__, __LINE__,buffer[2]);
        pwm_write(PWM_RECOVERABLE_GARBAGE);
        delay(2000);
        pwm_stop(PWM_RECOVERABLE_GARBAGE);
    }
    else if (buffer[2] != 0x45)
    {
        printf("%s|%s|%d: buffer[2]=0x%x\n", __FILE__, __func__, __LINE__,
        buffer[2]);
        pwm_write(PWM_GARBAGE);
        delay(2000);
        pwm_stop(PWM_GARBAGE);
    }
    pthread_exit(0);
}

void *poled_show(void *arg)
{
    pthread_detach(pthread_self());
    myoled_init();
    oled_show(arg);
    pthread_exit(0);
}

void *pcategory(void *arg){
    unsigned char buffer[6] = {0xAA, 0x55, 0x00, 0x00, 0X55, 0xAA};
    char *category = NULL;
    pthread_t send_voice_tid, trash_tid, oled_tid;
    while (1)
    {
        printf("%s|%s|%d: \n", __FILE__, __func__, __LINE__);
        pthread_mutex_lock(&mutex);

        //pthread_cond_wait当线程调用此函数时，它会释放之前获取的互斥锁，然后阻塞自己直到另一个线程通过 pthread_cond_signal() 
        //或 pthread_cond_broadcast() 唤醒它。当线程被唤醒时，它会重新尝试获取互斥锁，以便可以检查条件是否满足。
        pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex);
        printf("%s|%s|%d: \n", __FILE__, __func__, __LINE__);
        buffer[2] = 0x00;
        system(WGET_CMD);
        if (0 == access(GARBAGE_FILE, F_OK))
        {
            category = garbage_category(category);
            if (strstr(category, "干垃圾"))
            {
                buffer[2] = 0x41;
            }
            else if (strstr(category, "湿垃圾"))
            {
                buffer[2] = 0x42;
            }
            else if (strstr(category, "可回收垃圾"))
            {
                buffer[2] = 0x43;
            }
            else if (strstr(category, "有害垃圾"))
            {
                buffer[2] = 0x44;
            }
            else{
                buffer[2] = 0x45;
            }
        }else{
            buffer[2] = 0x45;
        }
        //开垃圾桶开关
        pthread_create(&trash_tid, NULL, psend_voice, (void *)buffer);
        //开语音播报线程
        pthread_create(&send_voice_tid, NULL, popen_trash_can, (void *)buffer);
        //oled显示线程
        pthread_create(&oled_tid, NULL, poled_show, (void *)buffer);
        //buffer[2] = 0x00;
        remove(GARBAGE_FILE);
    }
    pthread_exit(0);
}

void *pget_socket(void *arg){
    int s_fd = -1;//服务端
    int c_fd = -1;//客户端
    char buffer[6];
    int nread = -1;

    struct sockaddr_in c_addr;
    memset(&c_addr,0,sizeof(struct sockaddr_in));
    s_fd = socket_init(IPADDR, IPPORT);//服务端初始化
    printf("%s|%s|%d:s_fd=%d\n", __FILE__, __func__, __LINE__, s_fd);
    if (-1 == s_fd)
    {
        pthread_exit(0);
    }
    sleep(3);
    int clen = sizeof(struct sockaddr_in);

    while (1)
    {
        c_fd = accept(s_fd,(struct sockaddr *)&c_addr,&clen);//等待连接

        int keepalive = 1; // 开启TCP KeepAlive功能
        int keepidle = 5; // tcp_keepalive_time 3s内没收到数据开始发送心跳包
        int keepcnt = 3; // tcp_keepalive_probes 每次发送心跳包的时间间隔,单位秒
        int keepintvl = 3; // tcp_keepalive_intvl 每3s发送一次心跳包
        setsockopt(c_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive,sizeof(keepalive));
        setsockopt(c_fd, SOL_TCP, TCP_KEEPIDLE, (void *) &keepidle, sizeof(keepidle));
        setsockopt(c_fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcnt, sizeof(keepcnt));
        setsockopt(c_fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepintvl, sizeof(keepintvl));

        printf("%s|%s|%d: Accept a connection from %s:%d\n", __FILE__, __func__,__LINE__, inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port));

        if(c_fd == -1)//连接失败，打印错误信息并跳过本次循环等待下次连接
        {
            perror("accept");
            continue;
        }

        while(1)
        {
            memset(buffer, 0, sizeof(buffer));
            nread = recv(c_fd, buffer, sizeof(buffer), 0); //n_read = read(c_fd,buffer, sizeof(buffer));

            printf("%s|%s|%d:nread=%d, buffer=%s\n", __FILE__, __func__,__LINE__, nread, buffer);
            if (nread > 0)
            {
                if (strstr(buffer, "open"))
                {
                    pthread_mutex_lock(&mutex);
                    pthread_cond_signal(&cond);
                    pthread_mutex_unlock(&mutex);
                }
            }else if(0 == nread || -1 == nread)
            {
                break;
            }
        }
        close(c_fd);
    }
    
    pthread_exit(0);
}



int main(int argc,char *argv[]){
    //int serial_id = -1;
    int len = 0;
    int ret = 0;
    unsigned char buffer[6] = {0XAA,0X55,0X00,0X00,0X55,0XAA};
    char *category = NULL;
    pthread_t get_voice_tid, category_tid,get_socket_tid;

    wiringPiSetup();

    garbage_init();//c调python代码初始化

    ret = detect_process("mjpg_streamer");//用于判断mjpg_streamer服务是否已经启动
    if ( -1 == ret)
    {
        //goto END;
    }

    serial_fd = myserialOpen(SERIAL_DEV,BAUD);

    if (serial_fd == -1){//打开串口失败
        printf("打开串口失败");
        goto END;
    }

    //开语音线程
    printf("%s|%s|%d\n", __FILE__, __func__, __LINE__);
    pthread_create(&get_voice_tid, NULL, pget_voice, NULL);
    //开网络线程
    pthread_create(&get_socket_tid,NULL, pget_socket, NULL);
    //开阿里云交互线程
    printf("%s|%s|%d\n", __FILE__, __func__, __LINE__);


    pthread_create(&category_tid, NULL, pcategory, NULL);
    pthread_join(get_voice_tid, NULL);
    pthread_join(category_tid, NULL);
    pthread_join(get_socket_tid, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);


    close(serial_fd);
END:
    garbage_final();
    return 0;
    
}


/*
int main(int argc,char *argv[]){

    int serial_id = -1;
    int len = 0;
    int ret = 0;
    unsigned char buffer[6] = {0XAA,0X55,0X00,0X00,0X55,0XAA};
    char *category = NULL;

    wiringPiSetup();

    garbage_init();//c调python代码初始化

    ret = detect_process("mjpg_streamer");//用于判断mjpg_streamer服务是否已经启动
    if ( -1 == ret)
    {
        //goto END;
    }

    serial_id = myserialOpen(SERIAL_DEV,BAUD);

    if (serial_id == -1){//打开串口失败
        printf("打开串口失败");
        goto END;
    }

    while(1){
        len = serialGetstring(serial_id,buffer);//接受唤醒指令传过来的数据
        printf("len=%d, buf[2]=0x%x\n",len, buffer[2]);
        if(len >0 && buffer[2] == 0x46)
        {
            buffer[2] = 0x00;
            system(WGET_CMD);//调用指令让摄像头拍照

            if (access(GARBAGE_FILE,F_OK)==0)//判断照片是否存在
            {
                category = garbage_category(category);//存在，调用阿里云python接口进行图像识别，并返回数据
                if (strstr(category,"干垃圾"))
                {
                    buffer[2] = 0x41;
                }else if (strstr(category,"湿垃圾"))
                {
                    buffer[2] = 0x42;
                }else if (strstr(category,"可回收垃圾"))
                {
                    buffer[2] = 0x43;
                }else if (strstr(category,"有害垃圾"))
                {
                    buffer[2] = 0x44;
                }else{
                    buffer[2] = 0x45; 
                }
                
            }else{
                buffer[2] = 0x45;
            }
            serialSendstring(serial_id,buffer,6);//发送数据给语音模块，让其播报垃圾类型
            if(buffer[2] == 0x43){//这里控制舵机让垃圾桶开盖5秒，然后在关闭，如果是可回收垃圾的话
                pwm_write(PWM_RECOVERABLE_GARBAGE);
                delay(5000);
                pwm_stop(PWM_RECOVERABLE_GARBAGE);
            }
            buffer[2] = 0x00;
            remove(GARBAGE_FILE);//清除照片
        }

    }
    close(serial_id);
END:
    garbage_final();
    return 0;
    
}
*/