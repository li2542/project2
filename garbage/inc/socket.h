#ifndef __SOCKET__H
#define __SOCKET__H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#define IPADDR "192.168.1.50" //填写自己实际的ip地址
#define IPPORT "8192"
#define BUF_SIZE 6
int socket_init(const char *ipaddr, const char *port);
#endif