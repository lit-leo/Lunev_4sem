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
    const unsigned udp_port = 8886;
    const unsigned tcp_port = 8888;
    /*UDP - broadcast connection*/
    
    int udp_socket;
    if((udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("UDP client socket creation");
        exit(EXIT_FAILURE);
    } 

    /*get current ip adress*/
    struct ifreq ifr_ip;
    ifr_ip.ifr_addr.sa_family = AF_INET;
    strncpy(ifr_ip.ifr_name, "wlp3s0", IFNAMSIZ-1);
    if(ioctl(udp_socket, SIOCGIFADDR, &ifr_ip) == -1)
    {
        perror("Getting ip address");
        exit(EXIT_FAILURE);
    }
    struct in_addr ip_addr = ((struct sockaddr_in *)&ifr_ip.ifr_addr)->sin_addr;

    /*get broadcast address*/
    struct ifreq ifr_brd;
    ifr_brd.ifr_addr.sa_family = AF_INET;
    strncpy(ifr_brd.ifr_name, "wlp3s0", IFNAMSIZ-1);
    if(ioctl(udp_socket, SIOCGIFBRDADDR, &ifr_brd) == -1)
    {
        perror("Getting broadcast address:");
        exit(EXIT_FAILURE);
    }

    struct in_addr brd_addr = ((struct sockaddr_in *)&ifr_brd.ifr_addr)->sin_addr;

    struct sockaddr_in sock_in;
    sock_in.sin_addr.s_addr = ip_addr.s_addr;
    sock_in.sin_port = htons(udp_port);
    sock_in.sin_family = AF_INET;

    /*bind*/
    if(bind(udp_socket, (struct sockaddr*)&sock_in, sizeof(struct sockaddr)) == -1)
    {
        perror("UDP client socket binding");
        exit(EXIT_FAILURE);
    }

    int enable_brd = 1;
    if(setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &enable_brd, sizeof(int)) == -1)
    {
        perror("UDP client socket broadcast enabling");
        exit(EXIT_FAILURE);
    }

    sock_in.sin_addr.s_addr = brd_addr.s_addr;
    sock_in.sin_port = htons(udp_port);
    sock_in.sin_family = AF_INET;

    sendto(udp_socket, &ip_addr, sizeof(struct in_addr), 0,
        (struct sockaddr *)&sock_in, sizeof(struct sockaddr_in));

    close(udp_socket);

    /*tcp connection*/
    int tcp_sock = socket(AF_INET , SOCK_STREAM , 0);
    if(tcp_sock == -1)
    {
        perror("TCP client socket creation");
        exit(EXIT_FAILURE);
    }

    sock_in.sin_addr.s_addr = ip_addr.s_addr;
    sock_in.sin_port = htons(tcp_port);
    sock_in.sin_family = AF_INET;
    if(bind(tcp_sock, (struct sockaddr*)&sock_in, sizeof(struct sockaddr)) == -1)
    {
        perror("TCP client socket binding");
        exit(EXIT_FAILURE);
    }

    listen(tcp_sock, 1);
    int server_fd[1];

    struct sockaddr_in server;
    unsigned int sockaddr_len = sizeof(struct sockaddr);
    server_fd[0] = accept(tcp_sock, (struct sockaddr *)&server, (socklen_t *)&sockaddr_len);
    if(server_fd[0] < 0)
    {
        printf("TCP client accept: Connection failed\n");
        exit(EXIT_FAILURE);
    }

    char buffer[9];
    recv(server_fd[0], buffer, 9, 0);
    printf("%s\n", buffer);

    close(server_fd[0]);
    close(tcp_sock);
    
    return 0;

}