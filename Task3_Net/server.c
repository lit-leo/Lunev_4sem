#include "common.h"
#define NET_DEBUG
//#define INET_LOOPBACK

#define EXIT_FAILURE 1

double calculate_integral(int thrds_qty, double left, double right);

long pasreInput(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Usage: ./command <quantity_of_threads>\n");
        exit(-1);
    }
    
    char* endptr = NULL;
    errno = 0;  
    long threads_req = strtol(argv[1], &endptr, 10);
    if (errno != 0)
    {
        perror("Strtol:");
        exit(errno);
    }
    if (*endptr != '\0')
    {
        printf("2nd arg is not a number\n");
        exit(-1);
    }

    return threads_req;
}
 
int main(int argc , char *argv[])
{
    const unsigned udp_port = 8886;
    const unsigned tcp_port = 8888;
    const int threads_req = (int)pasreInput(argc, argv);

    //UDP connection
    int udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(udp_sock == -1)
    {
        perror("UDP Server socket creation");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sock_in;
    sock_in.sin_addr.s_addr = INADDR_ANY;
    sock_in.sin_port = htons(udp_port);
    sock_in.sin_family = AF_INET;/**/

    //bind
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

    #ifdef NET_DEBUG
    //display result
    printf("recv = %s\n", inet_ntoa(((struct sockaddr_in *)&sock_in)->sin_addr));
    printf("content = %s\n", inet_ntoa(client_addr));
    #endif
    close(udp_sock);
    
    //tcp connections
    int tcp_sock = socket(AF_INET , SOCK_STREAM , 0);
    if(tcp_sock == -1)
    {
        perror("TCP server socket creation");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in dest;
    dest.sin_addr = client_addr;
    #ifdef INET_LOOPBACK
    sock_in.sin_addr.s_addr = inet_addr("127.0.0.1");
    #endif
    dest.sin_port = htons(tcp_port);
    dest.sin_family = AF_INET;

    sleep(2);

    if(connect(tcp_sock, (struct sockaddr *)&dest, sizeof(struct sockaddr)) < 0)
    {
        printf("TCP server connect: Connection unsuccessful\n");
        exit(EXIT_FAILURE);
    }

    if(send(tcp_sock, &threads_req, sizeof(int), 0) < 0)
    {
        printf("TCP server send: Unsuccessful\n");
        exit(EXIT_FAILURE);
    }

    double left, right;
    if(recv(tcp_sock, &left, sizeof(double), 0) < 0)
    {
        printf("TCP server recv: Unsuccessful\n");
        exit(EXIT_FAILURE);
    }

    if(recv(tcp_sock, &right, sizeof(double), 0) < 0)
    {
        printf("TCP server recv: Unsuccessful\n");
        exit(EXIT_FAILURE);
    }

    /*double left = 1;
    double right = 2;*/
    double res = calculate_integral(threads_req, left, right);
    printf("res = %g\n", res);
    if(send(tcp_sock, &res, sizeof(double), 0) < 0)
    {
        printf("TCP server send: Unsuccessful\n");
        exit(EXIT_FAILURE);
    }

    close(tcp_sock);
     
    return 0;
}