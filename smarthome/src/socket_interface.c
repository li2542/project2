#include "socket_interface.h"
#include "msg_queue.h"
#include "uartTool.h"
#include <pthread.h>
#include "global.h"
#include "socket.h"


static int s_fd = -1;

static int tcpsocket_init(){
    s_fd = socket_init(IPADDR,IPPORT);
    return s_fd;
}

static void tcpsocket_final(){ 
    close(s_fd);
    s_fd = -1;

}

static void* tcpsocket_get(void *arg){
    
    int c_fd = -1;//客户端
    int ret = -1;
    char buffer[BUF_SIZE];
    int nread = -1;
    int keepalive = 1; // 开启TCP KeepAlive功能
    int keepidle = 10; // tcp_keepalive_time 3s内没收到数据开始发送心跳包
    int keepcnt = 3; // tcp_keepalive_probes 每次发送心跳包的时间间隔,单位秒
    int keepintvl = 5; // tcp_keepalive_intvl 每3s发送一次心跳包

    mqd_t mqd = -1;//消息队列描述符
    ctrl_info_t *ctrl_info= NULL;
    struct sockaddr_in c_addr;  
    memset(&c_addr,0,sizeof(struct sockaddr_in));
    //s_fd = socket_init(IPADDR, IPPORT);//服务端初始化
    printf("%s|%s|%d:s_fd=%d\n", __FILE__, __func__, __LINE__, s_fd);
    if (-1 == s_fd) 
    {
        s_fd = tcpsocket_init();//初始化
        if(s_fd == -1){
            printf("tcpsocket_init failed\n");
            pthread_exit(0);
        }
    }

    if (NULL != arg)
        ctrl_info = (ctrl_info_t *)arg;
    if(NULL != ctrl_info)
    {
        mqd = ctrl_info->mqd;
    }
    if ((mqd_t)-1 == mqd)
    {
        pthread_exit(0);
    }
    //sleep(3);
    

    int clen = sizeof(struct sockaddr_in);
    printf("%s thread start\n", __func__);
    while (1)
    {
        c_fd = accept(s_fd,(struct sockaddr *)&c_addr,&clen);//等待连接
        if(c_fd == -1)//连接失败，打印错误信息并跳过本次循环等待下次连接
        {
            perror("accept");
            continue;
        }
        
        ret = setsockopt(c_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive,sizeof(keepalive));// 设置TCP_KEEPALIVE选项
        if (ret == -1) { // 如果设置失败，打印错误信息并跳出循环
            perror("setsockopt");
            break;
        }
        ret = setsockopt(c_fd, SOL_TCP, TCP_KEEPIDLE, (void *) &keepidle, sizeof(keepidle));// 设置探测时间间隔选项
        if (ret == -1) { // 如果设置失败，打印错误信息并跳出循环
            perror("setsockopt");
            break;
        }
        ret = setsockopt(c_fd, SOL_TCP, TCP_KEEPCNT, (void *)&keepcnt, sizeof(keepcnt));// 设置探测包发送次数选项
        if (ret == -1) { // 如果设置失败，打印错误信息并跳出循环
            perror("setsockopt");
            break;
        }
        ret = setsockopt(c_fd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepintvl, sizeof(keepintvl));// 设置探测包发送间隔选项
        if (ret == -1) { // 如果设置失败，打印错误信息并跳出循环
            perror("setsockopt");
            break;
        }
        printf("%s|%s|%d: Accept a connection from %s:%d\n", __FILE__, __func__,__LINE__, inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port));

        

        while(1)//数据传输阶段
        {
            memset(buffer, 0, BUF_SIZE);//重置缓冲区数据
            //接收客户端发送过来的数据
            nread = recv(c_fd, buffer, BUF_SIZE, 0); //n_read = read(c_fd,buffer, sizeof(buffer));

            printf("%s|%s|%d: 0x%x, 0x%x,0x%x, 0x%x, 0x%x,0x%x\n", __FILE__,__func__, __LINE__,
            buffer[0], buffer[1], buffer[2], buffer[3],buffer[4],buffer[5]);
            if (nread > 0)
            {
                printf("%s|%s|%d,nread=%d\n", __FILE__, __func__, __LINE__,nread);
                if(buffer[0] == 0xAA && buffer[1] == 0x55 && buffer[4] == 0x55 && buffer[5] == 0xAA)
                {
                    printf("%s|%s|%d:send 0x%x, 0x%x,0x%x, 0x%x, 0x%x,0x%x\n",__FILE__, __func__, 
                    __LINE__, buffer[0], buffer[1], buffer[2], buffer[3],buffer[4],buffer[5]);
                    send_message(mqd, buffer, nread);//注意，不要用strlen去计算实际的长度
                }
            }else if(0 == nread || -1 == nread){
                break;
            }
        }
        //close(c_fd);
    }
    
    pthread_exit(0);
}


struct control tcpsocket_control = {
    .control_name = "tcpsocket",
    .init = tcpsocket_init,
    .final = tcpsocket_final,
    .get = tcpsocket_get,
    .set = NULL,
    .next = NULL
};

/***
    struct control *phead 要插入的链表的头结点
    
*/
//头插法
struct control *add_tcpsocket_to_ctrl_list(struct control *phead){
    
    // if(phead == NULL){//如果链表为空 
    //     phead = &tcpsocket_control;
    // }else{//不为空将当前节点指向链表头结点
    //     tcpsocket_control.next = phead;
    //     phead  = &tcpsocket_control;
    // }
    // return phead;
    return add_interface_to_ctrl_list(phead,&tcpsocket_control);
}