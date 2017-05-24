#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> /* for strncpy */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#define NET_DEBUG

#define EXIT_FAILURE 1
 
int main(int argc , char *argv[])
{
    /*UDP connection*/
    const unsigned udp_port = 8886;
    const unsigned tcp_port = 8888;

    int udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(udp_sock == -1)
    {
        perror("UDP Server socket creation");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sock_in;
    sock_in.sin_addr.s_addr = INADDR_ANY;
    sock_in.sin_port = htons(udp_port);
    sock_in.sin_family = AF_INET;

    /*bind*/
    if(bind(udp_sock, (struct sockaddr *)&sock_in, sizeof(struct sockaddr)) == -1)
    {
        perror("UDP server binding");
        exit(EXIT_FAILURE);
    }

    unsigned int sockaddr_len = sizeof(struct sockaddr);
    struct in_addr client_addr;
    memset(&client_addr, 0, sizeof(struct in_addr));
    recvfrom(udp_sock, &client_addr, sizeof(struct in_addr), 0, 
        (struct sockaddr *)&sock_in, &sockaddr_len);
    /* I want to get an IPv4 IP address */
    //ifr.ifr_addr.sa_family = AF_INET;

    /* I want IP address attached to "wlp3s0" */

    #ifdef NET_DEBUG
    /* display result */
    printf("recv = %s\n", inet_ntoa(((struct sockaddr_in *)&sock_in)->sin_addr));
    printf("content = %s\n", inet_ntoa(client_addr));
    #endif
    close(udp_sock);
    
    /*tcp connections*/
    int tcp_sock = socket(AF_INET , SOCK_STREAM , 0);
    if(tcp_sock == -1)
    {
        perror("TCP server socket creation");
        exit(EXIT_FAILURE);
    }
    sock_in.sin_port = tcp_port;

    if(connect(tcp_sock, (struct sockaddr *)&sock_in, sockaddr_len) < 0)
    {
        printf("TCP server connect: Connection unsuccessful\n");
        exit(EXIT_FAILURE);
    }

    if(send(tcp_sock, "tcp-test", 9, 0) < 0)
    {
        printf("TCP server send: Unsuccessful\n");
        exit(EXIT_FAILURE);
    }

    close(tcp_sock);
     
    return 0;
}